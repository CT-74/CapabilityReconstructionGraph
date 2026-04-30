// Copyright (c) 2026 Cyril Tissier. All rights reserved.
// Licensed under the Apache License, Version 2.0.
//
// =============================================================================
// CAPABILITY ROUTING GATEWAY (CRG) - STAGE 11: STATELESS DOD (ECS READY)
// =============================================================================
//
// @intent:
// High-performance logic routing via pre-resolved Dense Handles.
// - StaticGuard: Restored Lazy Initialization (Bakes on first Find).
// - ModelHandle: ECS-friendly component storing a physical dense index.
// - ModelBinding: Stateless transport using static function pointers.
// - ModelRouter: Zero-cost static proxy for strongly-typed Cold Path routing.
// =============================================================================

#include <iostream>
#include <vector>
#include <tuple>
#include <typeinfo>
#include <algorithm>
#include <unordered_map>
#include <type_traits>

// =============================================================================
// 1. INFRASTRUCTURE & DLL SAFETY
// =============================================================================

#ifndef CRG_DLL_ENABLED
#define CRG_DLL_ENABLED 0
#endif

// RegistrySlot: Ensures unique global state across module boundaries.
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

// NodeList: Static chaining for automatic Baker discovery.
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
// 2. IDENTITY & DENSE INDEXING
// =============================================================================

// Physical dense index for O(1) Tensor access.
struct DenseModelID {
    std::size_t index;
    explicit DenseModelID(std::size_t i) : index(i) {}
    static constexpr std::size_t Invalid = static_cast<std::size_t>(-1);
    bool IsValid() const { return index != Invalid; }
    operator std::size_t() const { return index; }
};

using ModelMap = std::unordered_map<ModelTypeID, std::size_t>; 
CRG_BIND_SLOT(ModelMap)

// ModelHandle: Resolved once, stored in ECS components for hot-path lookup.
struct ModelHandle {
    DenseModelID denseID;
    explicit ModelHandle(ModelTypeID hash) : denseID(DenseModelID::Invalid) {
        auto& map = RegistrySlot<ModelMap>::s_Value;
        auto it = map.find(hash);
        if (it != map.end()) denseID = DenseModelID(it->second);
    }
    template<class T> static ModelHandle FromType() { return ModelHandle(TypeIDOf<T>::Get()); }
};

// =============================================================================
// 3. TENSOR MATH (HORNER'S METHOD)
// =============================================================================

template<typename T> struct EnumTraits;

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
// 4. CORE ROUTING TYPES (STATELESS)
// =============================================================================

template<class TInterface> struct CapabilityRoutingTraits;
template<auto... Values> struct At {};

// Patch: Fallback context for contracts without dynamic rules[cite: 13].
struct NullContext {
    NullContext() = default;
    template<typename... Args> NullContext(const Args&...) {} 
};

// Patch: Lazy Context Selector (Fixes 'no type named FullContext' error)[cite: 13].
template<typename T, typename = void> struct ContextSelector { using Type = NullContext; };
template<typename T> struct ContextSelector<T, std::void_t<typename CapabilityRoutingTraits<T>::FullContext>> {
    using Type = typename CapabilityRoutingTraits<T>::FullContext;
};
template<typename T> using ContextTypeOf = typename ContextSelector<T>::Type;

// ModelBinding: Stateless container for static logic pointers.
template<class InterfaceT> struct ModelBinding : public InterfaceT {};

template<class InterfaceT>
struct Rule {
    using ContextT = ContextTypeOf<InterfaceT>; // Use patched lazy selector[cite: 13].
    using PredicatePtr = bool (*)(const void*, const ContextT&);

    ModelBinding<InterfaceT> binding; 
    const void*              configData;
    PredicatePtr             predicate;
    int                      priority;

    bool Matches(const ContextT& ctx) const { return !predicate || predicate(configData, ctx); }
};

template<class InterfaceT>
struct DispatchCell {
    std::vector<Rule<InterfaceT>> dynamicRules;      
    ModelBinding<InterfaceT>      fallback;
    bool                          hasFallback = false;
};

template<class InterfaceT> using TensorArena = RegistrySlot<std::vector<DispatchCell<InterfaceT>>>; 

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
// 5. BAKER (TENSOR COMPILER)
// =============================================================================

struct IAssembler { virtual void Bake() const = 0; };
struct IBakerNode : public NodeList<IBakerNode, IAssembler> {};
CRG_BIND_SLOT(const IBakerNode*)

