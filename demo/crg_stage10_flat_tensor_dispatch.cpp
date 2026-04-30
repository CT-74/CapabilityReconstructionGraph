// Copyright (c) 2024-2026 Cyril Tissier. All rights reserved.
// Licensed under the Apache License, Version 2.0.
//
// =============================================================================
// CAPABILITY ROUTING GATEWAY (CRG) - STAGE 10: FLAT TENSOR DISPATCH (DOD)
// =============================================================================
// @intent:
// Flatten the N-D Hypercube into a single contiguous memory arena (Tensor)[cite: 4].
// - Broad Phase: O(1) Indexing via ModelSlot and Cartesian offset[cite: 4].
// - Narrow Phase: O(K) Flat rule scan inside DispatchCells[cite: 4].
// - ModelRouter: Zero-cost static proxy for strongly-typed context routing.
// =============================================================================

#include <iostream>
#include <vector>
#include <tuple>
#include <typeinfo>
#include <algorithm>
#include <unordered_map>
#include <type_traits>

// =============================================================================
// 1. INFRASTRUCTURE & DLL MANAGEMENT
// =============================================================================

#ifndef CRG_DLL_ENABLED
#define CRG_DLL_ENABLED 0 
#endif

template<class T> struct RegistrySlot {
#if !CRG_DLL_ENABLED
    static inline T s_Value{}; 
#else
    static T s_Value; 
#endif
};

#if CRG_DLL_ENABLED
    #define CRG_BIND_SLOT(T) template<> T RegistrySlot<T>::s_Value{};
#else
    #define CRG_BIND_SLOT(T) 
#endif

template<class TNode, class TInterface>
struct NodeList : public TInterface {
    const TNode* m_Next = nullptr;
    NodeList() {
        m_Next = RegistrySlot<const TNode*>::s_Value;
        RegistrySlot<const TNode*>::s_Value = static_cast<const TNode*>(this);
    }
};

using ModelTypeID = std::size_t;
template<class T> struct TypeIDOf { static ModelTypeID Get() { return typeid(T).hash_code(); } };
template<typename... Ts> struct TypeList {};

// =============================================================================
// 2. DOMAINE & CONTEXTES
// =============================================================================

enum class WorldState { Day, Night };
enum class AlertState { Calm, Combat };

template<typename T> struct EnumTraits;
template<> struct EnumTraits<WorldState> { static constexpr std::size_t Count = 2; };
template<> struct EnumTraits<AlertState> { static constexpr std::size_t Count = 2; };

struct EnergyContext { float battery; };

template<typename... TArgs>
struct RuleContext {
    std::tuple<const TArgs&...> values;
    RuleContext(const TArgs&... args) : values(std::tie(args...)) {}
    template<typename T> constexpr const T& Get() const { return std::get<const T&>(values); }
};

// Patch: Fallback context for contracts without dynamic rules[cite: 6]
struct NullContext {
    NullContext() = default;
    template<typename... Args> NullContext(const Args&...) {} 
};

// =============================================================================
// 3. MATHÉMATIQUES DU TENSEUR (HORNER)
// =============================================================================

template<class... TAxes>
struct CapabilitySpace {
    using AxisTuple = std::tuple<TAxes...>;
    static constexpr std::size_t Dimensions = sizeof...(TAxes);
    
    // Safely handles 0D space (no axes provided)
    static constexpr std::size_t Volume = (Dimensions == 0) ? 1 : (EnumTraits<TAxes>::Count * ... * 1);

    template<std::size_t DimIdx>
    static constexpr std::size_t GetStride() {
        if constexpr (Dimensions == 0) return 1;
        else {
            constexpr std::size_t dims[] = { EnumTraits<TAxes>::Count... };
            std::size_t stride = 1;
            for (std::size_t i = DimIdx + 1; i < Dimensions; ++i) stride *= dims[i];
            return stride;
        }
    }

    template<std::size_t DimIdx>
    static constexpr auto GetCoordAtIndex(std::size_t index) {
        if constexpr (Dimensions == 0) return 0;
        else {
            using AxisT = std::tuple_element_t<DimIdx, AxisTuple>;
            return static_cast<AxisT>((index / CapabilitySpace<TAxes...>::template GetStride<DimIdx>()) % EnumTraits<AxisT>::Count);
        }
    }

    // Zero-copy variadic to tuple internal packing
    template<typename... TArgs>
    static constexpr std::size_t ComputeOffset(const TArgs&... args) {
        if constexpr (Dimensions == 0) return 0;
        else return ComputeInternal(std::tie(args...), std::make_index_sequence<Dimensions>{});
    }

private:
    template<typename TupleT, std::size_t... Is>
    static constexpr std::size_t ComputeInternal(const TupleT& t, std::index_sequence<Is...>) {
        const std::size_t coords[] = { static_cast<std::size_t>(std::get<const std::tuple_element_t<Is, AxisTuple>&>(t))... };
        constexpr std::size_t dims[] = { EnumTraits<std::tuple_element_t<Is, AxisTuple>>::Count... };
        std::size_t offset = 0;
        for (std::size_t i = 0; i < Dimensions; ++i) offset = offset * dims[i] + coords[i];
        return offset;
    }
};

