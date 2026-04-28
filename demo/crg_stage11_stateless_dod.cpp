// Copyright (c) 2024-2026 Cyril Tissier. All rights reserved.
//
// =============================================================================
// CAPABILITY ROUTING GATEWAY (CRG) - STAGE 11: STATELESS DOD (FINAL)
// =============================================================================

#include <iostream>
#include <vector>
#include <tuple>
#include <typeinfo>

// =============================================================================
// 1. INFRASTRUCTURE (STAGE 10 COMPLIANT - DLL SAFE)
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

using DenseID = std::size_t;

struct TypeIDGenerator {
    static DenseID GetNext() { static DenseID s_id = 0; return s_id++; }
};

template<class T> 
struct DenseTypeID { 
    static DenseID Get() { static DenseID s_id = TypeIDGenerator::GetNext(); return s_id; } 
};

// =============================================================================
// 2. SPACE MATH (HORNER'S METHOD)
// =============================================================================

template<typename T> struct EnumTraits;

template<class... TAxes>
struct Space {
    static constexpr std::size_t Dimensions = sizeof...(TAxes);
    static constexpr std::size_t Volume = (1 * ... * EnumTraits<TAxes>::Count);

    static constexpr std::size_t ComputeOffset(TAxes... coords) {
        std::size_t offset = 0;
        ((offset = offset * EnumTraits<TAxes>::Count + static_cast<std::size_t>(coords)), ...);
        return offset;
    }
};

template<class TInterface> struct CapabilityTopology;

// =============================================================================
// 3. CORE ROUTING TYPES (STATELESS DOD)
// =============================================================================

template<class InterfaceT>
struct RouteNode {
    using ContextT = typename InterfaceT::RuleContext;
    using PredicatePtr = bool (*)(const ContextT&);

    InterfaceT   descriptor;   
    PredicatePtr predicate = nullptr; 
    int          priority  = 0;
    RouteNode* next      = nullptr;

    bool Matches(const ContextT& ctx) const { return !predicate || predicate(ctx); }
};

template<class InterfaceT>
struct Bucket {
    RouteNode<InterfaceT>* head = nullptr;      
    InterfaceT             fallback; 
    bool                   hasFallback = false;
};

template<class InterfaceT>
using TensorArena = RegistrySlot<std::vector<Bucket<InterfaceT>>>;

// =============================================================================
// 4. THE ROUTER (HOT PATH)
// =============================================================================

struct IBakerNode;
struct IAssembler { virtual void Bake() const = 0; };
struct IBakerNode : public NodeList<IBakerNode, IAssembler> {};

CRG_BIND_SLOT(const IBakerNode*)

template<class InterfaceT>
class BehaviorRouter {
public:
    static void EnsureBaked() {
        static struct StaticGuard {
            StaticGuard() {
                for (const IBakerNode* b = NodeListAnchor<IBakerNode>::s_Value; b; b = b->m_Next) {
                    b->Bake();
                }
            }
        } s_guard;
    }

    static void Register(DenseID modelID, std::size_t localOffset, RouteNode<InterfaceT>* node) {
        auto& arena = TensorArena<InterfaceT>::s_Value;
        using TSpace = typename CapabilityTopology<InterfaceT>::SpaceType;
        
        std::size_t globalIdx = (modelID * TSpace::Volume) + localOffset;
        if (globalIdx >= arena.size()) arena.resize(globalIdx + 1, {});

        auto& b = arena[globalIdx];
        if (!node->predicate) { b.fallback = node->descriptor; b.hasFallback = true; }

        if (!b.head || node->priority > b.head->priority) {
            node->next = b.head; b.head = node;
        } else {
            auto* curr = b.head;
            while (curr->next && curr->next->priority >= node->priority) curr = curr->next;
            node->next = curr->next; curr->next = node;
        }
    }

    template<class... Args>
    static const InterfaceT* Find(DenseID modelID, const typename InterfaceT::RuleContext& ctx, Args... coords) {
        EnsureBaked();
        auto& arena = TensorArena<InterfaceT>::s_Value;
        using TSpace = typename CapabilityTopology<InterfaceT>::SpaceType;
        std::size_t idx = (modelID * TSpace::Volume) + TSpace::ComputeOffset(coords...);
        
        if (idx >= arena.size()) return nullptr;

        for (auto* n = arena[idx].head; n; n = n->next) {
            if (n->Matches(ctx)) return &n->descriptor;
        }
        return &arena[idx].fallback;
    }
};

