// Copyright (c) 2024-2026 Cyril Tissier. All rights reserved.
// Licensed under the Apache License, Version 2.0.
// See LICENSE file in the project root for full license information.
//
// ======================================================
// STAGE 6 — FUSION (TYPE ERASURE + EXTERNAL POLYMORPHISM)
// ======================================================
//
// @intent:
// Provide an ECS-ready O(1) resolution API by completely decoupling 
// memory identity from behavior topology. Prove domain decentralization.
//
// @what_changed:
// - HardwareHandle acts strictly as a Type-Erased data container.
// - BehaviorMatrixList acts as the global External Polymorphism Router.
// - Multiple Domain Bakers demonstrate that behaviors can be compiled 
//   and injected completely independently for the same Models.
// ======================================================

#include <iostream>
#include <typeinfo>
#include <string>
#include <cstddef>
#include <new>
#include <tuple>

// --- CORE UTILITIES ---
using TypeID = std::size_t;
template<class T> struct TypeIDOf { static TypeID Get() { return typeid(T).hash_code(); } };
template<typename... Ts> struct TypeList {};

// --- TYPE ERASED SHELL (The Data Container) ---
class HardwareHandle {
    static constexpr std::size_t SBO_SIZE = 48;
public:
    struct Concept { virtual ~Concept() = default; virtual TypeID GetTypeID() const = 0; };
    template<class T> struct Model : Concept {
        T value; Model(T v) : value(std::move(v)) {}
        TypeID GetTypeID() const override { return TypeIDOf<T>::Get(); }
    };
    HardwareHandle() = default; ~HardwareHandle() { Reset(); }
    template<class T> void Set(T v) {
        static_assert(sizeof(Model<T>) <= SBO_SIZE, "SBO Capacity Exceeded!");
        Reset(); new (&m_buffer) Model<T>(std::move(v)); m_set = true;
    }
    TypeID GetTypeID() const { return m_set ? reinterpret_cast<const Concept*>(&m_buffer)->GetTypeID() : 0; }
    template<class T> const T& Get() const { return static_cast<const Model<T>*>(reinterpret_cast<const Concept*>(&m_buffer))->value; }
private:
    void Reset() { if(m_set) { reinterpret_cast<Concept*>(&m_buffer)->~Concept(); m_set = false; } }
    alignas(std::max_align_t) std::byte m_buffer[SBO_SIZE]; bool m_set = false;
};

// --- EXTERNAL POLYMORPHISM LAYER ---

// 1. The common interface for all baked behavior nodes
struct IModelRegistryNode {
    virtual TypeID GetModelTypeID() const = 0;
    virtual const void* Resolve(TypeID interfaceID) const = 0;
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
    // Iterates over nodes of the matching ModelID (supports additive decoupled domains).
    template<class InterfaceT>
    static const InterfaceT* FindInterface(TypeID modelID) {
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

// 3. The Capability Node
template<class ModelT, template<class> class... Behaviors>
class BakedCapabilityNode : public IModelRegistryNode, public Behaviors<ModelT>... {
public:
    BakedCapabilityNode() {
        // Auto-Register this baked node into the global list at startup
        BehaviorMatrixList::Register(this);
    }

    TypeID GetModelTypeID() const override { 
        return TypeIDOf<ModelT>::Get(); 
    }

    const void* Resolve(TypeID interfaceID) const override {
        const void* result = nullptr;
        // Branchless O(1) evaluation resolved at compile time via fold expression
        ((interfaceID == TypeIDOf<typename Behaviors<ModelT>::InterfaceType>::Get() && 
         (result = static_cast<const typename Behaviors<ModelT>::InterfaceType*>(this))), ...);
        return result;
    }
};


// --- USER INTERFACES ---
struct IDiagnostic {
    using InterfaceType = IDiagnostic; // Metadata for the Baker
    virtual void Execute(const HardwareHandle& h) const = 0;
};

struct ITelemetry {
    using InterfaceType = ITelemetry;
    virtual void Send(const HardwareHandle& h) const = 0;
};

// --- USER MODELS (Data) ---
struct Drone { std::string name; };
struct HeavyLifter { std::string name; };

// --- BEHAVIOR DEFINITIONS ---
template<class T>
struct DiagnosticBehavior : IDiagnostic {
    void Execute(const HardwareHandle& h) const override { 
        std::cout << "[CRG] Diagnostic Executed on Model TypeID: " << TypeIDOf<T>::Get() << "\n"; 
    }
};

template<class T>
struct TelemetryBehavior : ITelemetry {
    void Send(const HardwareHandle& h) const override { 
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

// ======================================================
// MODULE INSTANTIATIONS (Proving Decentralization)
// ======================================================

// Extracted explicit model list
using SimulationModels = TypeList<Drone, HeavyLifter>;

// In a real codebase, these lines would live in completely different .cpp files!
// Module A (Diagnostic System) knows nothing about Module B (Telemetry System).
static const DomainBaker<SimulationModels, DiagnosticBehavior> g_DiagBaker{};
static const DomainBaker<SimulationModels, TelemetryBehavior> g_TelemetryBaker{};


// ======================================================
// USER CODE (ECS Hot Path Simulation)
// ======================================================
int main() {
    HardwareHandle h; 
    h.Set(Drone{"Scout-1"});

    std::cout << "--- STAGE 06: FUSION (DECENTRALIZED O(1) BAKER) ---\n";
    
    // 1. The System fetches the ID from the raw data
    TypeID currentModelID = h.GetTypeID();

    // 2. The Systems query the global CRG Matrix independently
    if (const IDiagnostic* diag = BehaviorMatrixList::FindInterface<IDiagnostic>(currentModelID)) {
        diag->Execute(h);
    }

    if (const ITelemetry* telemetry = BehaviorMatrixList::FindInterface<ITelemetry>(currentModelID)) {
        telemetry->Send(h);
    }

    return 0;
}