template<class TSpace, std::size_t Index, class IdxSeq = std::make_index_sequence<TSpace::Dimensions>> struct MakeAt;
template<class TSpace, std::size_t Index, std::size_t... DimIs>
struct MakeAt<TSpace, Index, std::index_sequence<DimIs...>> {
    using Type = At<TSpace::template GetCoordAtIndex<DimIs>(Index)...>;
};

template<class TModel, template<class, class> class Cap, class TIdxSeq> struct CapabilityNode;

template<class TModel, template<class, class> class Cap, std::size_t... Is>
struct CapabilityNode<TModel, Cap, std::integer_sequence<std::size_t, Is...>> 
    : public Cap<TModel, typename MakeAt<typename CapabilityRoutingTraits<typename Cap<TModel, At<>>::InterfaceType>::SpaceType, Is>::Type>... 
{
    using Intf = typename Cap<TModel, At<>>::InterfaceType;
    using TSpace = typename CapabilityRoutingTraits<Intf>::SpaceType;
    using ContextT = ContextTypeOf<Intf>; // Use patched lazy selector[cite: 13].

    void FillArena(DenseModelID denseID) const {
        auto& arena = TensorArena<Intf>::s_Value;
        std::size_t baseIdx = denseID.index * TSpace::Volume;
        if (arena.size() < baseIdx + TSpace::Volume) arena.resize(baseIdx + TSpace::Volume);

        ([&]() {
            using Impl = Cap<TModel, typename MakeAt<TSpace, Is>::Type>;
            auto& cell = arena[baseIdx + Is];

            if constexpr (!std::is_same_v<typename Impl::ConfigType, void>) {
                auto tramp = [](const void* o, const ContextT& c) -> bool { return static_cast<const Impl*>(o)->config.Condition(c); };
                ModelBinding<Intf> b;
                b.pfnExecute = &Impl::Execute; // Stateless bind.
                cell.dynamicRules.push_back(Rule<Intf>{ b, &static_cast<const Impl*>(this)->config, tramp, static_cast<const Impl*>(this)->config.priority });
                std::sort(cell.dynamicRules.begin(), cell.dynamicRules.end(), [](auto& a, auto& b){ return a.priority > b.priority; });
            } else {
                cell.fallback.pfnExecute = &Impl::Execute;
                cell.hasFallback = true;
            }
        }(), ...);
    }
};

template<class TModel, template<class, class> class... TCap>
struct CapabilityBaker : public IBakerNode {
    template<template<class, class> class C>
    static constexpr std::size_t VolOf = CapabilityRoutingTraits<typename C<TModel, At<>>::InterfaceType>::SpaceType::Volume;

    struct Unit : public CapabilityNode<TModel, TCap, std::make_index_sequence<VolOf<TCap>>>... {
        void Fill(DenseModelID slot) const { (CapabilityNode<TModel, TCap, std::make_index_sequence<VolOf<TCap>>>::FillArena(slot), ...); }
    } m_unit;

    void Bake() const override {
        ModelTypeID hash = TypeIDOf<TModel>::Get();
        auto& map = RegistrySlot<ModelMap>::s_Value;
        if (map.find(hash) == map.end()) map[hash] = map.size();
        m_unit.Fill(DenseModelID(map[hash]));
    }
};

template<class... Models, template<class, class> class... TCap>
struct CapabilityBaker<TypeList<Models...>, TCap...> : public CapabilityBaker<Models, TCap...>... {
    void Bake() const override {
        (CapabilityBaker<Models, TCap...>::Bake(), ...);
    }
};

// =============================================================================
// 6. CAPABILITY ROUTER (O(1) RUNTIME + LAZY INIT)
// =============================================================================

class CapabilityRouter {
public:
    // Made public so ModelRouter can guarantee evaluation order[cite: 13].
    static void EnsureBaked() {
        struct StaticGuard { StaticGuard() { CapabilityRouter::Bake(); } };
        static StaticGuard s_Guard;
    } 

    static void Bake() { for (auto* b = RegistrySlot<const IBakerNode*>::s_Value; b; b = b->m_Next) b->Bake(); }