// =============================================================================
// 5. THE BAKER (DOD COMPILER)
// =============================================================================

template<class TInterface> 
struct Capability : public TInterface { using InterfaceType = TInterface; };

template<typename T, typename = void> struct PriorityOf { static constexpr int Value = 0; };
template<typename T> struct PriorityOf<T, std::void_t<decltype(T::Priority)>> { static constexpr int Value = T::Priority; };

template<typename T, typename Context, typename = void> struct HasCondition : std::false_type {};
template<typename T, typename Context> struct HasCondition<T, Context, std::void_t<decltype(T::Condition(std::declval<const Context&>()))>> : std::true_type {};

template<class TModel, template<class> class... TCapabilities>
struct CapabilityBaker : public IBakerNode {
    void Bake() const override { (BakeOne<TCapabilities<TModel>>(), ...); }

private:
    template<class TImpl>
    void BakeOne() const {
        using Intf = typename TImpl::InterfaceType;
        using TSpace = typename CapabilityTopology<Intf>::SpaceType;
        using Ctx = typename Intf::RuleContext;
        
        for (std::size_t i = 0; i < TSpace::Volume; ++i) {
            auto* node = new RouteNode<Intf>();
            node->descriptor.pfnExecute = &TImpl::Execute; 
            node->priority = PriorityOf<TImpl>::Value;

            if constexpr (HasCondition<TImpl, Ctx>::value) {
                node->predicate = &TImpl::Condition;
            }

            BehaviorRouter<Intf>::Register(DenseTypeID<TModel>::Get(), i, node);
        }
    }
};

// =============================================================================
// 6. GAMEPLAY DOMAIN (NESTED PARAMS)
// =============================================================================

struct ICombat {
    // Nested action payload (Stateless action data)
    struct Params { int hp; int ammo; };

    // Nested query context (Decision data)
    struct RuleContext { const Params& data; };
    
    void (*pfnExecute)(Params&);
    void Execute(Params& p) const { pfnExecute(p); }
};

CRG_BIND_SLOT(std::vector<Bucket<ICombat>>)

enum class CombatState { Idle, Aggressive };
template<> struct EnumTraits<CombatState> { static constexpr std::size_t Count = 2; };
template<> struct CapabilityTopology<ICombat> { using SpaceType = Space<CombatState>; };

template<class T> 
struct StandardStrike : Capability<ICombat> {
    static void Execute(Params& p) { 
        std::cout << " [DOD] Standard Strike. Ammo: " << --p.ammo << "\n"; 
    }
};

template<class T> 
struct CriticalStrike : Capability<ICombat> {
    static constexpr int Priority = 100;
    static bool Condition(const RuleContext& ctx) { return ctx.data.hp < 30; }
    
    static void Execute(Params& p) { 
        std::cout << " [DOD] CRITICAL STRIKE! Ammo used: 2. Remaining: " << (p.ammo -= 2) << "\n"; 
    }
};

struct Soldier {};
static CapabilityBaker<Soldier, StandardStrike, CriticalStrike> g_SoldierCombatModule;

// =============================================================================
// 7. MAIN
// =============================================================================

int main() {
    std::cout << "--- CRG STAGE 11: STATELESS DOD (NESTED PARAMS) ---\n\n";

    DenseID soldierID = DenseTypeID<Soldier>::Get();
    ICombat::Params p { 100, 50 };
    ICombat::RuleContext ruleCtx { p };

    std::cout << "1. Healthy (100 HP):\n -> ";
    if (auto* c = BehaviorRouter<ICombat>::Find(soldierID, ruleCtx, CombatState::Aggressive)) {
        c->Execute(p);
    }

    std::cout << "\n2. Wounded (20 HP):\n -> ";
    p.hp = 20;
    if (auto* c = BehaviorRouter<ICombat>::Find(soldierID, ruleCtx, CombatState::Aggressive)) {
        c->Execute(p);
    }

    return 0;
}