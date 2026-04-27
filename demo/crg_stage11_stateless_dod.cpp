// Copyright (c) 2024-2026 Cyril Tissier. All rights reserved.
// Licensed under the Apache License, Version 2.0.
//
// =============================================================================
// CAPABILITY ROUTING GATEWAY (CRG) - STAGE 11: STATELESS DOD (THE CEILING)
// =============================================================================
//
// @intent:
// Achieve Zero-Cost Data-Oriented Polymorphism by eradicating VTables completely.
// Combine the Flat Tensor (Broad Phase) and Dynamic Rules (Narrow Phase) with 
// pure stateless function pointers.
//
// @what_changed:
// - VTable Eradication: The Interface (e.g., ICombat) is no longer abstract. 
//   It holds a raw C-style function pointer and wraps it (The Polymorphism Illusion).
// - The Sandbox: Behaviors no longer receive 'this' or generic handles. They 
//   receive a strictly typed Data Contract (Params).
// - Synthesized Node: The Baker extracts the static `Execute` function from the 
//   Gameplay logic and binds it directly into the RouteNode.
//
// @key_insight:
// We have reached the hardware limit. The CPU evaluates an O(1) math offset, 
// checks a quick contiguous rule, and jumps directly to a static memory address.
// Zero object allocations, zero virtual overhead, absolute L1 cache bliss.
// =============================================================================

#include <iostream>
#include <vector>
#include <string>
#include <tuple>
#include <type_traits>

// =============================================================================
// [ ENGINE CORE ] 1. IDENTITY
// =============================================================================
using DenseID = std::size_t;

struct TypeIDGenerator {
    static DenseID GetNext() { static DenseID s_id = 0; return s_id++; }
};

template<class T> 
struct DenseTypeID { 
    static DenseID Get() { static DenseID s_id = TypeIDGenerator::GetNext(); return s_id; } 
};

template<typename... Ts> struct TypeList {};
template<auto V> using When = std::integral_constant<decltype(V), V>;

// =============================================================================
// [ ENGINE CORE ] 2. TENSOR MATH (The Selector)
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
            if constexpr (EnumTraits<EnumAt<I>>::HasAny) val -= 1; 
            return val * Stride<I>() + ComputeOffsetImpl<I + 1>(args...);
        }
    }
};

// =============================================================================
// [ ENGINE CORE ] 3. NARROW PHASE NODE
// =============================================================================
template<class InterfaceT>
struct RouteNode {
    using CondFunc = bool(*)(const typename InterfaceT::RuleContext&);
    
    CondFunc Condition;
    bool isFallback;
    
    // The "Polymorphism Illusion" Descriptor
    InterfaceT descriptor; 
    
    RouteNode* next = nullptr;
};

// =============================================================================
// [ ENGINE CORE ] 4. FLAT TENSOR ROUTER (O(1) + Narrow Phase)
// =============================================================================
template<class InterfaceT>
class BehaviorRouter {
    static inline std::vector<std::vector<RouteNode<InterfaceT>*>> s_flat_tensor;

public:
    static void Register(DenseID modelID, std::size_t offset, RouteNode<InterfaceT>* node) {
        if (modelID >= s_flat_tensor.size()) {
            s_flat_tensor.resize(modelID + 1, std::vector<RouteNode<InterfaceT>*>(InterfaceT::Context::Volume(), nullptr));
        }

        auto& cell = s_flat_tensor[modelID][offset];

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
        if (modelID >= s_flat_tensor.size()) return nullptr;
        std::size_t offset = InterfaceT::Context::ComputeOffset(args...); 
        
        for (auto* node = s_flat_tensor[modelID][offset]; node; node = node->next) {
            if (node->Condition(ctx)) return &node->descriptor;
        }
        return nullptr;
    }
};

// =============================================================================
// [ ENGINE CORE ] 5. DOD BAKER
// =============================================================================
template<class InterfaceT, class ModelT, class... Constraints>
struct CapabilityDefinition {
    using InterfaceType = InterfaceT;
    
    static constexpr bool IsFallback = true;
    static bool Condition(const typename InterfaceT::RuleContext&) { return true; }

    static constexpr bool MatchIndex(std::size_t offset) { return MatchIndexImpl<0>(offset); }
private:
    template<std::size_t I>
    static constexpr bool MatchIndexImpl(std::size_t offset) {
        if constexpr (I == sizeof...(Constraints)) return true;
        else {
            using Context = typename InterfaceT::Context;
            using EnumType = typename Context::template EnumAt<I>;
            using ConstraintWrapper = std::tuple_element_t<I, std::tuple<Constraints...>>;
            
            std::size_t c_val = (offset / Context::template Stride<I>()) % Context::template DimSize<I>();
            auto runtime_val = static_cast<EnumType>(c_val + (EnumTraits<EnumType>::HasAny ? 1 : 0));
            
            return (EnumTraits<EnumType>::HasAny && (static_cast<std::size_t>(ConstraintWrapper::value) == 0) || 
                   (ConstraintWrapper::value == runtime_val)) && MatchIndexImpl<I + 1>(offset);
        }
    }
};

