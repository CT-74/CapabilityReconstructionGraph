// Copyright (c) 2026 Cyril Tissier. All rights reserved.
// Licensed under the Apache License, Version 2.0.
//
// =============================================================================
// CAPABILITY ROUTING GATEWAY (CRG) - STAGE 4: IDENTITY DECOUPLING
// =============================================================================

#include <iostream>
#include <memory>
#include <typeinfo>
#include <string>

using ModelTypeID = std::size_t;
template<class T> struct TypeIDOf { static ModelTypeID Get() { return typeid(T).hash_code(); } };

// =============================================================================
// 1. TYPE ERASED SHELL
// =============================================================================
class ModelShell {
public:
    struct Concept { virtual ~Concept() = default; virtual ModelTypeID GetTypeID() const = 0; };
    template<class T> struct Model : Concept {
        T value; Model(T v) : value(std::move(v)) {}
        ModelTypeID GetTypeID() const override { return TypeIDOf<T>::Get(); }
    };
    ModelShell() = default;
    template<class T> void Set(T v) { m_ptr = std::make_unique<Model<T>>(std::move(v)); }
    ModelTypeID GetTypeID() const { return m_ptr ? m_ptr->GetTypeID() : 0; }
private:
    std::unique_ptr<Concept> m_ptr;
};

// =============================================================================
// 2. INFRASTRUCTURE (Intrusive List)
// =============================================================================
#ifndef CRG_DLL_ENABLED
#define CRG_DLL_ENABLED 0
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

template<class TNode>
using NodeListAnchor = UniversalAnchor<const TNode*>;

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
// 3. THE BINDER CORE
// =============================================================================
struct IRouteable {
    virtual ~IRouteable() = default;
    virtual ModelTypeID GetTargetModelID() const = 0;
};

template<class InterfaceT>
struct ICapabilityNode : public NodeList<ICapabilityNode<InterfaceT>, IRouteable> {
    virtual const InterfaceT* GetInterface() const = 0;
};

template<class ModelT, template<class> class CapabilityT>
class CapabilityBinding : 
    public ICapabilityNode<typename CapabilityT<ModelT>::InterfaceType>, 
    public CapabilityT<ModelT> 
{
public:
    ModelTypeID GetTargetModelID() const override { return TypeIDOf<ModelT>::Get(); }
    const typename CapabilityT<ModelT>::InterfaceType* GetInterface() const override {
        return static_cast<const CapabilityT<ModelT>*>(this);
    }
};

// =============================================================================
// 4. USER DOMAIN (Renamed to Capability)
// =============================================================================

struct IDiagnostic {
    using InterfaceType = IDiagnostic;
    virtual ~IDiagnostic() = default;
    virtual void Execute() const = 0;
};

struct ITelemetry {
    using InterfaceType = ITelemetry;
    virtual ~ITelemetry() = default;
    virtual void Send() const = 0;
};

// Data Models
struct Drone {};
struct Scout {};

// Pure Logic (Renamed from Behavior to Capability)
template<class T>
struct DiagnosticCapability : IDiagnostic {
    void Execute() const override { std::cout << "[Diag] Target: " << typeid(T).name() << "\n"; }
};

template<class T>
struct TelemetryCapability : ITelemetry {
    void Send() const override { std::cout << "[Tele] Target: " << typeid(T).name() << "\n"; }
};

// =============================================================================
// 5. THE GATEWAY
// =============================================================================
template<class InterfaceT>
const InterfaceT* FindCapability(const ModelShell& s) {
    const ModelTypeID id = s.GetTypeID();
    for (auto* n = NodeListAnchor<ICapabilityNode<InterfaceT>>::Get(); n; n = n->m_Next) {
        if (n->GetTargetModelID() == id) return n->GetInterface();
    }
    return nullptr;
}

// =============================================================================
// REGISTRATION
// =============================================================================
static const CapabilityBinding<Drone, DiagnosticCapability> g_DroneDiag;
static const CapabilityBinding<Scout, DiagnosticCapability> g_ScoutDiag;
static const CapabilityBinding<Drone, TelemetryCapability>  g_DroneTele;

int main() {
    std::cout << "--- CRG STAGE 4: IDENTITY DECOUPLING ---\n\n";

    ModelShell shell;
    shell.Set(Drone{});

    if (const auto* diag = FindCapability<IDiagnostic>(shell)) diag->Execute();
    if (const auto* tele = FindCapability<ITelemetry>(shell)) tele->Send();

    return 0;
}