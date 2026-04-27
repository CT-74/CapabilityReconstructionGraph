// Copyright (c) 2024-2026 Cyril Tissier. All rights reserved.
// Licensed under the Apache License, Version 2.0.
// See LICENSE file in the project root for full license information.
//
// =============================================================================
// CAPABILITY ROUTING GATEWAY (CRG) - STAGE 5: FUSION (THE BEHAVIOR BAKER)
// =============================================================================
//
// @intent:
// Provide an ECS-ready O(1) resolution API by completely decoupling 
// memory identity from behavior topology. Prove domain decentralization.
//
// @what_changed:
// - ModelHandle acts strictly as a Type-Erased data container.
// - BehaviorMatrixList acts as the global External Polymorphism Router.
// - Multiple Domain Bakers demonstrate that behaviors can be compiled 
//   and injected completely independently for the same Models.
//
// @note:
// For pedagogical clarity on GitHub, ModelHandle uses std::unique_ptr.
// In a AAA production engine, this relies on Small Buffer Optimization (SBO)
// to guarantee zero-heap-allocation. See the CppCon paper for details.
// =============================================================================

#include <iostream>
#include <typeinfo>
#include <string>
#include <memory>
#include <tuple>

// --- CORE UTILITIES ---
using ModelTypeID = std::size_t;
template<class T> struct TypeIDOf { static ModelTypeID Get() { return typeid(T).hash_code(); } };
template<typename... Ts> struct TypeList {};

// --- 1. TYPE ERASED SHELL (The Data Container) ---
class ModelHandle {
public:
    struct Concept { virtual ~Concept() = default; virtual ModelTypeID GetTypeID() const = 0; };
    
    template<class T> struct Model : Concept {
        T value; Model(T v) : value(std::move(v)) {}
        ModelTypeID GetTypeID() const override { return TypeIDOf<T>::Get(); }
    };
    
    ModelHandle() = default;
    
    template<class T> void Set(T v) {
        m_ptr = std::make_unique<Model<T>>(std::move(v)); 
    }
    
    ModelTypeID GetTypeID() const { return m_ptr ? m_ptr->GetTypeID() : 0; }
    
    template<class T> const T& Get() const { 
        return static_cast<const Model<T>*>(m_ptr.get())->value; 
    }
private:
    std::unique_ptr<Concept> m_ptr;
};

// --- EXTERNAL POLYMORPHISM LAYER ---

// 1. The common interface for all baked behavior nodes
struct IModelRegistryNode {
    virtual ModelTypeID GetModelTypeID() const = 0;
    virtual const void* Resolve(ModelTypeID interfaceID) const = 0;
};

// 2. The Global Registry (The Router)
class BehaviorMatrixList {
    static inline const IModelRegistryNode* s_nodes[128] = {};
    static inline std::size_t s_nodeCount = 0;

public:
    static void Register(const IModelRegistryNode* node) {
        if (s_nodeCount < 128) s_nodes[s_nodeCount++] = node;
    }

    // THE CORE ECS API: Resolves an interface solely from a TypeID.
    // Iterates over registered domain chunks. 
    // (Note: In production, this uses dense indexing for true O(1) lookup).
    template<class InterfaceT>
    static const InterfaceT* FindInterface(ModelTypeID modelID) {
        for (std::size_t i = 0; i < s_nodeCount; ++i) {
            if (s_nodes[i]->GetModelTypeID() == modelID) {
                if (const void* ptr = s_nodes[i]->Resolve(TypeIDOf<InterfaceT>::Get())) {
                    return static_cast<const InterfaceT*>(ptr);
                }
            }
        }
        return nullptr;
    }
};

// 3. The Capability Node (The Magic)
template<class ModelT, template<class> class... Behaviors>
class BakedCapabilityNode : public IModelRegistryNode, public Behaviors<ModelT>... {
public:
    BakedCapabilityNode() {
        // Auto-Register this baked node into the global matrix at startup
        BehaviorMatrixList::Register(this);
    }

    ModelTypeID GetModelTypeID() const override { 
        return TypeIDOf<ModelT>::Get(); 
    }

    const void* Resolve(ModelTypeID interfaceID) const override {
        const void* result = nullptr;
        // Branchless O(1) evaluation resolved at compile time via C++17 fold expression
        ((interfaceID == TypeIDOf<typename Behaviors<ModelT>::InterfaceType>::Get() && 
         (result = static_cast<const typename Behaviors<ModelT>::InterfaceType*>(this))), ...);
        return result;
    }
};

// --- USER INTERFACES (Pure Domain) ---
struct IDiagnostic {
    using InterfaceType = IDiagnostic; // Metadata for the Baker
    virtual void Execute(const ModelHandle& h) const = 0;
};

struct ITelemetry {
    using InterfaceType = ITelemetry;
    virtual void Send(const ModelHandle& h) const = 0;
};

// --- USER MODELS (Data) ---
struct Drone { std::string name; };
struct HeavyLifter { std::string name; };

// --- BEHAVIOR DEFINITIONS (Logic) ---
template<class T>
struct DiagnosticBehavior : IDiagnostic {
    void Execute(const ModelHandle& h) const override { 
        std::cout << "[CRG] Diagnostic Executed on Model TypeID: " << TypeIDOf<T>::Get() << "\n"; 
    }
};

template<class T>
struct TelemetryBehavior : ITelemetry {
    void Send(const ModelHandle& h) const override { 
        std::cout << "[CRG] Telemetry Sent on Model TypeID: " << TypeIDOf<T>::Get() << "\n"; 
    }
};

// --- THE COMPILE-TIME BAKER (Instantiator) ---
template<class ModelList, template<class> class... Behaviors>
struct DomainBaker;

template<class... Models, template<class> class... Behaviors>
struct DomainBaker<TypeList<Models...>, Behaviors...> {
    // Instantiating this tuple automatically triggers the constructors of BakedCapabilityNode
    // which self-register into the BehaviorMatrixList.
    std::tuple<BakedCapabilityNode<Models, Behaviors...>...> m_nodes;
};

// =============================================================================
// MODULE INSTANTIATIONS (Proving Decentralization)
// =============================================================================

// Extracted explicit model list
using SimulationModels = TypeList<Drone, HeavyLifter>;

// In a real codebase, these lines would live in completely different .cpp/.dll files!
// Module A (Diagnostic System) knows nothing about Module B (Telemetry System).
static const DomainBaker<SimulationModels, DiagnosticBehavior> g_DiagBaker{};
static const DomainBaker<SimulationModels, TelemetryBehavior> g_TelemetryBaker{};

// =============================================================================
// USER CODE (ECS Hot Path Simulation)
// =============================================================================
int main() {
    std::cout << "--- CRG STAGE 5: FUSION (DECENTRALIZED BAKER) ---\n\n";

    ModelHandle h; 
    h.Set(Drone{"Scout-1"});
    
    // 1. The Engine System fetches the ID from the raw data
    ModelTypeID currentModelID = h.GetTypeID();

    // 2. The Systems query the global CRG Matrix independently
    if (const IDiagnostic* diag = BehaviorMatrixList::FindInterface<IDiagnostic>(currentModelID)) {
        diag->Execute(h);
    }

    if (const ITelemetry* telemetry = BehaviorMatrixList::FindInterface<ITelemetry>(currentModelID)) {
        telemetry->Send(h);
    }

    return 0;
}