    template<class InterfaceT, typename... TArgs>
    static const ModelBinding<InterfaceT>* Find(ModelHandle handle, const TArgs&... args) {
        EnsureBaked();
        if (!handle.denseID.IsValid()) return nullptr;

        const auto& arena = TensorArena<InterfaceT>::s_Value; 
        using TTraits = CapabilityRoutingTraits<InterfaceT>;
        using TSpace = typename TTraits::SpaceType;
        using TFullCtx = ContextTypeOf<InterfaceT>; // Use patched lazy selector[cite: 13].
        
        std::size_t idx = (handle.denseID.index * TSpace::Volume) + TSpace::ComputeOffset(args...);
        if (idx >= arena.size()) return nullptr;
        const auto& cell = arena[idx];

        // Patch: MVP Fix - explicit assignment prevents function declaration ambiguity[cite: 13].
        TFullCtx ctx = TFullCtx(args...); 
        for (const auto& rule : cell.dynamicRules) if (rule.Matches(ctx)) return &rule.binding;
        return cell.hasFallback ? &cell.fallback : nullptr;
    }
};

// =============================================================================
// 7. THE MODEL ROUTER (STATIC PROXY / COLD PATH)
// =============================================================================

template<class ModelT>
class ModelRouter {
public:
    // Zero-cost static wrapper over CapabilityRouter::Find[cite: 13].
    template<class InterfaceT, typename... TArgs>
    static const ModelBinding<InterfaceT>* Find(const TArgs&... args) {
        // Patch: Synchronization for evaluation order[cite: 13].
        CapabilityRouter::EnsureBaked(); 
        return CapabilityRouter::Find<InterfaceT>(ModelHandle::FromType<ModelT>(), args...);
    }
};

// =============================================================================
// 8. GPP IMPLEMENTATION[cite: 13]
// =============================================================================

enum class OpState { Normal, Alert };
template<> struct EnumTraits<OpState> { static constexpr std::size_t Count = 2; };

struct EnergyContext { float battery; };

struct ITask {
    struct Params { int battery; };
    struct RuleContext { 
        std::tuple<const OpState&, const EnergyContext&> values;
        RuleContext(const OpState& s, const EnergyContext& e) : values(std::tie(s, e)) {}
        template<typename T> const T& Get() const { return std::get<const T&>(values); }
    };

    void (*pfnExecute)(Params&) = nullptr; 

    void Execute(Params& p) const { if (pfnExecute) pfnExecute(p); }
};

template<> struct CapabilityRoutingTraits<ITask> { 
    using SpaceType = CapabilitySpace<OpState>; 
    using FullContext = ITask::RuleContext; 
};

struct EmergencyConfig {
    int priority = 100;
    bool Condition(const ITask::RuleContext& ctx) const { return ctx.Get<EnergyContext>().battery < 20.0f; }
};

template<class T, class TAt> struct Scan : Capability<ITask> {
    static void Execute(ITask::Params& p) { std::cout << "Action: Standard Scan. Battery: " << --p.battery << std::endl; }
};

template<class T, class TAt> struct Overdrive : Capability<ITask, EmergencyConfig> {
    static void Execute(ITask::Params& p) { std::cout << "Action: !!! OVERDRIVE !!!" << std::endl; }
};

struct Scout {};

static const CapabilityBaker<Scout, Scan, Overdrive> g_ScoutBaker{};

// =============================================================================
// 9. MAIN[cite: 13]
// =============================================================================

int main() {
    std::cout << "--- CRG STAGE 11: STATELESS DOD + MODEL ROUTER ---\n\n";
    
    ITask::Params droneParams { 15 };

    std::cout << "[ ECS PATH: Via ModelHandle (Hot Path) ]\n";
    // EnsureBaked triggered implicitly inside Find or explicitly if preferred.
    // We call EnsureBaked explicitly here to guarantee FromType gets a valid ID.
    CapabilityRouter::EnsureBaked();
    ModelHandle handle = ModelHandle::FromType<Scout>();

    EnergyContext ctxEmergency { 15.0f }; // Triggers Overdrive.
    std::cout << "OpState::Normal, 15% Batt -> ";
    if (const auto* binding = CapabilityRouter::Find<ITask>(handle, OpState::Normal, ctxEmergency)) {
        binding->Execute(droneParams);
    }
    
    std::cout << "\n[ COLD PATH: Via Strongly-Typed ModelRouter ]\n";
    // ModelRouter handles EnsureBaked internally before creating the handle[cite: 13].
    EnergyContext ctxNormal { 80.0f }; // Triggers Standard Scan
    std::cout << "OpState::Normal, 80% Batt -> ";
    if (const auto* binding = ModelRouter<Scout>::Find<ITask>(OpState::Normal, ctxNormal)) {
        binding->Execute(droneParams);
    }

    return 0;
}