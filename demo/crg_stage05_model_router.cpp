// Copyright (c) 2026 Cyril Tissier. All rights reserved.
// Licensed under the Apache License, Version 2.0.
//
// =============================================================================
// CAPABILITY ROUTING GATEWAY (CRG) - STAGE 5: MODEL ROUTER (OPTIMIZED)
// =============================================================================
//
// @intent:
// Create an OOP-style illusion directly on the ModelShell.
// - ModelRouter: Implements global resolution for a specific Model.
// - Stateless Enforcement: ModelShellMethodTraits strictly rejects non-const methods.
// - Pure TryInvoke: Direct return via implicit conversion.
// =============================================================================

#include <iostream>
#include <memory>
#include <typeinfo>
#include <string>
#include <optional>
#include <cassert>
#include <type_traits>

using ModelTypeID = std::size_t;
using InterfaceTypeID = std::size_t;

template<class T> struct TypeIDOf { static ModelTypeID Get() { return typeid(T).hash_code(); } };

// =============================================================================
// 1. INFRASTRUCTURE & DLL SAFETY
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

template<class TNode> using NodeListAnchor = RegistrySlot<const TNode*>;

template<class TNode, class TInterface>
struct NodeList : public TInterface {
    const TNode* m_Next = nullptr;
    NodeList() {
        m_Next = NodeListAnchor<TNode>::s_Value;
        NodeListAnchor<TNode>::s_Value = static_cast<const TNode*>(this);
    }
};

// =============================================================================
// 2. UNIFIED BINDER CORE
// =============================================================================

struct IRouteable {
    virtual ~IRouteable() = default;
    virtual ModelTypeID     GetTargetModelID() const = 0;
    virtual InterfaceTypeID GetInterfaceID() const = 0;
    virtual const void*     GetRawInterface() const = 0;
};

struct ICapabilityNode : public NodeList<ICapabilityNode, IRouteable> {};

template<class ModelT, template<class> class CapabilityT>
class CapabilityBinding : 
    public ICapabilityNode, 
    public CapabilityT<ModelT> 
{
    using InterfaceT = typename CapabilityT<ModelT>::InterfaceType;
public:
    ModelTypeID GetTargetModelID() const override { return TypeIDOf<ModelT>::Get(); }
    InterfaceTypeID GetInterfaceID() const override { return TypeIDOf<InterfaceT>::Get(); }
    const void* GetRawInterface() const override { return static_cast<const InterfaceT*>(this); }
};

// =============================================================================
// 3. MODEL ROUTER
// =============================================================================

template<class ModelT>
class ModelRouter {
public:
    static const void* Resolve(InterfaceTypeID interfaceID) {
        const ModelTypeID modelID = TypeIDOf<ModelT>::Get();
        for (auto* n = NodeListAnchor<ICapabilityNode>::s_Value; n; n = n->m_Next) {
            if (n->GetTargetModelID() == modelID && n->GetInterfaceID() == interfaceID) {
                return n->GetRawInterface();
            }
        }
        return nullptr;
    }
};

// =============================================================================
// 4. TRAITS EXTRACTOR
// =============================================================================

template <typename T> struct ModelShellMethodTraits;

// STRICT ENFORCEMENT: Only const member functions are allowed.
template <typename R, typename ClassType, typename... Args>
struct ModelShellMethodTraits<R (ClassType::*)(Args...) const> {
    using Interface = ClassType;
    using ReturnType = R;
};

// Return type deduction helper to avoid "auto deduction conflict" compilation errors
template<auto FuncPtr>
using TryInvokeRetType = std::conditional_t<
    std::is_void_v<typename ModelShellMethodTraits<decltype(FuncPtr)>::ReturnType>,
    void,
    std::optional<typename ModelShellMethodTraits<decltype(FuncPtr)>::ReturnType>
>;

// =============================================================================
// 5. TYPE ERASED SHELL (THE OOP ILLUSION)
// =============================================================================

class ModelShell {
public:
    struct Concept { 
        virtual ~Concept() = default; 
        virtual ModelTypeID GetTypeID() const = 0; 
        virtual const void* Resolve(InterfaceTypeID id) const = 0; 
    };
    
    template<class T> struct Model : Concept {
        T value; 
        Model(T v) : value(std::move(v)) {}
        
        ModelTypeID GetTypeID() const override { return TypeIDOf<T>::Get(); }
        const void* Resolve(InterfaceTypeID id) const override { return ModelRouter<T>::Resolve(id); }
    };

    ModelShell() = default;
    template<class T> void Set(T v) { m_ptr = std::make_unique<Model<T>>(std::move(v)); }
    ModelTypeID GetTypeID() const { return m_ptr ? m_ptr->GetTypeID() : 0; }

