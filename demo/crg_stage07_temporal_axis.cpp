// Copyright (c) 2026 Cyril Tissier. All rights reserved.
// Licensed under the Apache License, Version 2.0.
//
// =============================================================================
// CAPABILITY ROUTING GATEWAY (CRG) - STAGE 7: TEMPORAL AXIS (DECOUPLED)
// =============================================================================
//
// @intent:
// Introduce contextual resolution (1D) with ABSOLUTE decoupling.
// Interfaces are pure and have no knowledge of the variation Axis.
// The link between Interface and Axis is managed by external CapabilityRoutingTraits.
// Specialization utilizes native C++ template priority.
// =============================================================================

#include <iostream>
#include <typeinfo>
#include <vector>
#include <tuple>

// =============================================================================
// 1. INFRASTRUCTURE (DLL SAFE)
// =============================================================================

#ifndef CRG_DLL_ENABLED
#define CRG_DLL_ENABLED 1
#endif

template<class T>
struct UniversalAnchor {
#if !CRG_DLL_ENABLED
    static T& Get() {
        static T s_Value{};
        return s_Value;
    }
#else
    static T& Get();
#endif
};

#if CRG_DLL_ENABLED
    #define CRG_DEFINE_UNIVERSAL_ANCHOR(T) \
        template<> T& UniversalAnchor<T>::Get() { \
            static T s_Value{}; \
            return s_Value; \
        }
#else
    #define CRG_DEFINE_UNIVERSAL_ANCHOR(T) 
#endif

template<class TNode> using NodeListAnchor = UniversalAnchor<const TNode*>;

template<class TNode, class TInterface>
struct NodeList : public TInterface {
    const TNode* m_Next = nullptr;
    NodeList() {
        const TNode* derivedThis = static_cast<const TNode*>(this);
        m_Next = NodeListAnchor<TNode>::Get();
        NodeListAnchor<TNode>::Get() = derivedThis;
    }
};

// =============================================================================
// 2. DIMENSIONS & COORDINATES (DECOUPLING)
// =============================================================================

template<typename T> struct EnumTraits;

// Global state for 0D capabilities (no axis)
struct GlobalState { constexpr operator std::size_t() const { return 0; } };
template<> struct EnumTraits<GlobalState> { static constexpr std::size_t Count = 1; };

// --- EXTERNAL ROUTING TRAITS ---
// Defines the axis of an interface without modifying it (Zero coupling)
template<class TInterface> 
struct CapabilityRoutingTraits {
    using AxisType = GlobalState; 
};

// Coordinate selector for specialization
template<auto... Values> struct At {};

// Conversion helpers: Index -> At coordinate
template<class AxisType, std::size_t I> struct AxisToAt { using Type = At<static_cast<AxisType>(I)>; };
template<std::size_t I> struct AxisToAt<GlobalState, I> { using Type = At<>; };

// =============================================================================
// 3. CORE TYPES
// =============================================================================

using ModelTypeID = std::size_t;
using InterfaceTypeID = std::size_t; 
template<class T> struct TypeIDOf { static std::size_t Get() { return typeid(T).hash_code(); } };
template<typename... Ts> struct TypeList {};

// Capability Helper: Injects InterfaceType alias for the Binding
template<class TInterface> 
struct Capability : public TInterface { 
    using InterfaceType = TInterface; 
};

struct IRegistryNode {
    virtual ~IRegistryNode() = default;
    virtual ModelTypeID GetTargetModelID() const = 0;
    virtual const void* ResolveSpanRaw(InterfaceTypeID interfaceID) const = 0;
};

using RegistryVector = std::vector<const IRegistryNode*>;
using RouterSlot = UniversalAnchor<RegistryVector>;
CRG_DEFINE_UNIVERSAL_ANCHOR(RegistryVector) 

struct IAssembler { virtual void Assemble(RegistryVector& registry) const = 0; };
struct IBindingNode : public NodeList<IBindingNode, IAssembler> {};
CRG_DEFINE_UNIVERSAL_ANCHOR(const IBindingNode*)

// =============================================================================
// 4. THE BAKER (SPATIO-TEMPORAL COMPILER)
// =============================================================================

template<class TModel, template<class, class> class Cap, class IdxSeq>
struct CapabilityNode;

template<class TModel, template<class, class> class Cap, std::size_t... Is>
struct CapabilityNode<TModel, Cap, std::index_sequence<Is...>> 
    // Resolves axis via external CapabilityRoutingTraits and injects the correct At coordinate
    : public Cap<TModel, typename AxisToAt<typename CapabilityRoutingTraits<typename Cap<TModel, At<>>::InterfaceType>::AxisType, Is>::Type>... 
{
    using Intf = typename Cap<TModel, At<>>::InterfaceType;
    using Axis = typename CapabilityRoutingTraits<Intf>::AxisType;
    
    static constexpr std::size_t Size = sizeof...(Is);
    const Intf* m_buffer[Size];

    CapabilityNode() {
        const Intf* ptrs[] = { static_cast<const Intf*>(static_cast<const Cap<TModel, typename AxisToAt<Axis, Is>::Type>*>(this))... };
        for(std::size_t i=0; i<Size; ++i) m_buffer[i] = ptrs[i];
    }
    
    const void* GetSpanRaw() const { return m_buffer; }
};

