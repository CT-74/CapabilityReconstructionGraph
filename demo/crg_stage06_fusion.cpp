// Copyright (c) 2026 Cyril Tissier. All rights reserved.
// Licensed under the Apache License, Version 2.0.
//
// =============================================================================
// CAPABILITY ROUTING GATEWAY (CRG) - STAGE 6: FUSION + MODEL ROUTER (FINAL)
// =============================================================================
//
// @intent:
// The definitive fusion of the Binding, Router, and OOP Shell.
// - CapabilityBinding: Automates domain registration.
// - CapabilityRouter: The central gateway for raw lookup.
// - ModelRouter & ModelShell: The OOP illusion. Strictly enforces statelessness
//   and the 'const ModelShell&' context parameter.
// =============================================================================

#include <iostream>
#include <typeinfo>
#include <vector>
#include <memory>
#include <string>
#include <optional>
#include <cassert>
#include <type_traits>

// =============================================================================
// 1. INFRASTRUCTURE (Universal Storage & DLL Safety)
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
    #define CRG_DEFINE_SLOT(T) template<> T RegistrySlot<T>::s_Value{};
#else
    #define CRG_DEFINE_SLOT(T) 
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

// Bridge helper that injects metadata into user logic
template<class TInterface> 
struct Capability : public TInterface { 
    using InterfaceType = TInterface; 
};

struct IRegistryNode {
    virtual ~IRegistryNode() = default;
    virtual ModelTypeID GetTargetModelID() const = 0;
    virtual const void* Resolve(InterfaceTypeID interfaceID) const = 0;
};

using RegistryVector = std::vector<const IRegistryNode*>;
using RouterSlot = RegistrySlot<RegistryVector>;
CRG_DEFINE_SLOT(RegistryVector)

struct IAssembler {
    virtual ~IAssembler() = default;
    virtual void Assemble(RegistryVector& registry) const = 0;
};

struct IBindingNode : public NodeList<IBindingNode, IAssembler> {};
CRG_DEFINE_SLOT(const IBindingNode*)

// =============================================================================
// 3. THE BAKER CORE (Variadic Aggregation)
// =============================================================================

template<class TModel, template<class> class... TCapabilities>
class CapabilitySpace : public IRegistryNode, public TCapabilities<TModel>... {
public:
    ModelTypeID GetTargetModelID() const override { return TypeIDOf<TModel>::Get(); }
    
    const void* Resolve(InterfaceTypeID interfaceID) const override {
        const void* result = nullptr;
        ((interfaceID == TypeIDOf<typename TCapabilities<TModel>::InterfaceType>::Get() && 
         (result = static_cast<const typename TCapabilities<TModel>::InterfaceType*>(this))), ...);
        return result;
    }
};

// Primary Template: Single Model registration
template<class TModel, template<class> class... TCapabilities>
struct CapabilityBinding : public IBindingNode {
    CapabilitySpace<TModel, TCapabilities...> m_unit;
    void Assemble(RegistryVector& registry) const override { registry.push_back(&m_unit); }
};

// Specialization: Batch registration via TypeList
template<class... Models, template<class> class... TCapabilities>
struct CapabilityBinding<TypeList<Models...>, TCapabilities...> : public CapabilityBinding<Models, TCapabilities...>... {
    void Assemble(RegistryVector& registry) const override {
        (CapabilityBinding<Models, TCapabilities...>::Assemble(registry), ...);
    }
};

// =============================================================================
// 4. THE CAPABILITY ROUTER (High-Performance Gateway)
// =============================================================================

class CapabilityRouter {
private:
    static void EnsureBaked() {
        struct StaticGuard { StaticGuard() { CapabilityRouter::Bake(); } };
        static StaticGuard s_Guard;
    }

public:
    static void Bake() {
        auto& registry = RouterSlot::s_Value;
        registry.clear();
        for (auto* b = NodeListAnchor<IBindingNode>::s_Value; b; b = b->m_Next) {
            b->Assemble(registry);
        }
    }

    // Typed resolution for direct ECS access
    template<class InterfaceT>
    static const InterfaceT* Find(ModelTypeID modelID) {
        if (const void* ptr = ResolveRaw(modelID, TypeIDOf<InterfaceT>::Get())) {
            return static_cast<const InterfaceT*>(ptr);
        }
        return nullptr;
    }

    // Raw resolution for the ModelRouter integration
    static const void* ResolveRaw(ModelTypeID modelID, InterfaceTypeID interfaceID) {
        EnsureBaked();
        const auto& registry = RouterSlot::s_Value;
        for (const auto* node : registry) {
            if (node->GetTargetModelID() == modelID) {
                return node->Resolve(interfaceID);
            }
        }
        return nullptr;
    }
};

