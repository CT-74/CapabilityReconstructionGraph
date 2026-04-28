// Copyright (c) 2024-2026 Cyril Tissier. All rights reserved.
//
// =============================================================================
// CAPABILITY ROUTING GATEWAY (CRG) - STAGE 8: DYNAMIC RULES (NARROW PHASE)
// =============================================================================
//
// @intent:
// Resolve overlapping behaviors using prioritized runtime predicates.
// - Broad Phase (O(1)): N-D Tensor lookup for the bucket.
// - Narrow Phase (O(K)): Linear scan of rules within that bucket.
// =============================================================================

#include <iostream>
#include <vector>
#include <tuple>
#include <typeinfo>
#include <span>

// =============================================================================
// 1. INFRASTRUCTURE & TOPOLOGY
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

// --- N-D SPACE ---
template<typename T> struct EnumTraits;
struct GlobalState { constexpr operator std::size_t() const { return 0; } };
template<> struct EnumTraits<GlobalState> { static constexpr std::size_t Count = 1; };

template<class... TAxes>
struct Space {
    static constexpr std::size_t Dimensions = sizeof...(TAxes);
    static constexpr std::size_t Volume = (EnumTraits<TAxes>::Count * ... * 1);

    static constexpr std::size_t ComputeOffset(TAxes... coords) {
        std::size_t offset = 0;
        ((offset = offset * EnumTraits<TAxes>::Count + static_cast<std::size_t>(coords)), ...);
        return offset;
    }
};

template<> struct Space<> { static constexpr std::size_t Dimensions = 0; static constexpr std::size_t Volume = 1; };

template<class TInterface> struct CapabilityTopology { using SpaceType = Space<GlobalState>; };
template<auto... Values> struct At {};

template<class TSpace, std::size_t Index, std::size_t... DimIs>
auto MakeAtFromIndex(std::index_sequence<DimIs...>) {
    return At<TSpace::template GetCoordAtIndex<DimIs>(Index)...>{};
}

// =============================================================================
// 2. DISCOVERY TRAITS (CLEAN & NON-INTRUSIVE)
// =============================================================================

// Extract RuleContext from Interface
template<typename T, typename = void>
struct ContextTrait { using Type = GlobalState; };

template<typename T>
struct ContextTrait<T, std::void_t<typename T::RuleContext>> { using Type = typename T::RuleContext; };

// Extract static Priority (compile-time sorting weight)
template<typename T, typename = void>
struct PriorityTrait { static constexpr int Value = 0; };

template<typename T>
struct PriorityTrait<T, std::void_t<decltype(T::Priority)>> { static constexpr int Value = T::Priority; };

// Detect static bool Condition(const Context&)
template<typename T, typename Context, typename = void>
struct ConditionTrait : std::false_type {};

template<typename T, typename Context>
struct ConditionTrait<T, Context, std::void_t<decltype(T::Condition(std::declval<const Context&>()))>> : std::true_type {};

// =============================================================================
// 3. CORE ROUTING TYPES
// =============================================================================

template<class InterfaceT>
struct RouteNode {
    using ContextT = typename ContextTrait<InterfaceT>::Type;
    using PredicatePtr = bool (*)(const ContextT&);

    const InterfaceT* implementation = nullptr;
    PredicatePtr      predicate      = nullptr; 
    int               priority       = 0;
    RouteNode* next           = nullptr;

    bool Matches(const ContextT& ctx) const {
        return !predicate || predicate(ctx);
    }
};

template<class InterfaceT>
struct Bucket {
    RouteNode<InterfaceT>* head     = nullptr;      
    const InterfaceT* fallback = nullptr;  
};

using ModelTypeID = std::size_t;
using InterfaceTypeID = std::size_t; 
template<class T> struct TypeIDOf { static ModelTypeID Get() { return typeid(T).hash_code(); } };
template<typename... Ts> struct TypeList {};

template<class TInterface> 
struct Capability : public TInterface { using InterfaceType = TInterface; };

