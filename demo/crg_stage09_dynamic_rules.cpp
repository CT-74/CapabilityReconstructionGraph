// Copyright (c) 2024-2026 Cyril Tissier. All rights reserved.
// Licensed under the Apache License, Version 2.0.
//
// =============================================================================
// CAPABILITY ROUTING GATEWAY (CRG) - STAGE 9: DYNAMIC RULES + MODEL ROUTER
// =============================================================================
// @intent: 
// Resolve overlapping behaviors using prioritized runtime predicates.
// - Broadphase: O(1) N-Dimensional Tensor Lookup.
// - Narrowphase: O(K) linear scan of rules inside DispatchCells.
// - ModelRouter: Zero-cost static proxy for strongly-typed context routing.
// =============================================================================

#include <iostream>
#include <vector>
#include <tuple>
#include <typeinfo>
#include <algorithm>
#include <type_traits>

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
template<class T> struct TypeIDOf { static ModelTypeID Get() { return typeid(T).hash_code(); } };
template<typename... Ts> struct TypeList {};

// =============================================================================
// 2. GAMEPLAY DOMAIN & CONTEXT
// =============================================================================

enum class WorldState { Day, Night };
enum class AlertState { Calm, Combat };
enum class Weather    { Clear, Rain };

template<typename T> struct EnumTraits;
template<> struct EnumTraits<WorldState> { static constexpr std::size_t Count = 2; };
template<> struct EnumTraits<AlertState> { static constexpr std::size_t Count = 2; };
template<> struct EnumTraits<Weather>    { static constexpr std::size_t Count = 2; };

struct EnergyContext { float battery; };

// RuleContext : Zero-copy via std::tie for the variadic API.
template<typename... TArgs>
struct RuleContext {
    std::tuple<const TArgs&...> values;
    RuleContext(const TArgs&... args) : values(std::tie(args...)) {}
    template<typename T> constexpr const T& Get() const { return std::get<const T&>(values); }
};

// Patch: Fallback context for contracts without dynamic rules[cite: 11]
struct NullContext {
    NullContext() = default;
    template<typename... Args> NullContext(const Args&...) {} 
};

// =============================================================================
// 3. TENSOR MATHEMATICS (HORNER)
// =============================================================================

template<class... TAxes>
struct Space {
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
            return static_cast<AxisT>((index / Space<TAxes...>::template GetStride<DimIdx>()) % EnumTraits<AxisT>::Count);
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
// 4. ROUTING TRAITS & RULE DEFINITION
// =============================================================================

template<class TInterface> struct CapabilityRoutingTraits;

// Patch: Lazy Context Selector to handle optional FullContext definition[cite: 11]
template<typename T, typename = void> struct ContextSelector { using Type = NullContext; };
template<typename T> struct ContextSelector<T, std::void_t<typename CapabilityRoutingTraits<T>::FullContext>> {
    using Type = typename CapabilityRoutingTraits<T>::FullContext;
};
template<typename T> using ContextTypeOf = typename ContextSelector<T>::Type;

template<class InterfaceT>
struct Rule {
    using ContextT = ContextTypeOf<InterfaceT>; // Switched to lazy selector
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
// 5. BAKER (NODE ASSEMBLY)
// =============================================================================

struct IRegistryNode {
    virtual ~IRegistryNode() = default;
    virtual ModelTypeID GetTargetModelID() const = 0;
    virtual const void* Resolve(std::size_t interfaceID) const = 0;
};

using RegistryVector = std::vector<const IRegistryNode*>;
CRG_BIND_SLOT(RegistryVector)

struct IAssembler { virtual void Assemble(RegistryVector& registry) const = 0; };
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
    DispatchCell<Intf> m_cells[TSpace::Volume];

    CapabilityNode() {
        ([&]() {
            using Impl = Cap<TModel, typename MakeAt<TSpace, Is>::Type>;
            auto& cell = m_cells[Is];
            const Intf* ptr = static_cast<const Intf*>(static_cast<const Impl*>(this));

            if constexpr (!std::is_same_v<typename Impl::ConfigType, void>) {
                auto tramp = [](const void* o, const ContextT& c) -> bool { return static_cast<const Impl*>(o)->config.Condition(c); };
                cell.dynamicRules.push_back(Rule<Intf>{ ptr, static_cast<const void*>(this), tramp, static_cast<const Impl*>(this)->config.priority });
                std::sort(cell.dynamicRules.begin(), cell.dynamicRules.end(), [](auto& a, auto& b) { return a.priority > b.priority; });
            } else { cell.fallback = ptr; }
        }(), ...);
    }
    const void* ResolveData() const { return m_cells; }
};

template<class TModel, template<class, class> class... TCap>
class CapabilitySpace : public IRegistryNode, 
    public CapabilityNode<TModel, TCap, std::make_index_sequence<CapabilityRoutingTraits<typename TCap<TModel, At<>>::InterfaceType>::SpaceType::Volume>>... 
{
public:
    ModelTypeID GetTargetModelID() const override { return TypeIDOf<TModel>::Get(); }
    const void* Resolve(std::size_t id) const override {
        const void* res = nullptr;
        (void)((id == typeid(typename TCap<TModel, At<>>::InterfaceType).hash_code() && 
               (res = static_cast<const CapabilityNode<TModel, TCap, std::make_index_sequence<CapabilityRoutingTraits<typename TCap<TModel, At<>>::InterfaceType>::SpaceType::Volume>>*>(this)->ResolveData())), ...);
        return res;
    }
};

template<class TModel, template<class, class> class... TCapabilities>
struct CapabilityBaker : public IBakerNode {
    CapabilitySpace<TModel, TCapabilities...> m_unit;
    void Assemble(RegistryVector& registry) const override { registry.push_back(&m_unit); }
};

template<class... Models, template<class, class> class... TCapabilities>
struct CapabilityBaker<TypeList<Models...>, TCapabilities...> : public CapabilityBaker<Models, TCapabilities...>... {
    void Assemble(RegistryVector& registry) const override {
        (CapabilityBaker<Models, TCapabilities...>::Assemble(registry), ...);
    }
};

// =============================================================================
// 6. CAPABILITY ROUTER (ECS / HOT PATH)
// =============================================================================

class CapabilityRouter {
public: // Made public to allow ModelRouter synchronization[cite: 11]
    static void EnsureBaked() {
        struct StaticGuard { StaticGuard() { CapabilityRouter::Bake(); } };
        static StaticGuard s_Guard;
    } 
    static void Bake() {
        RegistrySlot<RegistryVector>::s_Value.clear();
        for (auto* b = RegistrySlot<const IBakerNode*>::s_Value; b; b = b->m_Next) b->Assemble(RegistrySlot<RegistryVector>::s_Value);
    }

