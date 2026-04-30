// Copyright (c) 2026 Cyril Tissier. All rights reserved.
// Licensed under the Apache License, Version 2.0.
//
// =============================================================================
// CAPABILITY CRG - STAGE 1: INTRUSIVE INFRASTRUCTURE
// =============================================================================
//
// @intent:
// Establish a zero-allocation, DLL-compliant discovery backbone using 
// intrusive linked lists and decentralized static anchors.
//
// @architecture:
// - NodeListAnchor: The "Hook". Manages the head of the list. Hybrid design
//   supporting both C++17 'inline' (Monolithic) and explicit (DLL) linkage.
// - NodeList (CRTP): The "Plumbing". Injects self-registration logic into 
//   any class hierarchy without manual registry management.
// =============================================================================

#include <iostream>

// =============================================================================
// 1. THE ANCHOR (Hybrid Linkage Manager)
// =============================================================================
#ifndef CRG_DLL_ENABLED
#define CRG_DLL_ENABLED 1
#endif

template<class T>
struct RegistrySlot {
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

// =============================================================================
// 2. THE MIXIN (Intrusive Auto-Registration)
// =============================================================================
template<class TNode>
using NodeListAnchor = RegistrySlot<const TNode*>;

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
// 3. EXAMPLE USAGE (The "Behavior" Domain)
// =============================================================================
struct IBehavior {
    virtual ~IBehavior() = default;
    virtual void Execute() const = 0;
};

// Merging the infrastructure with the domain interface.
struct BehaviorNode : public NodeList<BehaviorNode, IBehavior> {};

// In DLL mode, this would live in BehaviorNode.cpp
CRG_BIND_SLOT(const BehaviorNode*)

struct DroneBehavior : public BehaviorNode { 
    void Execute() const override { std::cout << "Drone: Scanning area.\n"; } 
};

struct TankBehavior : public BehaviorNode { 
    void Execute() const override { std::cout << "Tank: Target locked.\n"; } 
};

// =============================================================================
// MAIN: INFRASTRUCTURE VERIFICATION
// =============================================================================
int main() {
    std::cout << "--- CRG STAGE 1: Intrusive Infrastructure ---\n\n";

    DroneBehavior a; 
    TankBehavior b; 

    // We iterate through the Anchor, proving the infrastructure works.
    for (const BehaviorNode* n = NodeListAnchor<BehaviorNode>::s_Value; n; n = n->m_Next) {
        n->Execute();
    }

    return 0;
}