// =============================================================================
// 4. ARENA & ROUTING TYPES
// =============================================================================

template<class TInterface> struct CapabilityRoutingTraits;

// Patch: Lazy Context Selector to handle optional FullContext definition[cite: 6]
template<typename T, typename = void> struct ContextSelector { using Type = NullContext; };
template<typename T> struct ContextSelector<T, std::void_t<typename CapabilityRoutingTraits<T>::FullContext>> {
    using Type = typename CapabilityRoutingTraits<T>::FullContext;
};
template<typename T> using ContextTypeOf = typename ContextSelector<T>::Type;

template<class InterfaceT>
struct Rule {
    using ContextT = ContextTypeOf<InterfaceT>; // Use lazy selector
    using PredicatePtr = bool (*)(const void*, const ContextT&);

    const InterfaceT* implementation;
    const void*       configData;
    PredicatePtr      predicate;
    int               priority;

    bool Matches(const ContextT& ctx) const { return !predicate || predicate(configData, ctx); }
};

template<class InterfaceT>
struct DispatchCell {
    std::vector<Rule<InterfaceT>> dynamicRules;
    const InterfaceT* fallback = nullptr;      
};

template<class InterfaceT> using TensorArena = std::vector<DispatchCell<InterfaceT>>;

template<class TInterface, class TConfig = void> 
struct Capability : public TInterface { 
    using InterfaceType = TInterface; 
    using ConfigType = TConfig; 
    TConfig config; 
};

template<class TInterface> 
struct Capability<TInterface, void> : public TInterface { 
    using InterfaceType = TInterface; 
    using ConfigType = void; 
};

// =============================================================================
// 5. BAKER
// =============================================================================

struct IAssembler { virtual void Bake() const = 0; };
struct IBakerNode : public NodeList<IBakerNode, IAssembler> {};
CRG_BIND_SLOT(const IBakerNode*)

template<auto... V> struct At {};
template<class TSpace, std::size_t Index, class IdxSeq = std::make_index_sequence<TSpace::Dimensions>> struct MakeAt;
template<class TSpace, std::size_t Index, std::size_t... Is>
struct MakeAt<TSpace, Index, std::index_sequence<Is...>> {
    using Type = At<TSpace::template GetCoordAtIndex<Is>(Index)...>;
};

template<class TModel, template<class, class> class Cap, class TIdxSeq> struct CapabilityNode;
template<class TModel, template<class, class> class Cap, std::size_t... Is>
struct CapabilityNode<TModel, Cap, std::index_sequence<Is...>> 
    : public Cap<TModel, typename MakeAt<typename CapabilityRoutingTraits<typename Cap<TModel, At<>>::InterfaceType>::SpaceType, Is>::Type>... 
{
    using Intf = typename Cap<TModel, At<>>::InterfaceType;
    using TSpace = typename CapabilityRoutingTraits<Intf>::SpaceType;
    using ContextT = ContextTypeOf<Intf>;

    void FillArena(std::size_t slot) const {
        auto& arena = RegistrySlot<TensorArena<Intf>>::s_Value;
        std::size_t baseIdx = slot * TSpace::Volume;
        if (arena.size() < baseIdx + TSpace::Volume) arena.resize(baseIdx + TSpace::Volume);

        ([&]() {
            using Impl = Cap<TModel, typename MakeAt<TSpace, Is>::Type>;
            auto& cell = arena[baseIdx + Is];
            const Intf* ptr = static_cast<const Intf*>(static_cast<const Impl*>(this));

            if constexpr (!std::is_same_v<typename Impl::ConfigType, void>) {
                auto tramp = [](const void* o, const ContextT& c) -> bool { return static_cast<const Impl*>(o)->config.Condition(c); };
                cell.dynamicRules.push_back({ ptr, static_cast<const void*>(this), tramp, static_cast<const Impl*>(this)->config.priority });
                std::sort(cell.dynamicRules.begin(), cell.dynamicRules.end(), [](auto& a, auto& b){ return a.priority > b.priority; });
            } else { cell.fallback = ptr; }
        }(), ...);
    }
};

template<class TModel, template<class, class> class... TCap>
struct CapabilityBaker : public IBakerNode {
    template<template<class, class> class C> 
    static constexpr std::size_t VolOf = CapabilityRoutingTraits<typename C<TModel, At<>>::InterfaceType>::SpaceType::Volume;

    struct Unit : public CapabilityNode<TModel, TCap, std::make_index_sequence<VolOf<TCap>>>... {
        void Fill(std::size_t s) const { (CapabilityNode<TModel, TCap, std::make_index_sequence<VolOf<TCap>>>::FillArena(s), ...); }
    } m_unit;

    void Bake() const override {
        ModelTypeID id = TypeIDOf<TModel>::Get();
        auto& map = RegistrySlot<std::unordered_map<ModelTypeID, std::size_t>>::s_Value;
        if (map.find(id) == map.end()) map[id] = map.size();
        m_unit.Fill(map[id]);
    }
};