    // --- Invoke: Assumes success (Fail-fast via assert) ---
    template<auto FuncPtr, class... TArgs>
    auto Invoke(TArgs&&... args) const {
        using Traits = ModelShellMethodTraits<decltype(FuncPtr)>;
        using ExpectedInterface = typename Traits::Interface;

        assert(m_ptr != nullptr && "Invoke called on empty ModelShell");
        const void* rawPtr = m_ptr->Resolve(TypeIDOf<ExpectedInterface>::Get());
        assert(rawPtr != nullptr && "Interface not found for this Model. Use TryInvoke if unsure.");

        const auto* capability = static_cast<const ExpectedInterface*>(rawPtr);
        return (capability->*FuncPtr)(*this, std::forward<TArgs>(args)...);
    }

    // --- TryInvoke: Exactly as requested ---
    template<auto FuncPtr, class... TArgs>
    TryInvokeRetType<FuncPtr> TryInvoke(TArgs&&... args) const {
        using Traits = ModelShellMethodTraits<decltype(FuncPtr)>;
        using ExpectedInterface = typename Traits::Interface;
        using RetType = typename Traits::ReturnType;

        const void* rawPtr = m_ptr ? m_ptr->Resolve(TypeIDOf<ExpectedInterface>::Get()) : nullptr;
        const auto* capability = static_cast<const ExpectedInterface*>(rawPtr);

        if constexpr (std::is_void_v<RetType>) {
            if (capability) {
                (capability->*FuncPtr)(*this, std::forward<TArgs>(args)...);
            }
        } else {
            if (capability) {
                return (capability->*FuncPtr)(*this, std::forward<TArgs>(args)...);
            }
            return std::optional<RetType>{};
        }
    }

private:
    std::unique_ptr<Concept> m_ptr;
};

// =============================================================================
// 6. USER DOMAIN
// =============================================================================

struct IDiagnostic {
    using InterfaceType = IDiagnostic;
    virtual ~IDiagnostic() = default;
    virtual void Execute(const ModelShell& shell) const = 0; 
};

struct ITelemetry {
    using InterfaceType = ITelemetry;
    virtual ~ITelemetry() = default;
    virtual std::string RetrieveData(const ModelShell& shell, int verbosity) const = 0; 
    virtual std::optional<int> GetSignalStrength(const ModelShell& shell) const = 0; 
};

// Data Models
struct Drone {};
struct Scout {};

// Capabilities
template<class T>
struct DiagnosticCapability : IDiagnostic {
    void Execute(const ModelShell& shell) const override { 
        std::cout << "[Diag] System check pass for: " << typeid(T).name() << "\n"; 
    }
};

template<class T>
struct TelemetryCapability : ITelemetry {
    std::string RetrieveData(const ModelShell& shell, int verbosity) const override { 
        if (verbosity > 1) return "Detailed Telemetry: ALL SYSTEMS GREEN";
        return "Basic Telemetry: OK";
    }
    
    std::optional<int> GetSignalStrength(const ModelShell& shell) const override {
        return 95; 
    }
};

// =============================================================================
// REGISTRATION
// =============================================================================
static const CapabilityBinding<Drone, DiagnosticCapability> g_DroneDiag;
static const CapabilityBinding<Scout, DiagnosticCapability> g_ScoutDiag;
static const CapabilityBinding<Drone, TelemetryCapability>  g_DroneTele;

// =============================================================================
// MAIN
// =============================================================================
int main() {
    std::cout << "--- CRG STAGE 5: MODEL ROUTER (OPTIMIZED) ---\n\n";

    ModelShell droneShell;
    droneShell.Set(Drone{});

    std::cout << "--- Drone Shell ---\n";
    droneShell.Invoke<&IDiagnostic::Execute>();
    
    std::string data = droneShell.Invoke<&ITelemetry::RetrieveData>(2);
    std::cout << "[Tele] Data received: " << data << "\n";

    auto signalOpt = droneShell.TryInvoke<&ITelemetry::GetSignalStrength>();
    if (signalOpt.has_value() && signalOpt.value().has_value()) {
        std::cout << "[Tele] Signal Strength: " << signalOpt.value().value() << "%\n\n";
    }

    std::cout << "--- Scout Shell ---\n";
    ModelShell scoutShell;
    scoutShell.Set(Scout{});
    
    scoutShell.TryInvoke<&IDiagnostic::Execute>();
    std::cout << "-> Diagnostic TryInvoke attempted.\n";

    auto missingData = scoutShell.TryInvoke<&ITelemetry::RetrieveData>(1);
    if (!missingData.has_value()) {
        std::cout << "-> [Tele] Capability NOT supported on Scout (Graceful silent fail)!\n";
    }

    return 0;
}