    template<class InterfaceT, typename... TArgs>
    static const InterfaceT* Find(ModelTypeID modelID, const TArgs&... args) {
        using TTraits = CapabilityRoutingTraits<InterfaceT>;
        using TSpace = typename TTraits::SpaceType;
        using TFullCtx = ContextTypeOf<InterfaceT>;

        EnsureBaked();

        // Broadphase O(1) via variadic Enum extraction
        std::size_t offset = TSpace::ComputeOffset(args...); 

        for (const auto* node : RegistrySlot<RegistryVector>::s_Value) {
            if (node->GetTargetModelID() == modelID) {
                if (const void* data = node->Resolve(typeid(InterfaceT).hash_code())) {
                    const auto& cell = static_cast<const DispatchCell<InterfaceT>*>(data)[offset];
                    
                    // Patch: MVP Fix - explicit assignment prevents function declaration ambiguity[cite: 11]
                    TFullCtx ctx = TFullCtx(args...); 
                    for (const auto& rule : cell.dynamicRules) {
                        if (rule.Matches(ctx)) return rule.implementation;
                    }
                    
                    return cell.fallback;
                }
            }
        }
        return nullptr;
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
        CapabilityRouter::EnsureBaked(); // Patch: Synchronization for evaluation order[cite: 11]
        return CapabilityRouter::Find<InterfaceT>(TypeIDOf<ModelT>::Get(), args...);
    }
};

// =============================================================================
// 8. GPP IMPLEMENTATION
// =============================================================================

struct IUnitAI { virtual void Execute() const = 0; };

template<> struct CapabilityRoutingTraits<IUnitAI> {
    using SpaceType = Space<WorldState, AlertState>;
    using FullContext = RuleContext<WorldState, AlertState, Weather, EnergyContext>;
};

struct EmergencyConfig {
    int priority = 100;
    bool Condition(const typename CapabilityRoutingTraits<IUnitAI>::FullContext& ctx) const {
        return ctx.Get<EnergyContext>().battery < 20.0f;
    }
};

template<class T, class TAt> struct NormalAI : Capability<IUnitAI> {
    void Execute() const override { std::cout << "Action: Standard Operative.\n"; }
};

template<class T, class TAt> struct LowPowerAI : Capability<IUnitAI, EmergencyConfig> {
    void Execute() const override { std::cout << "Action: !!! EMERGENCY RECHARGE !!!\n"; }
};

struct Drone {};
static const CapabilityBaker<Drone, NormalAI, LowPowerAI> g_DroneBaker{};

// =============================================================================
// 9. MAIN
// =============================================================================

int main() {
    CapabilityRouter::Bake();
    auto id = TypeIDOf<Drone>::Get();

    std::cout << "--- CRG STAGE 9: DYNAMIC RULES + MODEL ROUTER ---\n\n";

    // Cas 1 : Batterie OK
    {
        EnergyContext ctx{ 80.0f };
        std::cout << "80% Battery : ";
        if (auto* ai = CapabilityRouter::Find<IUnitAI>(id, WorldState::Day, AlertState::Calm, Weather::Clear, ctx)) {
            ai->Execute();
        }
    }
    {
        EnergyContext ctx{ 15.0f };
        std::cout << "15% Battery : ";
        if (auto* ai = CapabilityRouter::Find<IUnitAI>(id, WorldState::Day, AlertState::Calm, Weather::Clear, ctx)) {
            ai->Execute();
        }
    }

    std::cout << "\n[ COLD PATH: Via Strongly-Typed ModelRouter ]\n";
    {
        EnergyContext ctx{ 90.0f };
        std::cout << "90% Battery : ";
        if (auto* ai = ModelRouter<Drone>::Find<IUnitAI>(WorldState::Night, AlertState::Combat, Weather::Rain, ctx)) {
            ai->Execute();
        }
    }
    {
        EnergyContext ctx{ 5.0f };
        std::cout << " 5% Battery : ";
        if (auto* ai = ModelRouter<Drone>::Find<IUnitAI>(WorldState::Night, AlertState::Combat, Weather::Rain, ctx)) {
            ai->Execute();
        }
    }

    return 0;
}