struct IRegistryNode {
    virtual ~IRegistryNode() = default;
    virtual ModelTypeID GetTargetModelID() const = 0;
    virtual const void* Resolve(InterfaceTypeID interfaceID) const = 0;
};

using RegistryVector = std::vector<const IRegistryNode*>;
using RouterSlot = RegistrySlot<RegistryVector>;
CRG_BIND_SLOT(RegistryVector)

struct IAssembler { virtual void Assemble(RegistryVector& registry) const = 0; };
struct IBakerNode : public NodeList<IBakerNode, IAssembler> {};
CRG_BIND_SLOT(const IBakerNode*)

// =============================================================================
// 4. THE BAKER (RULE COMPILER)
// =============================================================================

template<class TModel, template<class, class> class Cap, std::size_t... Is>
struct CapabilityNode : public Cap<TModel, decltype(MakeAtFromIndex<typename CapabilityTopology<typename Cap<TModel, At<>>::InterfaceType>::SpaceType, Is>(std::make_index_sequence<CapabilityTopology<typename Cap<TModel, At<>>::InterfaceType>::SpaceType::Dimensions>{}))>... 
{
    using Intf = typename Cap<TModel, At<>>::InterfaceType;
    using TSpace = typename CapabilityTopology<Intf>::SpaceType;
    using ContextT = typename ContextTrait<Intf>::Type;
    
    RouteNode<Intf> m_nodes[sizeof...(Is)];
    Bucket<Intf>    m_buckets[TSpace::Volume];

    CapabilityNode() {
        for (std::size_t i = 0; i < TSpace::Volume; ++i) m_buckets[i] = {};

        std::size_t nodeIdx = 0;
        ([&]() {
            using Impl = Cap<TModel, decltype(MakeAtFromIndex<TSpace, Is>(std::make_index_sequence<TSpace::Dimensions>{}))>;
            RouteNode<Intf>& node = m_nodes[nodeIdx++];
            
            node.implementation = static_cast<const Intf*>(static_cast<const Impl*>(this));
            node.priority       = PriorityTrait<Impl>::Value;

            if constexpr (ConditionTrait<Impl, ContextT>::value) {
                node.predicate = &Impl::Condition;
            } else {
                node.predicate = nullptr;
                // If a rule is unconditional, it becomes the cell's default fallback
                m_buckets[Is].fallback = node.implementation;
            }

            // Insertion and sort logic for the Narrow Phase
            auto& b = m_buckets[Is];
            if (!b.head || node.priority > b.head->priority) {
                node.next = b.head;
                b.head = &node;
            } else {
                auto* curr = b.head;
                while (curr->next && curr->next->priority >= node.priority) curr = curr->next;
                node.next = curr->next;
                curr->next = &node;
            }
        }(), ...);
    }
    const void* ResolveData() const { return m_buckets; }
};

template<class TModel, template<class, class> class... TCapabilities>
class CapabilitySpace : public IRegistryNode, 
    public CapabilityNode<TModel, TCapabilities, std::make_index_sequence<CapabilityTopology<typename TCapabilities<TModel, At<>>::InterfaceType>::SpaceType::Volume>>... 
{
public:
    ModelTypeID GetTargetModelID() const override { return TypeIDOf<TModel>::Get(); }
    const void* Resolve(InterfaceTypeID interfaceID) const override {
        const void* result = nullptr;
        (void)((interfaceID == TypeIDOf<typename TCapabilities<TModel, At<>>::InterfaceType>::Get() && 
         (result = static_cast<const CapabilityNode<TModel, TCapabilities, std::make_index_sequence<CapabilityTopology<typename TCapabilities<TModel, At<>>::InterfaceType>::SpaceType::Volume>>*>(this)->ResolveData())), ...);
        return result;
    }
};

template<class TModel, template<class, class> class... TCapabilities>
struct CapabilityBaker : public IBakerNode {
    CapabilitySpace<TModel, TCapabilities...> m_unit;
    void Assemble(RegistryVector& registry) const override { registry.push_back(&m_unit); }
};

