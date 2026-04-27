// Copyright (c) 2024-2026 Cyril Tissier. All rights reserved.
// Licensed under the Apache License, Version 2.0.
//
// =============================================================================
// CAPABILITY ROUTING GATEWAY (CRG) - STAGE 10: THE FLAT TENSOR (DENSE ID)
// =============================================================================
//
// @intent:
// Eradicate structural overhead by replacing associative lookups (std::unordered_map) 
// and non-deterministic hashes with contiguous memory and Dense IDs.
//
// @what_changed:
// - TypeIDOf (hash-based) is replaced by DenseTypeID (contiguous 0, 1, 2...).
// - BehaviorRouter now uses a Flat Vector of Vectors (s_flat_tensor) for pure O(1).
// - Broad Phase & Narrow Phase are preserved but now resolve through direct 
//   hardware-friendly array offsets instead of ID matching loops.
//
// @key_insight:
// Hardware performance is driven by predictability. By forcing logical 
// identities into a dense numerical range, we transform abstract polymorphism 
// into raw, linear memory access. The Silicon limit is in sight.
// =============================================================================

#include <iostream>
#include <vector>
#include <string>
#include <tuple>
#include <typeinfo>

// =============================================================================
// [ ENGINE CORE ] 1. DENSE IDENTITY (HARDWARE OPTIM)
// =============================================================================
using DenseID = std::size_t;

struct TypeIDGenerator {
    static DenseID GetNext() { static DenseID s_id = 0; return s_id++; }
};

template<class T> 
struct DenseTypeID { 
    // Assigns a contiguous, unique index (0, 1, 2...) to each type at runtime.
    static DenseID Get() { static DenseID s_id = TypeIDGenerator::GetNext(); return s_id; } 
};

template<typename... Ts> struct TypeList {};
template<auto V> using When = std::integral_constant<decltype(V), V>;
template<auto V> using In   = std::integral_constant<decltype(V), V>;

// =============================================================================
// [ ENGINE CORE ] 2. TENSOR MATH (Broad Phase Setup)
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
// [ ENGINE CORE ] 3. THE NARROW PHASE NODE
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
// [ ENGINE CORE ] 4. FLAT TENSOR ROUTER (The Ultimate Hot Path)
// =============================================================================
template<class InterfaceT>
class BehaviorRouter {
    // FLAT TENSOR: Outer vector is the DenseID, Inner vector is the Dimensional Math Offset.
    // Replaces loops and hash maps completely!
    static inline std::vector<std::vector<RouteNode<InterfaceT>*>> s_flat_tensor;

public:
    static void Register(DenseID modelID, std::size_t offset, RouteNode<InterfaceT>* node) {
        // Auto-expand the outer Flat Tensor based on the Dense ID
        if (modelID >= s_flat_tensor.size()) {
            s_flat_tensor.resize(modelID + 1, std::vector<RouteNode<InterfaceT>*>(InterfaceT::Context::Volume(), nullptr));
        }

        auto& cell = s_flat_tensor[modelID][offset];

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
    static const InterfaceT* Find(DenseID modelID, const typename InterfaceT::RuleContext& ctx, Args... args) {
        // 1. BOUNDS CHECK (Instant)
        if (modelID >= s_flat_tensor.size()) return nullptr;

        // 2. BROAD PHASE: O(1) Math offset evaluation
        std::size_t offset = InterfaceT::Context::ComputeOffset(args...); 
        
        // 3. NARROW PHASE: Direct Array Jump + Linked List Loop
        for (auto* node = s_flat_tensor[modelID][offset]; node; node = node->next) {
            if (node->Condition(ctx)) return node->behavior;
        }
        
        return nullptr;
    }
};

// =============================================================================
// [ ENGINE CORE ] 5. BAKING INFRASTRUCTURE
// =============================================================================
template<class InterfaceT, class ModelT, class... Constraints>
struct CapabilityDefinition : InterfaceT {
    using InterfaceType = InterfaceT;
    
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
                // Register directly using the Dense ID
                BehaviorRouter<InterfaceT>::Register(DenseTypeID<ModelT>::Get(), i, &s_node);
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

struct Health { int value = 100; };
struct Scout  { };
struct Drone  { }; // Added to prove contiguous IDs

struct WorldSettings { int critical_health_threshold = 30; };

// =============================================================================
// [ GAMEPLAY SPACE ] 2. INTERFACES & BEHAVIORS
// =============================================================================
struct IMove {
    using InterfaceType = IMove;
    using Context = Selector<State, Zone>; 
    struct RuleContext { const Health& hp; const WorldSettings& settings; };
    virtual void Execute(Health& hp) const = 0;
};

// FALLBACK
template<class T> 
struct StandardMoveDef : CapabilityDefinition<IMove, T, When<State::Any>, In<Zone::Any>> { 
    void Execute(Health& hp) const override { std::cout << " [Action] Standard Move executed.\n"; }
};

// EXCEPTION (Narrow Phase Rule)
template<class T> 
struct FleeMoveDef : CapabilityDefinition<IMove, T, When<State::Combat>, In<Zone::Any>> { 
    static constexpr bool IsFallback = false; 
    static bool Condition(const IMove::RuleContext& ctx) { 
        return ctx.hp.value < ctx.settings.critical_health_threshold; 
    }
    void Execute(Health& hp) const override { 
        std::cout << " [Action] CRITICAL FLEE TRIGGERED! (-5 HP Cost)\n"; 
        hp.value -= 5;
    }
};

using SimModels = TypeList<Scout, Drone>;
static const DomainBaker<SimModels, StandardMoveDef, FleeMoveDef> g_BehaviorModule{};

// =============================================================================
// [ THE SYSTEM ] HOT PATH SIMULATION
// =============================================================================
int main() {
    std::cout << "--- CRG STAGE 10: THE FLAT TENSOR (DENSE ID + NARROW PHASE) ---\n\n";
    
    DenseID scoutID = DenseTypeID<Scout>::Get();
    DenseID droneID = DenseTypeID<Drone>::Get();
    
    std::cout << "Engine Type IDs Mapping:\n";
    std::cout << " Scout ID: " << scoutID << "\n";
    std::cout << " Drone ID: " << droneID << "\n";
    std::cout << " Notice they are strictly contiguous (0, 1). Absolute L1 Cache bliss.\n\n";

    WorldSettings settings { 30 }; 
    Health my_health { 15 }; // Critical
    IMove::RuleContext ctx { my_health, settings };
    
    std::cout << "--- SCENARIO: Critical Health (15 HP), Combat State ---\n";
    
    // Direct Array Offset [scoutID][Math] -> Narrow Phase -> Execution
    if (auto* move = BehaviorRouter<IMove>::Find(scoutID, ctx, State::Combat, Zone::Desert)) {
        move->Execute(my_health);
    }
    
    return 0;
}