template<class TModel, template<class, class> class... TCapabilities>
class CapabilitySpace : public IRegistryNode, 
    public CapabilityNode<TModel, TCapabilities, std::make_index_sequence<EnumTraits<typename CapabilityRoutingTraits<typename TCapabilities<TModel, At<>>::InterfaceType>::AxisType>::Count>>... 
{
public:
    ModelTypeID GetTargetModelID() const override { return TypeIDOf<TModel>::Get(); }
    
    const void* ResolveSpanRaw(InterfaceTypeID interfaceID) const override {
        const void* result = nullptr;
        (void)((interfaceID == TypeIDOf<typename TCapabilities<TModel, At<>>::InterfaceType>::Get() && 
         (result = static_cast<const CapabilityNode<TModel, TCapabilities, std::make_index_sequence<EnumTraits<typename CapabilityRoutingTraits<typename TCapabilities<TModel, At<>>::InterfaceType>::AxisType>::Count>>*>(this)->GetSpanRaw())), ...);
        return result;
    }
};

template<class TModel, template<class, class> class... TCapabilities>
struct CapabilityBinding : public IBindingNode {
    CapabilitySpace<TModel, TCapabilities...> m_unit;
    void Assemble(RegistryVector& registry) const override { registry.push_back(&m_unit); }
};

template<class... Models, template<class, class> class... TCapabilities>
struct CapabilityBinding<TypeList<Models...>, TCapabilities...> : public CapabilityBinding<Models, TCapabilities...>... {
    void Assemble(RegistryVector& registry) const override {
        (CapabilityBinding<Models, TCapabilities...>::Assemble(registry), ...);
    }
};

// =============================================================================
// 5. THE ROUTER
// =============================================================================

class CapabilityRouter {
private:
    static void EnsureBaked() {
        struct StaticGuard { StaticGuard() { CapabilityRouter::Bake(); } };
        static StaticGuard s_Guard;
    }
public:
    static void Bake() {
        auto& registry = RouterSlot::Get();
        registry.clear();
        for (auto* b = NodeListAnchor<IBindingNode>::Get(); b; b = b->m_Next) {
            b->Assemble(registry);
        }
    }

    template<class InterfaceT>
    static const InterfaceT* Find(ModelTypeID modelID, typename CapabilityRoutingTraits<InterfaceT>::AxisType coord = {}) {
        EnsureBaked();
        for (const auto* node : RouterSlot::Get()) {
            if (node->GetTargetModelID() == modelID) {
                if (const void* ptr = node->ResolveSpanRaw(TypeIDOf<InterfaceT>::Get())) {
                    return static_cast<const InterfaceT* const*>(ptr)[static_cast<std::size_t>(coord)];
                }
            }
        }
        return nullptr;
    }
};

// =============================================================================
// 6. DEMO (PURE OOP - ZERO COUPLING)
// =============================================================================

// --- A. CONTRACTS ---
struct IDiagnostic { virtual void Run() const = 0; };
struct ITelemetry  { virtual void Ping() const = 0; };

// --- B. AXES ---
enum class WorldState { Day, Night };
template<> struct EnumTraits<WorldState> { static constexpr std::size_t Count = 2; };

// --- C. ROUTING TRAITS (External Mapping) ---
template<> struct CapabilityRoutingTraits<ITelemetry> { using AxisType = WorldState; };

// --- D. LOGIC ---

// 0D : Diagnostic (Matches At<> by default)
template<class T, class TAt = At<>> 
struct DiagLogic : Capability<IDiagnostic> { 
    void Run() const override { std::cout << "[Diag] System check OK.\n"; } 
};

// 1D : Specialized Telemetry
template<class T, class TAt = At<>> struct TeleLogic : Capability<ITelemetry> {}; 

template<class T>
struct TeleLogic<T, At<WorldState::Day>> : Capability<ITelemetry> {
    void Ping() const override { std::cout << "[Tele] Day: Standard Radio Ping.\n"; }
};

template<class T>
struct TeleLogic<T, At<WorldState::Night>> : Capability<ITelemetry> {
    void Ping() const override { std::cout << "[Tele] Night: Encrypted Burst.\n"; }
};

// --- E. REGISTRATION ---
struct Drone {};
static const CapabilityBinding<Drone, DiagLogic, TeleLogic> g_AirBinding{};

int main() {
    std::cout << "--- CRG STAGE 7: TEMPORAL AXIS (ROUTING TRAITS) ---\n\n";

    auto id = TypeIDOf<Drone>::Get();

    std::cout << "--- [ ENGINE TIME: DAY ] ---\n";
    CapabilityRouter::Find<IDiagnostic>(id)->Run();
    CapabilityRouter::Find<ITelemetry>(id, WorldState::Day)->Ping();

    std::cout << "\n--- [ ENGINE TIME: NIGHT ] ---\n";
    CapabilityRouter::Find<IDiagnostic>(id)->Run();
    CapabilityRouter::Find<ITelemetry>(id, WorldState::Night)->Ping();

    return 0;
}