// =============================================================================
// 5. THE ROUTER (HYBRID)
// =============================================================================

class CapabilityRouter {
private:
    static void EnsureBaked() { struct StaticGuard { StaticGuard() { CapabilityRouter::Bake(); } }; static StaticGuard s_Guard; }
public:
    static void Bake() {
        RouterSlot::s_Value.clear();
        for (auto* b = NodeListAnchor<IBakerNode>::s_Value; b; b = b->m_Next) b->Assemble(RouterSlot::s_Value);
    }

    // Dynamic Search (O(Broad + Narrow))
    template<class InterfaceT, class... Coords>
    static const InterfaceT* Find(ModelTypeID modelID, const typename ContextTrait<InterfaceT>::Type& ctx, Coords... coords) {
        EnsureBaked();
        using TSpace = typename CapabilityTopology<InterfaceT>::SpaceType;
        std::size_t offset = (sizeof...(Coords) > 0) ? TSpace::ComputeOffset(coords...) : 0;
        for (const auto* node : RouterSlot::s_Value) {
            if (node->GetTargetModelID() == modelID) {
                if (const void* buckets = node->Resolve(TypeIDOf<InterfaceT>::Get())) {
                    auto* routeNode = static_cast<const Bucket<InterfaceT>*>(buckets)[offset].head;
                    for (; routeNode; routeNode = routeNode->next) if (routeNode->Matches(ctx)) return routeNode->implementation;
                }
            }
        }
        return nullptr;
    }

    // Static Search (O(1) Default)
    template<class InterfaceT, class... Coords>
    static const InterfaceT* Find(ModelTypeID modelID, Coords... coords) {
        EnsureBaked();
        using TSpace = typename CapabilityTopology<InterfaceT>::SpaceType;
        std::size_t offset = (sizeof...(Coords) > 0) ? TSpace::ComputeOffset(coords...) : 0;
        for (const auto* node : RouterSlot::s_Value) {
            if (node->GetTargetModelID() == modelID) {
                if (const void* buckets = node->Resolve(TypeIDOf<InterfaceT>::Get())) 
                    return static_cast<const Bucket<InterfaceT>*>(buckets)[offset].fallback;
            }
        }
        return nullptr;
    }
};

// =============================================================================
// 6. GPP IMPLEMENTATION (PURE LOGIC)
// =============================================================================

struct ICombat {
    struct RuleContext { float hp; };
    virtual void Execute() const = 0;
};

enum class WorldState { Day, Night };
template<> struct EnumTraits<WorldState> { static constexpr std::size_t Count = 2; };
template<> struct CapabilityTopology<ICombat> { using SpaceType = Space<WorldState>; };

// FALLBACK
template<class T, class TAt = At<>> 
struct Melee : Capability<ICombat> {
    void Execute() const override { std::cout << "Normal Strike.\n"; }
};

// RULE EXCEPTION
template<class T, auto... R>
struct Berserk : Capability<ICombat> {
    // Priority is a meta-weight for the Baker to sort rules
    static constexpr int Priority = 100; 

    // Directly uses ICombat::RuleContext via natural inheritance scope
    static bool Condition(const RuleContext& ctx) { return ctx.hp < 20.0f; }

    void Execute() const override { std::cout << "!!! BERSERK OVERDRIVE !!!\n"; }
};

struct Drone {};
static const CapabilityBaker<Drone, Melee, Berserk> g_DroneBaker{};

int main() {
    std::cout << "--- CRG STAGE 8: DYNAMIC RULES (NARROW PHASE) ---\n\n";

    auto id = TypeIDOf<Drone>::Get();
    ICombat::RuleContext lowHealth { 10.0f };

    std::cout << "Dynamic Find (Rules evaluated):\n -> ";
    if (auto* c = CapabilityRouter::Find<ICombat>(id, lowHealth, WorldState::Day)) c->Execute();

    std::cout << "\nStatic Find (O(1) Fallback):\n -> ";
    if (auto* c = CapabilityRouter::Find<ICombat>(id, WorldState::Day)) c->Execute();

    return 0;
}