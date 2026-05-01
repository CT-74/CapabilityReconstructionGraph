// Copyright (c) 2026 Cyril Tissier. All rights reserved.
// Licensed under the Apache License, Version 2.0.
//
// =============================================================================
// CAPABILITY ROUTING GATEWAY (CRG) - STAGE 8: N-DIMENSIONAL ROUTING (CLEAN)
// =============================================================================
// @intent:
// O(1) N-Dimensional Tensor Routing using Horner's Method.
// - CapabilitySpace: Robust implementation supporting 0D to N-Dimensional spaces.
// - CapabilityRouter: The core high-performance gateway (ECS / Hot path).
// - ModelRouter: A zero-cost static proxy for direct type access (Cold path).
// =============================================================================

#include <iostream>
#include <vector>
#include <tuple>
#include <typeinfo>

// =============================================================================
// 1. INFRASTRUCTURE & DLL SAFETY
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
using InterfaceTypeID = std::size_t;
template<class T> struct TypeIDOf { static std::size_t Get() { return typeid(T).hash_code(); } };
template<typename... Ts> struct TypeList {};

// =============================================================================
// 2. N-DIMENSIONAL SPACE MATH (HORNER)
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
// 3. CORE ROUTING & BAKER ENGINE
// =============================================================================

// Default CapabilitySpace traits fallback to 0D CapabilitySpace<>
template<class TInterface> struct CapabilityRoutingTraits { using SpaceType = CapabilitySpace<>; };
template<auto... Values> struct At {};
template<class TInterface> struct Capability : public TInterface { using InterfaceType = TInterface; };

struct IRegistryNode {
    virtual ~IRegistryNode() = default;
    virtual ModelTypeID GetTargetModelID() const = 0;
    virtual const void* ResolveSpanRaw(InterfaceTypeID interfaceID) const = 0;
};

using RegistryVector = std::vector<const IRegistryNode*>;
CRG_BIND_SLOT(RegistryVector)

struct IAssembler { virtual void Assemble(RegistryVector& registry) const = 0; };
struct IBindingNode : public NodeList<IBindingNode, IAssembler> {};
CRG_BIND_SLOT(const IBindingNode*)

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
    const Intf* m_buffer[TSpace::Volume];

    CapabilityNode() {
        const Intf* ptrs[] = { static_cast<const Intf*>(static_cast<const Cap<TModel, typename MakeAt<TSpace, Is>::Type>*>(this))... };
        for(std::size_t i=0; i < TSpace::Volume; ++i) m_buffer[i] = ptrs[i];
    }
    const void* GetSpanRaw() const { return m_buffer; }
};

template<class TModel, template<class, class> class... TCap>
class CapabilitySpace : public IRegistryNode, 
    public CapabilityNode<TModel, TCap, std::make_index_sequence<CapabilityRoutingTraits<typename TCap<TModel, At<>>::InterfaceType>::SpaceType::Volume>>... 
{
public:
    ModelTypeID GetTargetModelID() const override { return TypeIDOf<TModel>::Get(); }
    const void* ResolveSpanRaw(InterfaceTypeID id) const override {
        const void* res = nullptr;
        (void)((id == TypeIDOf<typename TCap<TModel, At<>>::InterfaceType>::Get() && 
         (res = static_cast<const CapabilityNode<TModel, TCap, std::make_index_sequence<CapabilityRoutingTraits<typename TCap<TModel, At<>>::InterfaceType>::SpaceType::Volume>>*>(this)->GetSpanRaw())), ...);
        return res;
    }
};

template<class TModel, template<class, class> class... TCapabilities>
struct CapabilityBinding : public IBindingNode {
    CapabilitySpace<TModel, TCapabilities...> m_unit;
    void Assemble(RegistryVector& registry) const override { registry.push_back(&m_unit); }
};

template<class... Models, template<class, class> class... TCapabilities>
struct CapabilityBinding<TypeList<Models...>, TCapabilities...> : public CapabilityBinding<Models, TCapabilities...>... {
    void Assemble(RegistryVector& registry) const override { (CapabilityBinding<Models, TCapabilities...>::Assemble(registry), ...); }
};

// =============================================================================
// 4. THE CAPABILITY ROUTER (ECS / HOT PATH)
// =============================================================================

class CapabilityRouter {
private:
    static void EnsureBaked() {
        struct StaticGuard { StaticGuard() { CapabilityRouter::Bake(); } };
        static StaticGuard s_Guard;
    } 
public:
    static void Bake() {
        RegistrySlot<RegistryVector>::s_Value.clear();
        for (auto* b = RegistrySlot<const IBindingNode*>::s_Value; b; b = b->m_Next) {
            b->Assemble(RegistrySlot<RegistryVector>::s_Value);
        }
    }

