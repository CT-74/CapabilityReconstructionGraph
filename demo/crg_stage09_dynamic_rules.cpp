// Copyright (c) 2024-2026 Cyril Tissier. All rights reserved.
// Licensed under the Apache License, Version 2.0.
//
// =============================================================================
// CAPABILITY ROUTING GATEWAY (CRG) - STAGE 9: BROAD & NARROW PHASE
// =============================================================================
//
// @intent:
// Handle continuous logic (Time, Health, Distance) without breaking the O(1) 
// tensor. Introduce the concept of a Narrow Phase filter inside the Broad Phase.
//
// @what_changed:
// - The Tensor cells no longer hold a single pointer, but a `RouteNode` linked list.
// - Broad Phase (O(1)): Math calculation to find the correct Tensor cell.
// - Narrow Phase (O(K)): Small linear traversal of conditions inside that cell.
// - Behaviors can now define a `Condition(RuleContext)` to act as exceptions.
// - The Router automatically prioritizes Exceptions before Fallbacks.
//
// @key_insight:
// The N-Dimensional Tensor handles discrete state (Zone, Stance).
// The Narrow Phase handles continuous state (Health < 30, Time > 18.0f).
// This is the definitive resolution architecture.
// =============================================================================

#include <iostream>
#include <vector>
#include <string>
#include <tuple>
#include <typeinfo>
#include <memory>

// =============================================================================
// [ ENGINE CORE ] 1. IDENTITY & SEMANTICS 
// =============================================================================
using ModelTypeID = std::size_t;
template<class T> struct TypeIDOf { static ModelTypeID Get() { return typeid(T).hash_code(); } };
template<typename... Ts> struct TypeList {};

template<auto V> using When = std::integral_constant<decltype(V), V>;
template<auto V> using In   = std::integral_constant<decltype(V), V>;

// =============================================================================
// [ ENGINE CORE ] 2. TYPE ERASED SHELL (Pedagogical version)
// =============================================================================
class ModelHandle {
public:
    struct Concept { virtual ~Concept() = default; virtual ModelTypeID GetTypeID() const = 0; };
    template<class T> struct Model : Concept {
        T value; Model(T v) : value(std::move(v)) {}
        ModelTypeID GetTypeID() const override { return TypeIDOf<T>::Get(); }
    };
    
    ModelHandle() = default;
    template<class T> void Set(T v) { m_ptr = std::make_unique<Model<T>>(std::move(v)); }
    ModelTypeID GetTypeID() const { return m_ptr ? m_ptr->GetTypeID() : 0; }
private:
    std::unique_ptr<Concept> m_ptr;
};

// =============================================================================
// [ ENGINE CORE ] 3. TENSOR MATH (Broad Phase Setup)
// =============================================================================
template<typename T> struct EnumTraits;

template<typename... Enums>
struct Selector {
    static constexpr std::size_t Dimensions = sizeof...(Enums);
    template<std::size_t I> using EnumAt = std::tuple_element_t<I, std::tuple<Enums...>>;

    template<std::size_t I>
    static constexpr std::size_t DimSize() {
        using E = EnumAt<I>;
        return EnumTraits<E>::Count - (EnumTraits<E>::HasAny ? 1 : 0);
    }

    template<std::size_t I>
    static constexpr std::size_t Stride() {
        if constexpr (I + 1 >= Dimensions) return 1;
        else return DimSize<I + 1>() * Stride<I + 1>();
    }

    static constexpr std::size_t Volume() { return (DimSize<0>() * ... * 1); }
    static constexpr std::size_t ComputeOffset(Enums... args) { return ComputeOffsetImpl<0>(args...); }

private:
    template<std::size_t I>
    static constexpr std::size_t ComputeOffsetImpl(Enums... args) {
        if constexpr (I == Dimensions) return 0;
        else {
            auto val_enum = std::get<I>(std::make_tuple(args...));
            std::size_t val = static_cast<std::size_t>(val_enum);
            using E = EnumAt<I>;
            if constexpr (EnumTraits<E>::HasAny) val -= 1; 
            return val * Stride<I>() + ComputeOffsetImpl<I + 1>(args...);
        }
    }
};

// =============================================================================
// [ ENGINE CORE ] 4. THE NARROW PHASE NODE
// =============================================================================
template<class InterfaceT>
struct RouteNode {
    using CondFunc = bool(*)(const typename InterfaceT::RuleContext&);
    
