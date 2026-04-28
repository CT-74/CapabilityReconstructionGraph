// Copyright (c) 2024-2026 Cyril Tissier. All rights reserved.
//
// =============================================================================
// CAPABILITY ROUTING GATEWAY (CRG) - STAGE 6: TEMPORAL AXIS (DECOUPLED)
// =============================================================================
//
// @intent:
// Introduce contextual resolution (1D) with ABSOLUTE decoupling.
// Interfaces are pure and have no knowledge of the variation Axis.
// The link between Interface and Axis is managed by an external Topology.
// Specialization utilizes native C++ template priority.
// =============================================================================

#include <iostream>
#include <typeinfo>
#include <vector>
#include <span>

// =============================================================================
// 1. INFRASTRUCTURE (DLL SAFE)
// =============================================================================

#ifndef CRG_DLL_ENABLED
#define CRG_DLL_ENABLED 1
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

template<class TNode> using NodeListAnchor = RegistrySlot<const TNode*>;

template<class TNode, class TInterface>
struct NodeList : public TInterface {
    const TNode* m_Next = nullptr;
    NodeList() {
        const TNode* derivedThis = static_cast<const TNode*>(this);
        m_Next = NodeListAnchor<TNode>::s_Value;
        NodeListAnchor<TNode>::s_Value = derivedThis;
    }
};

// =============================================================================
// 2. DIMENSIONS & COORDINATES (DECOUPLING)
// =============================================================================

template<typename T> struct EnumTraits;

// Global state for 0D capabilities (no axis)
struct GlobalState { constexpr operator std::size_t() const { return 0; } };
template<> struct EnumTraits<GlobalState> { static constexpr std::size_t Count = 1; };

// --- EXTERNAL TOPOLOGY ---
// Defines the axis of an interface without modifying it (Zero coupling)
template<class TInterface> 
struct CapabilityTopology {
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

// Capability Helper: Injects InterfaceType alias for the Baker
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
using RouterSlot = RegistrySlot<RegistryVector>;
CRG_BIND_SLOT(RegistryVector) 

struct IAssembler { virtual void Assemble(RegistryVector& registry) const = 0; };
struct IBakerNode : public NodeList<IBakerNode, IAssembler> {};
CRG_BIND_SLOT(const IBakerNode*)

// =============================================================================
// 4. THE BAKER (SPATIO-TEMPORAL COMPILER)
// =============================================================================

template<class TModel, template<class, class> class Cap, class IdxSeq>
struct CapabilityNode;

template<class TModel, template<class, class> class Cap, std::size_t... Is>
struct CapabilityNode<TModel, Cap, std::index_sequence<Is...>> 
    // Resolves axis via external Topology and injects the correct At coordinate
    : public Cap<TModel, typename AxisToAt<typename CapabilityTopology<typename Cap<TModel, At<>>::InterfaceType>::AxisType, Is>::Type>... 
{
    using Intf = typename Cap<TModel, At<>>::InterfaceType;
    using Axis = typename CapabilityTopology<Intf>::AxisType;
    
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
    public CapabilityNode<TModel, TCapabilities, std::make_index_sequence<EnumTraits<typename CapabilityTopology<typename TCapabilities<TModel, At<>>::InterfaceType>::AxisType>::Count>>... 
{
public:
    ModelTypeID GetTargetModelID() const override { return TypeIDOf<TModel>::Get(); }
    
    const void* ResolveSpanRaw(InterfaceTypeID interfaceID) const override {
        const void* result = nullptr;
        (void)((interfaceID == TypeIDOf<typename TCapabilities<TModel, At<>>::InterfaceType>::Get() && 
         (result = static_cast<const CapabilityNode<TModel, TCapabilities, std::make_index_sequence<EnumTraits<typename CapabilityTopology<typename TCapabilities<TModel, At<>>::InterfaceType>::AxisType>::Count>>*>(this)->GetSpanRaw())), ...);
        return result;
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
        auto& registry = RouterSlot::s_Value;
        registry.clear();
        for (auto* b = NodeListAnchor<IBakerNode>::s_Value; b; b = b->m_Next) {
            b->Assemble(registry);
        }
    }

    template<class InterfaceT>
    static const InterfaceT* Find(ModelTypeID modelID, typename CapabilityTopology<InterfaceT>::AxisType coord = {}) {
        EnsureBaked();
        for (const auto* node : RouterSlot::s_Value) {
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
// 6. DEMO (PURE DOD - ZERO COUPLING)
// =============================================================================

// --- A. CONTRACTS ---
struct IDiagnostic { virtual void Run() const = 0; };
struct ITelemetry  { virtual void Ping() const = 0; };

// --- B. AXES ---
enum class WorldState { Day, Night };
template<> struct EnumTraits<WorldState> { static constexpr std::size_t Count = 2; };

// --- C. TOPOLOGY (External Mapping) ---
template<> struct CapabilityTopology<ITelemetry> { using AxisType = WorldState; };

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
using AirModels = TypeList<Drone>;
static const CapabilityBaker<AirModels, DiagLogic, TeleLogic> g_AirBaker{};

int main() {
    std::cout << "--- CRG STAGE 6: TEMPORAL AXIS (DECOUPLED) ---\n\n";

    auto id = TypeIDOf<Drone>::Get();

    std::cout << "--- [ ENGINE TIME: DAY ] ---\n";
    CapabilityRouter::Find<IDiagnostic>(id)->Run();
    CapabilityRouter::Find<ITelemetry>(id, WorldState::Day)->Ping();

    std::cout << "\n--- [ ENGINE TIME: NIGHT ] ---\n";
    CapabilityRouter::Find<IDiagnostic>(id)->Run();
    CapabilityRouter::Find<ITelemetry>(id, WorldState::Night)->Ping();

    return 0;
}