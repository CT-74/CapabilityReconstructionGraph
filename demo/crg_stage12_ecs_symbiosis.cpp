// Copyright (c) 2024-2026 Cyril Tissier. All rights reserved.
// Licensed under the Apache License, Version 2.0.
//
// =============================================================================
// CAPABILITY ROUTING GATEWAY (CRG) - STAGE 12: ECS SYMBIOSIS + MODEL ROUTER
// =============================================================================
// @intent:
// Resolve logic via Tensor (O(1)) and Dynamic Rules (O(K)) for ECS hot-paths.
// - Auto-Extraction: Framework pulls static methods into DODDescriptors.
// - MakeAt: Fixed to correctly resolve N-Dimensional coordinates.
// - ModelRouter: Zero-cost static proxy for cold-path access.
// =============================================================================

#include <iostream>
#include <vector>
#include <tuple>
#include <typeinfo>
#include <algorithm>
#include <unordered_map>
#include <type_traits>

// =============================================================================
// 1. INFRASTRUCTURE & DLL-SAFE
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

using ModelTypeID = std::size_t;
template<class T> struct TypeIDOf { static ModelTypeID Get() { return typeid(T).hash_code(); } };
template<typename... Ts> struct TypeList {};

struct DenseModelID {
    std::size_t index;
    explicit DenseModelID(std::size_t i) : index(i) {}
    static constexpr std::size_t Invalid = static_cast<std::size_t>(-1);
    bool IsValid() const { return index != Invalid; }
    operator std::size_t() const { return index; }
};

using ModelMap = std::unordered_map<ModelTypeID, std::size_t>; 
CRG_BIND_SLOT(ModelMap)

struct ModelHandle {
    DenseModelID denseID;
    explicit ModelHandle(ModelTypeID hash) : denseID(DenseModelID::Invalid) {
        const auto& map = RegistrySlot<ModelMap>::s_Value;
        auto it = map.find(hash);
        if (it != map.end()) denseID = DenseModelID(it->second);
    }
    template<class T> static ModelHandle FromType() { return ModelHandle(TypeIDOf<T>::Get()); }
};

template<class TNode, class TInterface> struct NodeList : public TInterface {
    const TNode* m_Next = nullptr;
    NodeList() {
        m_Next = RegistrySlot<const TNode*>::s_Value;
        RegistrySlot<const TNode*>::s_Value = static_cast<const TNode*>(this);
    }
};

// =============================================================================
// 2. TENSOR MATH (HORNER'S METHOD)
// =============================================================================

template<typename T> struct EnumTraits;

template<class... TAxes>
struct Space {
    using AxisTuple = std::tuple<TAxes...>;
    static constexpr std::size_t Dimensions = sizeof...(TAxes);
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
            return static_cast<AxisT>((index / Space<TAxes...>::template GetStride<DimIdx>()) % EnumTraits<AxisT>::Count);
        }
    }

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

template<class TContract> struct CapabilityRoutingTraits { using SpaceType = Space<>; }; 
template<auto... Values> struct At {};

template<class TSpace, std::size_t Index, class IdxSeq> struct MakeAt;
template<class TSpace, std::size_t Index, std::size_t... DimIs>
struct MakeAt<TSpace, Index, std::index_sequence<DimIs...>> {
    using Type = At<TSpace::template GetCoordAtIndex<DimIs>(Index)...>; 
};

template<class TSpace, std::size_t Index>
using MakeAt_t = typename MakeAt<TSpace, Index, std::make_index_sequence<TSpace::Dimensions>>::Type;

// =============================================================================
// 3. CORE ROUTING TYPES & DOD DESCRIPTOR
// =============================================================================

struct NullContext {
    NullContext() = default;
    template<typename... Args> NullContext(const Args&...) {} 
};

// Lazy Context Selector: Fixes SFINAE substitution errors
template<typename T, typename = void> struct ContextSelector { using Type = NullContext; };
template<typename T> struct ContextSelector<T, std::void_t<typename CapabilityRoutingTraits<T>::RuleContext>> {
    using Type = typename CapabilityRoutingTraits<T>::RuleContext;
};

template<typename T> using ContextTypeOf = typename ContextSelector<T>::Type;

template<typename T, typename = void> struct HasParams : std::false_type {};
template<typename T> struct HasParams<T, std::void_t<typename T::Params>> : std::true_type {};

template<class TContract>
struct DODDescriptor {
    using ParamsT = typename TContract::Params;
    void (*pfnExecute)(ParamsT&); 
    inline void Execute(ParamsT& p) const { if(pfnExecute) pfnExecute(p); } 
};