    CondFunc Condition;
    bool isFallback;
    const InterfaceT* behavior;
    RouteNode* next = nullptr;
};

// =============================================================================
// [ ENGINE CORE ] 5. THE ROUTER (Broad + Narrow Resolution)
// =============================================================================
template<class InterfaceT>
class BehaviorRouter {
    // Each ModelID points to an array of RouteNodes (The Tensor)
    struct BakedRouter {
        std::vector<ModelTypeID> ids;
        std::vector<std::vector<RouteNode<InterfaceT>*>> tensors; 
    };

    static BakedRouter& GetBaked() { static BakedRouter instance; return instance; }

public:
    static void Register(ModelTypeID modelID, std::size_t offset, RouteNode<InterfaceT>* node) {
        auto& router = GetBaked();
        
        // Find or create the Model's Tensor
        std::size_t mIdx = router.ids.size();
        for (std::size_t i = 0; i < router.ids.size(); ++i) {
            if (router.ids[i] == modelID) { mIdx = i; break; }
        }
        if (mIdx == router.ids.size()) {
            router.ids.push_back(modelID);
            router.tensors.push_back(std::vector<RouteNode<InterfaceT>*>(InterfaceT::Context::Volume(), nullptr));
        }

        auto& cell = router.tensors[mIdx][offset];

        // Sorting: Priority Rules first, Fallbacks last
        if (!cell || (cell->isFallback && !node->isFallback)) {
            node->next = cell;
            cell = node;
        } else {
            auto* curr = cell;
            while (curr->next && !curr->next->isFallback) curr = curr->next;
            node->next = curr->next;
            curr->next = node;
        }
    }

    template<class... Args>
    static const InterfaceT* Find(ModelTypeID modelID, const typename InterfaceT::RuleContext& ctx, Args... args) {
        const auto& router = GetBaked();
        
        // 1. BROAD PHASE: O(1) Math evaluation
        std::size_t offset = InterfaceT::Context::ComputeOffset(args...); 
        
        for (std::size_t i = 0; i < router.ids.size(); ++i) {             
            if (router.ids[i] == modelID) {
                // 2. NARROW PHASE: Loop through candidates
                for (auto* node = router.tensors[i][offset]; node; node = node->next) {
                    if (node->Condition(ctx)) return node->behavior;
                }
            }
        }
        return nullptr;
    }
};

// =============================================================================
// [ ENGINE CORE ] 6. BAKING INFRASTRUCTURE 
// =============================================================================
template<class InterfaceT, class ModelT, class... Constraints>
struct CapabilityDefinition : InterfaceT {
    using InterfaceType = InterfaceT;
    
    // Default Fallback settings (Derived classes can hide these)
    static constexpr bool IsFallback = true;
    static bool Condition(const typename InterfaceT::RuleContext&) { return true; }

    bool MatchIndex(std::size_t offset) const { return MatchIndexImpl<0>(offset); }
private:
    template<std::size_t I>
    bool MatchIndexImpl(std::size_t offset) const {
        if constexpr (I == sizeof...(Constraints)) return true;
        else {
            using Context = typename InterfaceT::Context;
            using EnumType = typename Context::template EnumAt<I>;
            using ConstraintWrapper = std::tuple_element_t<I, std::tuple<Constraints...>>;
            
            constexpr auto constraint_val = ConstraintWrapper::value;
            std::size_t c_val = (offset / Context::template Stride<I>()) % Context::template DimSize<I>();
            EnumType runtime_val = static_cast<EnumType>(c_val + (EnumTraits<EnumType>::HasAny ? 1 : 0));
            
            return (EnumTraits<EnumType>::HasAny && (static_cast<std::size_t>(constraint_val) == 0) || (constraint_val == runtime_val)) && MatchIndexImpl<I + 1>(offset);
        }
    }
};

template<class ModelT, template<class> class... Behaviors>
class BakedCapabilityNode : public Behaviors<ModelT>... {
    
    template<class InterfaceT, class ConcreteT>
    void RegisterConcrete() {
        static RouteNode<InterfaceT> s_node {
            &ConcreteT::Condition, 
            ConcreteT::IsFallback, 
            static_cast<const InterfaceT*>(static_cast<const ConcreteT*>(this)), 
            nullptr
        };

        for (std::size_t i = 0; i < InterfaceT::Context::Volume(); ++i) {
            if (static_cast<const ConcreteT*>(this)->MatchIndex(i)) {
                BehaviorRouter<InterfaceT>::Register(TypeIDOf<ModelT>::Get(), i, &s_node);
            }
        }
    }

public:
    BakedCapabilityNode() {
        (RegisterConcrete<typename Behaviors<ModelT>::InterfaceType, Behaviors<ModelT>>(), ...);
    }
};