template<class... Models, template<class, class> class... TCap>
struct CapabilityBaker<TypeList<Models...>, TCap...> : public CapabilityBaker<Models, TCap...>... {
    void Bake() const override {
        (CapabilityBaker<Models, TCap...>::Bake(), ...);
    }
};


// =============================================================================
// 6. ROUTER (API UNIFIÉE)
// =============================================================================

class CapabilityRouter {
public: // Made public to allow ModelRouter synchronization[cite: 6]
    static void EnsureBaked() {
        struct StaticGuard { StaticGuard() { CapabilityRouter::Bake(); } };
        static StaticGuard s_Guard;
    } 
    static void Bake() { for (auto* b = RegistrySlot<const IBakerNode*>::s_Value; b; b = b->m_Next) b->Bake(); }

    template<class InterfaceT, typename... TArgs>
    static const InterfaceT* Find(ModelTypeID modelID, const TArgs&... args) {
        EnsureBaked();

        auto& map = RegistrySlot<std::unordered_map<ModelTypeID, std::size_t>>::s_Value;
        auto it = map.find(modelID); if (it == map.end()) return nullptr;

        using TTraits = CapabilityRoutingTraits<InterfaceT>;
        using TSpace = typename TTraits::SpaceType;
        using TFullCtx = ContextTypeOf<InterfaceT>;

        std::size_t idx = (it->second * TSpace::Volume) + TSpace::ComputeOffset(args...);
        const auto& cell = RegistrySlot<TensorArena<InterfaceT>>::s_Value[idx];

        // MVP Fix: Explicit assignment prevents Most Vexing Parse[cite: 6]
        TFullCtx ctx = TFullCtx(args...); 
        for (const auto& rule : cell.dynamicRules) if (rule.Matches(ctx)) return rule.implementation;
        
        return cell.fallback;
    }
};

// =============================================================================
// 7. THE MODEL ROUTER (STATIC PROXY / COLD PATH)
// =============================================================================

template<class ModelT>
class ModelRouter {
public:
    // Zero-cost static wrapper over CapabilityRouter::Find
    template<class InterfaceT, typename... TArgs>
    static const InterfaceT* Find(const TArgs&... args) {
        CapabilityRouter::EnsureBaked(); // Order synchronization fix[cite: 6]
        return CapabilityRouter::Find<InterfaceT>(TypeIDOf<ModelT>::Get(), args...);
    }
};

// =============================================================================
// 8. GPP POV
// =============================================================================

struct IUnitAI { virtual void Execute() const = 0; };

template<> struct CapabilityRoutingTraits<IUnitAI> {
    using SpaceType = CapabilitySpace<WorldState, AlertState>;
    using FullContext = RuleContext<WorldState, AlertState, EnergyContext>;
};

struct EmergencyConfig {
    int priority = 100;
    bool Condition(const typename CapabilityRoutingTraits<IUnitAI>::FullContext& ctx) const {
        return ctx.Get<EnergyContext>().battery < 20.0f;
    }
};

template<class T, class TAt> struct NormalTask : Capability<IUnitAI> {
    void Execute() const override { std::cout << "Action: Standard Operative.\n"; }
};

template<class T, class TAt> struct EmergencyTask : Capability<IUnitAI, EmergencyConfig> {
    void Execute() const override { std::cout << "Action: !!! EMERGENCY RECHARGE !!!\n"; }
};

struct Scout {};
static const CapabilityBaker<Scout, NormalTask, EmergencyTask> g_ScoutBaker{};

// =============================================================================
// 9. MAIN
// =============================================================================

int main() {
    auto id = TypeIDOf<Scout>::Get();

    std::cout << "--- CRG STAGE 10: DOD FLAT TENSOR ---\n\n";

    std::cout << "[ ECS PATH: Via CapabilityRouter directly ]\n";
    // Cas 1 : Batterie à 80% (Broadphase O(1) -> Fallback)
    {
        EnergyContext ctx{ 80.0f };
        std::cout << "Day/Calm, 80% Batt -> ";
        if (auto* ai = CapabilityRouter::Find<IUnitAI>(id, WorldState::Day, AlertState::Calm, ctx)) ai->Execute();
    }

    // Cas 2 : Batterie à 15% (Narrowphase O(K) -> Règle prioritaire)
    {
        EnergyContext ctx{ 15.0f };
        std::cout << "Day/Calm, 15% Batt -> ";
        if (auto* ai = CapabilityRouter::Find<IUnitAI>(id, WorldState::Day, AlertState::Calm, ctx)) ai->Execute();
    }
    
    std::cout << "\n[ COLD PATH: Via Strongly-Typed ModelRouter ]\n";
    // Cas 3 : Utilisation du ModelRouter pour la clarté (Cold Path)
    {
        EnergyContext ctx{ 5.0f };
        std::cout << "Night/Combat, 5% Batt -> ";
        if (auto* ai = ModelRouter<Scout>::Find<IUnitAI>(WorldState::Night, AlertState::Combat, ctx)) ai->Execute();
    }

    return 0;
}