// =============================================================================
// 5. MODEL ROUTER & SHELL (THE OOP ILLUSION)
// =============================================================================

class ModelShell; // Forward declaration required for ModelShellMethodTraits

template <typename T> struct ModelShellMethodTraits;

// STRICT ENFORCEMENT: 
// 1. Must be a const method (Stateless logic).
// 2. The first argument MUST be `const ModelShell&`.
template <typename R, typename ClassType, typename... Args>
struct ModelShellMethodTraits<R (ClassType::*)(const ModelShell&, Args...) const> {
    using Interface = ClassType;
    using ReturnType = R;
};

// Helper trait to seamlessly handle void vs value return types in TryInvoke
template<auto FuncPtr>
using TryInvokeRetType = std::conditional_t<
    std::is_void_v<typename ModelShellMethodTraits<decltype(FuncPtr)>::ReturnType>,
    void,
    std::optional<typename ModelShellMethodTraits<decltype(FuncPtr)>::ReturnType>
>;

// The ModelRouter directly wraps the central CapabilityRouter
template<class ModelT>
class ModelRouter {
public:
    static const void* Resolve(InterfaceTypeID interfaceID) {
        return CapabilityRouter::ResolveRaw(TypeIDOf<ModelT>::Get(), interfaceID);
    }
};

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

    // --- TryInvoke: Pragmatic silent failure / direct optional return ---
    template<auto FuncPtr, class... TArgs>
    TryInvokeRetType<FuncPtr> TryInvoke(TArgs&&... args) const {
        using Traits = ModelShellMethodTraits<decltype(FuncPtr)>;
        using ExpectedInterface = typename Traits::Interface;
        using RetType = typename Traits::ReturnType;

        // Static cast on nullptr is guaranteed safe in C++
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
// 6. USER DOMAIN (Pure Logic)
// =============================================================================

struct IDiagnostic { 
    virtual ~IDiagnostic() = default; 
    virtual void Run(const ModelShell& shell) const = 0; 
};

struct ITelemetry { 
    virtual ~ITelemetry() = default; 
    virtual std::string Ping(const ModelShell& shell, int verbosity) const = 0; 
};

template<class T> 
struct DiagCapability : Capability<IDiagnostic> { 
    void Run(const ModelShell& shell) const override { 
        std::cout << "[Diag] System check for: " << typeid(T).name() << " (ID: " << shell.GetTypeID() << ")\n"; 
    } 
};

template<class T> 
struct TeleCapability : Capability<ITelemetry> { 
    std::string Ping(const ModelShell& shell, int verbosity) const override { 
        if (verbosity > 1) return "[Tele] Detailed Ping: ALL SYSTEMS NOMINAL";
        return "[Tele] Basic Ping: OK";
    } 
};

struct Drone {};
struct Scout {};

// One line binds an entire domain
using AirModels = TypeList<Drone, Scout>;
static const CapabilityBinding<AirModels, DiagCapability, TeleCapability> g_AirBinding{};

// =============================================================================
// MAIN
// =============================================================================
int main() {
    std::cout << "--- CRG STAGE 6: FUSION + MODEL ROUTER (FINAL) ---\n\n";

    ModelShell droneShell;
    droneShell.Set(Drone{});

    std::cout << "[ OOP PATH: Via ModelShell (Invoke) ]\n";
    droneShell.Invoke<&IDiagnostic::Run>();
    std::cout << droneShell.Invoke<&ITelemetry::Ping>(2) << "\n\n";

    std::cout << "[ OOP PATH: Via ModelShell (TryInvoke Graceful Fail) ]\n";
    ModelShell emptyShell;
    emptyShell.TryInvoke<&IDiagnostic::Run>(); // Silent fail
    auto pingOpt = emptyShell.TryInvoke<&ITelemetry::Ping>(1);
    if (!pingOpt.has_value()) {
        std::cout << "-> Ping gracefully failed on empty shell.\n\n";
    }

    std::cout << "[ ECS PATH: Via CapabilityRouter directly ]\n";
    ModelTypeID scoutID = TypeIDOf<Scout>::Get();
    ModelShell dummyScoutShell; // Mimics context inside ECS loop
    dummyScoutShell.Set(Scout{});

    if (auto* d = CapabilityRouter::Find<IDiagnostic>(scoutID)) {
        d->Run(dummyScoutShell);
    }
    
    if (auto* t = CapabilityRouter::Find<ITelemetry>(scoutID)) {
        std::cout << t->Ping(dummyScoutShell, 1) << "\n";
    }

    return 0;
}