template<class ModelList, template<class> class... Behaviors> struct DomainBaker;
template<class... Models, template<class> class... Behaviors>
struct DomainBaker<TypeList<Models...>, Behaviors...> {
    std::tuple<BakedCapabilityNode<Models, Behaviors...>...> m_nodes;
};

// =============================================================================
// [ GAMEPLAY SPACE ] 1. ECS COMPONENTS & STATE
// =============================================================================
enum class State { Any, Idle, Combat };
enum class Zone  { Any, Desert, Snow };

template<> struct EnumTraits<State> { static constexpr std::size_t Count = 3; static constexpr bool HasAny = true; };
template<> struct EnumTraits<Zone>  { static constexpr std::size_t Count = 3; static constexpr bool HasAny = true; };

// ECS Components
struct Health { int value = 100; };
struct Scout  { };

// System Contexts
struct WorldSettings { int critical_health_threshold = 30; };

// =============================================================================
// [ GAMEPLAY SPACE ] 2. INTERFACES
// =============================================================================
struct IMove {
    using InterfaceType = IMove;
    using Context = Selector<State, Zone>; 
    
    // The data required to evaluate Narrow Phase rules
    struct RuleContext { const Health& hp; const WorldSettings& settings; };
    
    virtual void Execute(Health& hp) const = 0;
};

// =============================================================================
// [ GAMEPLAY SPACE ] 3. BEHAVIORS (Rules & Fallbacks)
// =============================================================================

// FALLBACK: Always valid, gets pushed to the end of the cell list.
template<class T> 
struct StandardMoveDef : CapabilityDefinition<IMove, T, When<State::Any>, In<Zone::Any>> { 
    void Execute(Health& hp) const override { std::cout << " [Action] Standard Move executed.\n"; }
};

// PRIORITY EXCEPTION: Only valid if health is critical!
template<class T> 
struct FleeMoveDef : CapabilityDefinition<IMove, T, When<State::Combat>, In<Zone::Any>> { 
    
    // Override the defaults for the Narrow Phase
    static constexpr bool IsFallback = false; 
    static bool Condition(const IMove::RuleContext& ctx) { 
        return ctx.hp.value < ctx.settings.critical_health_threshold; 
    }

    void Execute(Health& hp) const override { 
        std::cout << " [Action] CRITICAL FLEE TRIGGERED! (-5 HP Cost)\n"; 
        hp.value -= 5;
    }
};

// =============================================================================
// [ GAMEPLAY SPACE ] 4. MODULE INSTANTIATION
// =============================================================================
using SimModels = TypeList<Scout>;
// Both definitions are baked into the same matrix. FleeMoveDef will share cells 
// with StandardMoveDef, but its condition will be evaluated first.
static const DomainBaker<SimModels, StandardMoveDef, FleeMoveDef> g_BehaviorModule{};


// =============================================================================
// [ THE SYSTEM ] HOT PATH SIMULATION
// =============================================================================
int main() {
    std::cout << "--- CRG STAGE 9: BROAD PHASE (O(1)) & NARROW PHASE (RULES) ---\n\n";
    
    ModelTypeID scoutID = TypeIDOf<Scout>::Get();
    WorldSettings settings { 30 }; // Critical at 30 HP
    Health my_health { 100 };
    
    std::cout << "--- SCENARIO 1: Full Health (100 HP), Combat State ---\n";
    IMove::RuleContext ctx1 { my_health, settings };
    
    if (auto* move = BehaviorRouter<IMove>::Find(scoutID, ctx1, State::Combat, Zone::Desert)) {
        move->Execute(my_health); 
        // Broad Phase matches both. Narrow Phase evaluates Flee (False), then Standard (True).
    }

    my_health.value = 15; // Oh no.
    
    std::cout << "\n--- SCENARIO 2: Critical Health (15 HP), Combat State ---\n";
    IMove::RuleContext ctx2 { my_health, settings };
    
    if (auto* move = BehaviorRouter<IMove>::Find(scoutID, ctx2, State::Combat, Zone::Desert)) {
        move->Execute(my_health);
        // Broad Phase matches both. Narrow Phase evaluates Flee (True) -> Returns immediately.
    }
    
    return 0;
}