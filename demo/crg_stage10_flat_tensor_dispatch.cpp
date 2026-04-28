// Copyright (c) 2024-2026 Cyril Tissier. All rights reserved.
// =============================================================================
// CAPABILITY ROUTING GATEWAY (CRG) - STAGE 10: FLAT TENSOR (DLL COMPLIANT)
// =============================================================================

#include <iostream>
#include <vector>
#include <tuple>
#include <typeinfo>

// =============================================================================
// 1. INFRASTRUCTURE DLL-SAFE (RESTORED FROM STAGE 9)
// =============================================================================

#ifndef CRG_DLL_ENABLED
#define CRG_DLL_ENABLED 1
#endif

template<class T> struct RegistrySlot {
#if !CRG_DLL_ENABLED
    static inline T s_Value{}; 
#else
    // En mode DLL, s_Value est une promesse d'existence externe
    static T s_Value;
#endif
};

#if CRG_DLL_ENABLED
    // Macro pour réaliser l'allocation physique dans le module de ton choix
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
// 2. TENSOR MATH (HORNER)
// =============================================================================

template<typename T> struct EnumTraits;

template<class... TAxes>
struct Space {
    static constexpr std::size_t Volume = (1 * ... * EnumTraits<TAxes>::Count);

    // Horner's Method: O(D) index linearization
    static constexpr std::size_t ComputeOffset(TAxes... coords) {
        std::size_t offset = 0;
        ((offset = offset * EnumTraits<TAxes>::Count + static_cast<std::size_t>(coords)), ...);
        return offset;
    }
};

template<class TInterface> struct CapabilityTopology;

// =============================================================================
// 3. CORE ROUTING TYPES
// =============================================================================

template<class InterfaceT>
struct RouteNode {
    using ContextT = typename InterfaceT::RuleContext;
    using PredicatePtr = bool (*)(const ContextT&);

    const InterfaceT* implementation = nullptr;
    PredicatePtr      predicate      = nullptr; 
    int               priority       = 0;
    RouteNode* next           = nullptr;

    bool Matches(const ContextT& ctx) const { return !predicate || predicate(ctx); }
};

template<class InterfaceT>
struct Bucket {
    RouteNode<InterfaceT>* head     = nullptr;      
    const InterfaceT* fallback = nullptr;  
};

// Utilisation du RegistrySlot pour l'arène contiguë
template<class InterfaceT>
using TensorAnchor = RegistrySlot<std::vector<Bucket<InterfaceT>>>;

// =============================================================================
// 4. THE ROUTER (LAZY FLAT DISPATCH)
// =============================================================================

struct IBakerNode;
struct IAssembler { virtual void Bake() const = 0; };
struct IBakerNode : public NodeList<IBakerNode, IAssembler> {};

// BINDING OBLIGATOIRE : Ancre de la NodeList
CRG_BIND_SLOT(const IBakerNode*)

template<class InterfaceT>
class BehaviorRouter {
public:
    static void EnsureBaked() {
        static struct StaticGuard {
            StaticGuard() {
                // Utilise la NodeList partagée entre DLLs pour remplir l'arène
                for (const IBakerNode* node = NodeListAnchor<IBakerNode>::s_Value; node; node = node->m_Next) {
                    node->Bake();
                }
            }
        } s_guard;
    }

    static void Register(std::size_t modelID, std::size_t localOffset, RouteNode<InterfaceT>* node) {
        auto& arena = TensorAnchor<InterfaceT>::s_Value;
        using TSpace = typename CapabilityTopology<InterfaceT>::SpaceType;
        
        std::size_t globalIdx = (modelID * TSpace::Volume) + localOffset;
        if (globalIdx >= arena.size()) arena.resize(globalIdx + 1, {});

        auto& bucket = arena[globalIdx];
        if (!node->predicate) bucket.fallback = node->implementation;

        // Tri par priorité (Narrow Phase)
        if (!bucket.head || node->priority > bucket.head->priority) {
            node->next = bucket.head;
            bucket.head = node;
        } else {
            auto* curr = bucket.head;
            while (curr->next && curr->next->priority >= node->priority) curr = curr->next;
            node->next = curr->next;
            curr->next = node;
        }
    }

    template<class... Coords>
    static const InterfaceT* Find(std::size_t modelID, const typename InterfaceT::RuleContext& ctx, Coords... coords) {
        EnsureBaked();
        auto& arena = TensorAnchor<InterfaceT>::s_Value;
        using TSpace = typename CapabilityTopology<InterfaceT>::SpaceType;
        std::size_t idx = (modelID * TSpace::Volume) + TSpace::ComputeOffset(coords...);
        
        if (idx >= arena.size()) return nullptr;
        for (auto* n = arena[idx].head; n; n = n->next) {
            if (n->Matches(ctx)) return n->implementation;
        }
        return nullptr;
    }
};

// =============================================================================
// 5. SAMPLE GAMEPLAY (POUR VÉRIFIER)
// =============================================================================

struct ICombat {
    struct RuleContext { float hp; };
    virtual void Execute() const = 0;
};

// BINDING OBLIGATOIRE : Ancre de l'arène de combat
CRG_BIND_SLOT(std::vector<Bucket<ICombat>>)

enum class WorldState { Day, Night };
template<> struct EnumTraits<WorldState> { static constexpr std::size_t Count = 2; };
template<> struct CapabilityTopology<ICombat> { using SpaceType = Space<WorldState>; };

// ... reste de l'implémentation (Bakers, Logic)