template<class TContract>
struct Rule {
    using ContextT = ContextTypeOf<TContract>;
    using PredicatePtr = bool (*)(const void*, const ContextT&);

    DODDescriptor<TContract> descriptor; 
    const void*              configData;
    PredicatePtr             predicate;
    int                      priority;

    bool Matches(const ContextT& ctx) const { return !predicate || predicate(configData, ctx); }
};

template<class TContract>
struct DispatchCell {
    std::vector<Rule<TContract>> dynamicRules;      
    DODDescriptor<TContract>     fallback;
    bool                         hasFallback = false;
};

template<class TContract> using TensorArena = RegistrySlot<std::vector<DispatchCell<TContract>>>; 

template<class TContract, class TConfig = void> 
struct Capability { using InterfaceType = TContract; using ConfigType = TConfig; TConfig config; };

template<class TContract> 
struct Capability<TContract, void> { using InterfaceType = TContract; using ConfigType = void; };

template<typename T, typename = void> struct HasConfigType : std::false_type {};
template<typename T> struct HasConfigType<T, std::void_t<typename T::ConfigType>> {
    static constexpr bool value = !std::is_same_v<typename T::ConfigType, void>;
};

// =============================================================================
// 4. THE BAKER (AUTO-EXTRACTION)
// =============================================================================

struct IAssembler { virtual void Bake() const = 0; };
struct IBakerNode : public NodeList<IBakerNode, IAssembler> {};
CRG_BIND_SLOT(const IBakerNode*) 

template<class TModel, template<class, class> class Cap, class TIdxSeq> struct CapabilityNode;

