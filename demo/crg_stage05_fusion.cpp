// Copyright (c) 2024-2026 Cyril Tissier. All rights reserved.
// Licensed under the Apache License, Version 2.0.
//
// =============================================================================
// CAPABILITY ROUTING GATEWAY (CRG) - STAGE 5: FUSION (THE HERO CODE)
// =============================================================================
//
// @intent:
// The definitive fusion of all previous stages. This stage introduces the 
// CapabilityBaker, which automates the registration of entire domains, and 
// the CapabilityRouter, which provides O(1) resolution via a contiguous registry.
//
// @architecture:
// - Capability<T>: Bridge helper that injects metadata into user logic.
// - CapabilityBaker: Variadic aggregator that "bakes" capabilities into the router.
// - CapabilityRouter: The high-performance gateway (O(1) lookup).
// =============================================================================

#include <iostream>
#include <typeinfo>
#include <vector>
#include <memory>
#include <string>

// =============================================================================
// 1. INFRASTRUCTURE (Universal Storage)
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
// 2. CORE TYPES & HELPERS
// =============================================================================

using ModelTypeID = std::size_t;
using InterfaceTypeID = std::size_t; 

template<class T> struct TypeIDOf { static std::size_t Get() { return typeid(T).hash_code(); } };
template<typename... Ts> struct TypeList {};

// --- CAPABILITY HELPER ---
// This simple bridge eliminates the need for manual 'InterfaceType' aliases.
template<class TInterface> 
struct Capability : public TInterface { 
    using InterfaceType = TInterface; 
};

// --- ROUTING INTERFACE ---
struct IRegistryNode {
    virtual ~IRegistryNode() = default;
    virtual ModelTypeID GetTargetModelID() const = 0;
    virtual const void* Resolve(InterfaceTypeID interfaceID) const = 0;
};

using RegistryVector = std::vector<const IRegistryNode*>;
using RouterSlot = RegistrySlot<RegistryVector>;
CRG_BIND_SLOT(RegistryVector)

// --- BAKER INFRASTRUCTURE ---
struct IAssembler {
    virtual ~IAssembler() = default;
    virtual void Assemble(RegistryVector& registry) const = 0;
};

struct IBakerNode : public NodeList<IBakerNode, IAssembler> {};
CRG_BIND_SLOT(const IBakerNode*)

// =============================================================================
// 3. THE BAKER CORE (Variadic Aggregation)
// =============================================================================

// The Space aggregates all provided Capabilities for a specific Model.
template<class TModel, template<class> class... TCapabilities>
class CapabilitySpace : public IRegistryNode, public TCapabilities<TModel>... {
public:
    ModelTypeID GetTargetModelID() const override { return TypeIDOf<TModel>::Get(); }
    
    const void* Resolve(InterfaceTypeID interfaceID) const override {
        const void* result = nullptr;
        // Fold expression: searches through all inherited Capability bases.
        ((interfaceID == TypeIDOf<typename TCapabilities<TModel>::InterfaceType>::Get() && 
         (result = static_cast<const typename TCapabilities<TModel>::InterfaceType*>(this))), ...);
        return result;
    }
};

// Primary Template: Single Model registration
template<class TModel, template<class> class... TCapabilities>
struct CapabilityBaker : public IBakerNode {
    CapabilitySpace<TModel, TCapabilities...> m_unit;
    void Assemble(RegistryVector& registry) const override { registry.push_back(&m_unit); }
};

// Specialization: Batch registration for multiple Models via TypeList
template<class... Models, template<class> class... TCapabilities>
struct CapabilityBaker<TypeList<Models...>, TCapabilities...> : public CapabilityBaker<Models, TCapabilities...>... {
    void Assemble(RegistryVector& registry) const override {
        // Delegates assembly to every model-specific baker parent.
        (CapabilityBaker<Models, TCapabilities...>::Assemble(registry), ...);
    }
};

// =============================================================================
// 4. THE ROUTER (High-Performance Gateway)
// =============================================================================

class CapabilityRouter {
private:
    // Ensures a single global bake happens only once, even with multiple interface queries.
    static void EnsureBaked() {
        struct StaticGuard { StaticGuard() { CapabilityRouter::Bake(); } };
        static StaticGuard s_Guard;
    }

public:
    static void Bake() {
        auto& registry = RouterSlot::s_Value;
        registry.clear();
        // Traverse the intrusive list of bakers and collect their assembled units.
        for (auto* b = NodeListAnchor<IBakerNode>::s_Value; b; b = b->m_Next) {
            b->Assemble(registry);
        }
    }

    template<class InterfaceT>
    static const InterfaceT* Find(ModelTypeID modelID) {
        EnsureBaked();

        const auto& registry = RouterSlot::s_Value;
        const InterfaceTypeID iid = TypeIDOf<InterfaceT>::Get();

        // O(1) in concept (direct access), O(N_models) for ID matching.
        for (const auto* node : registry) {
            if (node->GetTargetModelID() == modelID) {
                if (const void* ptr = node->Resolve(iid)) {
                    return static_cast<const InterfaceT*>(ptr);
                }
            }
        }
        return nullptr;
    }
};

// =============================================================================
// 5. USER DOMAIN (Pure Logic)
// =============================================================================

// --- Interfaces ---
struct IDiagnostic { 
    virtual ~IDiagnostic() = default; 
    virtual void Run() const = 0; 
};

struct ITelemetry { 
    virtual ~ITelemetry() = default; 
    virtual void Ping() const = 0; 
};

// --- Implementations (using Capability helper) ---
template<class T> 
struct DiagCapability : Capability<IDiagnostic> { 
    void Run() const override { std::cout << "[Diag] System check for: " << typeid(T).name() << "\n"; } 
};

template<class T> 
struct TeleCapability : Capability<ITelemetry> { 
    void Ping() const override { std::cout << "[Tele] Pinging: " << typeid(T).name() << "\n"; } 
};

// --- Models ---
struct Drone {};
struct Scout {};

// --- Registration ---
// One line to bind a whole domain of models to their logic.
using AirModels = TypeList<Drone, Scout>;
static const CapabilityBaker<AirModels, DiagCapability, TeleCapability> g_AirBaker{};

// =============================================================================
// MAIN
// =============================================================================
int main() {
    std::cout << "--- CRG STAGE 5: THE CAPABILITY ROUTER ---\n\n";

    // Scenario: An ECS system wants to run diagnostics on a Scout
    ModelTypeID scoutID = TypeIDOf<Scout>::Get();

    // The Find call triggers the first Bake() automatically.
    if (auto* d = CapabilityRouter::Find<IDiagnostic>(scoutID)) {
        d->Run();
    }

    if (auto* t = CapabilityRouter::Find<ITelemetry>(scoutID)) {
        t->Ping();
    }

    // Engine Sync: Simulation of a Hot-Reload
    std::cout << "\n[Engine] Manual Sync triggered.\n";
    CapabilityRouter::Bake(); 

    return 0;
}