template<class ModelT, template<class> class... Behaviors>
class BakedDODNode {
    template<class InterfaceT, class ConcreteT>
    void RegisterConcrete() {
        // We capture the static &ConcreteT::Execute and pass it to the Descriptor
        static RouteNode<InterfaceT> s_node {
            &ConcreteT::Condition, 
            ConcreteT::IsFallback, 
            { &ConcreteT::Execute }, // <--- The Polymorphism Illusion binds here!
            nullptr
        };

        for (std::size_t i = 0; i < InterfaceT::Context::Volume(); ++i) {
            if (ConcreteT::MatchIndex(i)) {
                BehaviorRouter<InterfaceT>::Register(DenseTypeID<ModelT>::Get(), i, &s_node);
            }
        }
    }

public:
    BakedDODNode() {
        (RegisterConcrete<typename Behaviors<ModelT>::InterfaceType, Behaviors<ModelT>>(), ...);
    }
};

template<class ModelList, template<class> class... Behaviors> struct DomainBaker;
template<class... Models, template<class> class... Behaviors>
struct DomainBaker<TypeList<Models...>, Behaviors...> {
    std::tuple<BakedDODNode<Models, Behaviors...>...> m_nodes;
};


// =============================================================================
// [ GAMEPLAY SPACE ] 1. DATA, ENUMS & SANDBOX
// =============================================================================
enum class State { Any, Idle, Aggressive };
template<> struct EnumTraits<State> { static constexpr std::size_t Count = 3; static constexpr bool HasAny = true; };

struct EntityData { int health; };
struct CombatSettings { int criticalThreshold = 30; int dmgBonus = 10; };

struct Scout {};

// =============================================================================
// [ GAMEPLAY SPACE ] 2. THE CONTRACT (STATELESS INTERFACE)
// =============================================================================
struct ICombat {
    using Context = Selector<State>;
    
    struct RuleContext { const EntityData& entity; };
    struct Params { EntityData& entity; const CombatSettings& settings; };

    // The Function Pointer (No VTable!)
    void (*pfnExecute)(Params&);

    // Static wrapper (The Illusion)
    void Execute(Params& p) const { pfnExecute(p); }
};

// =============================================================================
// [ GAMEPLAY SPACE ] 3. BEHAVIORS (Pure Static Logic)
// =============================================================================
template<class T>
struct StandardStrikeLogic : CapabilityDefinition<ICombat, T, When<State::Aggressive>> {
    static void Execute(ICombat::Params& p) {
        std::cout << " [DOD] Standard Strike! Base Damage applied.\n";
        p.entity.health -= 5; 
    }
};

template<class T>
struct CriticalStrikeLogic : CapabilityDefinition<ICombat, T, When<State::Aggressive>> {
    static constexpr bool IsFallback = false;
    static bool Condition(const ICombat::RuleContext& ctx) {
        return ctx.entity.health < 50; // Critical mode triggers below 50 HP
    }

    static void Execute(ICombat::Params& p) {
        std::cout << " [DOD] CRITICAL STRIKE! Bonus Damage applied.\n";
        p.entity.health -= (5 + p.settings.dmgBonus); 
    }
};

// Injection
using ScoutModels = TypeList<Scout>;
static const DomainBaker<ScoutModels, StandardStrikeLogic, CriticalStrikeLogic> g_ScoutModule{};

// =============================================================================
// [ THE SYSTEM ] HOT PATH SIMULATION
// =============================================================================
int main() {
    std::cout << "--- CRG STAGE 11: STATELESS DOD & NARROW PHASE ---\n\n";

    DenseID scoutID = DenseTypeID<Scout>::Get();
    CombatSettings settings;

    std::cout << "--- SCENARIO 1: Scout at 100 HP ---\n";
    EntityData healthyScout { 100 };
    ICombat::RuleContext ruleCtx1 { healthyScout };
    ICombat::Params sandbox1 { healthyScout, settings };

    if (auto* combat = BehaviorRouter<ICombat>::Find(scoutID, ruleCtx1, State::Aggressive)) {
        combat->Execute(sandbox1); // DOD static jump!
    }

    std::cout << "\n--- SCENARIO 2: Scout at 40 HP ---\n";
    EntityData hurtScout { 40 };
    ICombat::RuleContext ruleCtx2 { hurtScout };
    ICombat::Params sandbox2 { hurtScout, settings };

    if (auto* combat = BehaviorRouter<ICombat>::Find(scoutID, ruleCtx2, State::Aggressive)) {
        combat->Execute(sandbox2); // DOD static jump!
    }

    return 0;
}