    template<class InterfaceT, typename... TAxes>
    static const InterfaceT* Find(ModelTypeID modelID, const TAxes&... axes) {
        EnsureBaked();
        using TSpace = typename CapabilityRoutingTraits<InterfaceT>::SpaceType;
        
        static_assert(sizeof...(TAxes) == TSpace::Dimensions, "Invalid number of axes provided for Tensor Lookup.");

        // O(1) mathematical offset resolution
        std::size_t offset = TSpace::ComputeOffset(axes...); 

        for (const auto* node : RegistrySlot<RegistryVector>::s_Value) {
            if (node->GetTargetModelID() == modelID) {
                if (const void* ptr = node->ResolveSpanRaw(TypeIDOf<InterfaceT>::Get())) {
                    return static_cast<const InterfaceT* const*>(ptr)[offset];
                }
            }
        }
        return nullptr;
    }
};

// =============================================================================
// 5. THE MODEL ROUTER (STATIC PROXY / COLD PATH)
// =============================================================================

template<class ModelT>
class ModelRouter {
public:
    // Zero-cost static wrapper over CapabilityRouter::Find
    template<class InterfaceT, typename... TAxes>
    static const InterfaceT* Find(const TAxes&... axes) {
        return CapabilityRouter::Find<InterfaceT>(TypeIDOf<ModelT>::Get(), axes...);
    }
};

// =============================================================================
// 6. GAMEPLAY DOMAIN (GPP)
// =============================================================================

enum class WorldState { Day, Night };
enum class AlertState { Calm, Combat };
enum class Weather    { Clear, Rain };

template<> struct EnumTraits<WorldState> { static constexpr std::size_t Count = 2; };
template<> struct EnumTraits<AlertState> { static constexpr std::size_t Count = 2; };
template<> struct EnumTraits<Weather>    { static constexpr std::size_t Count = 2; };

struct IUnitAI { 
    using InterfaceType = IUnitAI;
    virtual ~IUnitAI() = default;
    virtual void Execute() const = 0; 
};
template<> struct CapabilityRoutingTraits<IUnitAI> { 
    using SpaceType = CapabilitySpace<WorldState, AlertState, Weather>; 
};

// Default Logic
template<class T, class TAt = At<>> 
struct UnitLogic : Capability<IUnitAI> {
    void Execute() const override { std::cout << "[AI] Default Idle.\n"; }
};

// Spec: Combat
template<class T, auto World, auto... Rest>
struct UnitLogic<T, At<World, AlertState::Combat, Rest...>> : Capability<IUnitAI> {
    void Execute() const override { std::cout << "[AI] Logic: Combat Maneuvers.\n"; }
};

// Spec: Rain
template<class T, auto World, auto Alert, auto... Rest>
struct UnitLogic<T, At<World, Alert, Weather::Rain, Rest...>> : Capability<IUnitAI> {
    void Execute() const override { std::cout << "[AI] Logic: Rain Operations.\n"; }
};

// Spec Intersection: Combat + Rain
template<class T, auto World, auto... Rest>
struct UnitLogic<T, At<World, AlertState::Combat, Weather::Rain, Rest...>> : Capability<IUnitAI> {
    void Execute() const override { std::cout << "[AI] Logic: Tactical Combat under Heavy Rain.\n"; }
};

// 0D Capability Fallback (No CapabilitySpace defined)
struct ISimplePing {
    using InterfaceType = ISimplePing;
    virtual ~ISimplePing() = default;
    virtual void Ping() const = 0; 
};

template<class T, class TAt = At<>>
struct PingLogic : Capability<ISimplePing> {
    void Ping() const override { std::cout << "[Ping] Pong!\n"; }
};

// =============================================================================
// 7. MAIN
// =============================================================================

struct ScoutDrone {};
static const CapabilityBinding<ScoutDrone, UnitLogic, PingLogic> g_DroneBinding{};

int main() {
    std::cout << "--- CRG STAGE 8: N-DIMENSIONAL TENSOR ROUTING (CLEAN) ---\n\n";

    ModelTypeID scoutID = TypeIDOf<ScoutDrone>::Get();

    std::cout << "[ ECS PATH: Via CapabilityRouter directly (Requires ModelTypeID) ]\n";
    
    std::cout << "Night + Combat + Rain -> ";
    if (auto* ai = CapabilityRouter::Find<IUnitAI>(scoutID, WorldState::Night, AlertState::Combat, Weather::Rain)) ai->Execute();

    std::cout << "Day + Calm + Rain     -> ";
    if (auto* ai = CapabilityRouter::Find<IUnitAI>(scoutID, WorldState::Day, AlertState::Calm, Weather::Rain)) ai->Execute();


    std::cout << "\n[ SCRIPT/COLD PATH: Via ModelRouter (Strongly Typed Proxy) ]\n";

    std::cout << "Day + Combat + Clear  -> ";
    if (auto* ai = ModelRouter<ScoutDrone>::Find<IUnitAI>(WorldState::Day, AlertState::Combat, Weather::Clear)) ai->Execute();

    std::cout << "Night + Calm + Clear  -> ";
    if (auto* ai = ModelRouter<ScoutDrone>::Find<IUnitAI>(WorldState::Night, AlertState::Calm, Weather::Clear)) ai->Execute();


    std::cout << "\n[ 0D FALLBACK PATH: No Tensor Axes ]\n";
    std::cout << "Triggering 0D Logic   -> ";
    if (auto* p = ModelRouter<ScoutDrone>::Find<ISimplePing>()) p->Ping();

    return 0;
}