template<class TModel, template<class, class> class Cap, std::size_t... Is>
struct CapabilityNode<TModel, Cap, std::index_sequence<Is...>> 
    : public Cap<TModel, MakeAt_t<typename CapabilityRoutingTraits<typename Cap<TModel, At<>>::InterfaceType>::SpaceType, Is>>... 
{
    using ContractT = typename Cap<TModel, At<>>::InterfaceType;
    using TSpace = typename CapabilityRoutingTraits<ContractT>::SpaceType;
    using ContextT = ContextTypeOf<ContractT>;

    void FillArena(DenseModelID denseID) const {
        auto& arena = TensorArena<ContractT>::s_Value;
        std::size_t baseIdx = denseID.index * TSpace::Volume;
        if (arena.size() < baseIdx + TSpace::Volume) arena.resize(baseIdx + TSpace::Volume);

        ([&]() {
            using Impl = Cap<TModel, MakeAt_t<TSpace, Is>>;
            auto& cell = arena[baseIdx + Is];

            DODDescriptor<ContractT> desc { &Impl::Execute };

            if constexpr (HasConfigType<Impl>::value) {
                auto tramp = [](const void* o, const ContextT& ctx) -> bool { return static_cast<const Impl*>(o)->config.Condition(ctx); };
                cell.dynamicRules.push_back(Rule<ContractT>{ desc, &static_cast<const Impl*>(this)->config, tramp, static_cast<const Impl*>(this)->config.priority });
                std::sort(cell.dynamicRules.begin(), cell.dynamicRules.end(), [](auto& a, auto& b){ return a.priority > b.priority; });
            } else {
                cell.fallback = desc;
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
        void Fill(DenseModelID slot) const { 
            (CapabilityNode<TModel, TCap, std::make_index_sequence<VolOf<TCap>>>::FillArena(slot), ...); 
        }
    } m_unit{}; 

    void Bake() const override {
        ModelTypeID hash = TypeIDOf<TModel>::Get();
        auto& map = RegistrySlot<ModelMap>::s_Value;
        if (map.find(hash) == map.end()) map[hash] = map.size();
        m_unit.Fill(DenseModelID(map[hash]));
    }
};

template<class... Models, template<class, class> class... TCapabilities>
struct CapabilityBaker<TypeList<Models...>, TCapabilities...> : public CapabilityBaker<Models, TCapabilities...>... {
    void Bake() const override { (CapabilityBaker<Models, TCapabilities...>::Bake(), ...); }
};

// =============================================================================
// 5. ROUTER
// =============================================================================

class CapabilityRouter {
public:
    static void EnsureBaked() {
        static struct StaticGuard {
            StaticGuard() {
                for (auto* b = RegistrySlot<const IBakerNode*>::s_Value; b; b = b->m_Next) b->Bake();
            }
        } s_guard;
    } 

    template<class TContract, typename... TArgs>
    static const DODDescriptor<TContract>* Find(ModelHandle handle, const TArgs&... args) {
        EnsureBaked();
        if (!handle.denseID.IsValid()) return nullptr;

        const auto& arena = TensorArena<TContract>::s_Value; 
        using TSpace = typename CapabilityRoutingTraits<TContract>::SpaceType;
        using TFullCtx = ContextTypeOf<TContract>;
        
        std::size_t idx = (handle.denseID.index * TSpace::Volume) + TSpace::ComputeOffset(args...);
        if (idx >= arena.size()) return nullptr;

        const auto& cell = arena[idx];
        TFullCtx ctx = TFullCtx(args...); 

        for (const auto& rule : cell.dynamicRules) if (rule.Matches(ctx)) return &rule.descriptor;
        return cell.hasFallback ? &cell.fallback : nullptr;
    }
};

// =============================================================================
// 6. MODEL ROUTER
// =============================================================================

template<class ModelT>
class ModelRouter {
public:
    template<class TContract, typename... TArgs>
    static const DODDescriptor<TContract>* Find(const TArgs&... args) {
        CapabilityRouter::EnsureBaked(); 
        return CapabilityRouter::Find<TContract>(ModelHandle::FromType<ModelT>(), args...);
    }
};

// =============================================================================
// 7. CONTRACTS & GPP
// =============================================================================

struct EnergyContract {
    struct Params { float& batteryCharge; };
};

struct DiagnosticsContract {
    struct Params { const char* name; };
};

enum class WorldState { Day, Night };
enum class Biome { Desert, Tundra };

template<> struct EnumTraits<WorldState> { static constexpr std::size_t Count = 2; };
template<> struct EnumTraits<Biome> { static constexpr std::size_t Count = 2; };

template<> struct CapabilityRoutingTraits<EnergyContract> { 
    using SpaceType = Space<WorldState, Biome>; 
};

template<class T, class TAt = At<>> 
struct DrainLogic : Capability<EnergyContract> {
    static void Execute(EnergyContract::Params& p) { p.batteryCharge -= 1.0f; }
};

template<class T, auto... R>
struct DrainLogic<T, At<WorldState::Night, Biome::Tundra, R...>> : Capability<EnergyContract> {
    static void Execute(EnergyContract::Params& p) { p.batteryCharge -= 10.0f; }
};

template<class T, class TAt = At<>>
struct DiagLogic : Capability<DiagnosticsContract> {
    static void Execute(DiagnosticsContract::Params& p) { std::cout << "[DIAG] Executing on: " << p.name << "\n"; }
};

struct Scout {};
struct HeavyLifter {};
using DroneFleet = TypeList<Scout, HeavyLifter>;

static const CapabilityBaker<DroneFleet, DrainLogic, DiagLogic> g_DroneBaker{};

// =============================================================================
// 8. ECS LOOP & MAIN
// =============================================================================

struct EnergySystem {
    std::vector<ModelHandle> handles;
    std::vector<float>       batteries;
    std::vector<Biome>       biomes;

    void Update(WorldState currentTime) {
        for (std::size_t i = 0; i < handles.size(); ++i) {
            auto* descriptor = CapabilityRouter::Find<EnergyContract>(handles[i], currentTime, biomes[i]);
            if (descriptor) {
                EnergyContract::Params params { batteries[i] };
                descriptor->Execute(params);
            }
        }
    }
};

int main() {
    CapabilityRouter::EnsureBaked(); 

    EnergySystem ecs;
    ecs.handles.push_back(ModelHandle::FromType<Scout>());
    ecs.batteries.push_back(100.0f);
    ecs.biomes.push_back(Biome::Desert);

    ecs.handles.push_back(ModelHandle::FromType<HeavyLifter>());
    ecs.batteries.push_back(100.0f);
    ecs.biomes.push_back(Biome::Tundra);

    std::cout << "--- CRG STAGE 12: ECS SYMBIOSIS + MODEL ROUTER (FIXED) ---\n\n";

    ecs.Update(WorldState::Night);
    std::cout << "Scout Battery: " << ecs.batteries[0] << "%" << std::endl;
    std::cout << "Heavy Battery: " << ecs.batteries[1] << "%" << std::endl;

    DiagnosticsContract::Params p1{"Scout Alpha"};
    if (auto* d = ModelRouter<Scout>::Find<DiagnosticsContract>()) d->Execute(p1);

    return 0;
}