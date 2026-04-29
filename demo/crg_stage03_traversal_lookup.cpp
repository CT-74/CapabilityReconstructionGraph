// Copyright (c) 2024-2026 Cyril Tissier. All rights reserved.
// Licensed under the Apache License, Version 2.0.
//
// =============================================================================
// CAPABILITY CRG - STAGE 3: TRAVERSAL LOOKUP
// =============================================================================
//
// @intent:
// Resolve behavior by matching Model Identity during an O(N) traversal.
//
// @architecture:
// - ModelHandle: Uses std::unique_ptr (Stage 2 version).
// - Plumbing (Anchor & NodeList): The intrusive infrastructure from Stage 1.
// - Templated Behavior: A generic class that bridges the handle to the data 
//   and executes logic using the typed model.
// =============================================================================

#include <iostream>
#include <memory>
#include <typeinfo>
#include <string>

// =============================================================================
// 1. THE TRANSPORT (From Stage 2)
// =============================================================================
// Global identity type
using ModelTypeID = std::size_t;
template<class T> struct TypeIDOf { static ModelTypeID Get() { return typeid(T).hash_code(); } };

class ModelShell {
public:
    struct Concept { 
        virtual ~Concept() = default; 
        virtual ModelTypeID GetModelID() const = 0; 
    };

    template<class T> 
    struct Model : Concept {
        T data;
        Model(T v) : data(std::move(v)) {}
        ModelTypeID GetModelID() const override { return TypeIDOf<T>::Get(); }
    };

    template<class T> 
    void Set(T v) { m_ptr = std::make_unique<Model<T>>(std::move(v)); }

    ModelTypeID GetID() const { return m_ptr ? m_ptr->GetModelID() : 0; }

    template<class T> 
    const T& GetAs() const { 
        return static_cast<const Model<T>*>(m_ptr.get())->data; 
    }

private:
    std::unique_ptr<Concept> m_ptr;
};

// =============================================================================
// 2. THE PLUMBING (From Stage 1 - Decentralized)
// =============================================================================
#ifndef CRG_DLL_ENABLED
#define CRG_DLL_ENABLED 0
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
// 3. THE DOMAIN (Interface & Data Models)
// =============================================================================
struct IMovement {
    virtual ~IMovement() = default;
    virtual ModelTypeID GetTargetModelID() const = 0;
    virtual void Execute(const ModelShell& s) const = 0;
};

// Concrete Router Node for Movement
struct MovementNode : public NodeList<MovementNode, IMovement> {};
CRG_BIND_SLOT(const MovementNode*)

struct Scout { std::string callsign; };
struct HeavyTank { int armor; };

// =============================================================================
// 4. THE BEHAVIOR IMPLEMENTATION (The Template Bridge)
// =============================================================================
template<class TModel>
struct TMovementLogic : MovementNode {
    ModelTypeID GetTargetModelID() const override { 
        return typeid(TModel).hash_code(); 
    }
    
    void Execute(const ModelShell& s) const override {
        // Recovering the typed data from the handle using the node's TModel
        const TModel& data = s.GetAs<TModel>();
        
        std::cout << "[Traversal Lookup] Found logic for: " << typeid(TModel).name() << "\n";
    }
};

// =============================================================================
// 5. THE LOOKUP ENGINE (O(N) Traversal)
// =============================================================================
template<class TNode>
const TNode* FindBehavior(const ModelShell& s) {
    // The Traversal: Walking the intrusive list to match the Identity
    for (const TNode* n = NodeListAnchor<TNode>::s_Value; n != nullptr; n = n->m_Next) {
        if (n->GetTargetModelID() == s.GetID()) return n;
    }
    return nullptr;
}

// =============================================================================
// MAIN: INTEGRATION TEST
// =============================================================================
static const TMovementLogic<Scout> g_scoutLogic;
static const TMovementLogic<HeavyTank> g_tankLogic;

int main() {
    std::cout << "--- CRG STAGE 3: Traversal Lookup ---\n\n";
    ModelShell shell;
    
    shell.Set(Scout{"Vanguard-01"});
    if (const auto* logic = FindBehavior<MovementNode>(shell)) {
        logic->Execute(shell);
    }

    shell.Set(HeavyTank{600});
    if (const auto* logic = FindBehavior<MovementNode>(shell)) {
        logic->Execute(shell);
    }

    return 0;
}