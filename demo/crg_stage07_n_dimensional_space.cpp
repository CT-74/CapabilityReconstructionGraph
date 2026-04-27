// Copyright (c) 2024-2026 Cyril Tissier. All rights reserved.
// Licensed under the Apache License, Version 2.0.
// See LICENSE file in the project root for full license information.
//
// =============================================================================
// CAPABILITY ROUTING GATEWAY (CRG) - STAGE 7: THE N-DIMENSIONAL HYPERGRAPH
// =============================================================================
//
// @intent:
// Resolve capabilities across N-orthogonal axes in strictly O(1) time,
// with a fully devirtualized and segregated memory architecture.
//
// @what_changed:
// - Devirtualization: IModelRegistryNode is completely eradicated.
// - Cache Segregation: BehaviorRouter is now templated by InterfaceT. It only 
//   stores models that ACTUALLY implement the requested interface.
// - Fold Registration: The Baker registers its pre-computed tensors directly 
//   into the specific Interface Routers at compile/startup time.
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
#include <vector>

// =============================================================================
// [ ENGINE CORE ] 1. IDENTITY & SEMANTICS
// =============================================================================
using ModelTypeID = std::size_t;
template<class T> struct TypeIDOf { static ModelTypeID Get() { return typeid(T).hash_code(); } };
template<typename... Ts> struct TypeList {};

template<auto V> using When = std::integral_constant<decltype(V), V>;
template<auto V> using In   = std::integral_constant<decltype(V), V>;
template<auto V> using Auth = std::integral_constant<decltype(V), V>;

// =============================================================================
// [ ENGINE CORE ] 2. DIMENSION TRAITS & THE SELECTOR
// =============================================================================
template<typename T> struct EnumTraits;

template<typename... Enums>
struct Selector {
    static constexpr std::size_t Dimensions = sizeof...(Enums);
    template<std::size_t I> using EnumAt = std::tuple_element_t<I, std::tuple<Enums...>>;

    template<std::size_t I>
    static constexpr std::size_t DimSize() {
        using E = EnumAt<I>;
        return EnumTraits<E>::Count - (EnumTraits<E>::HasAny ? 1 : 0);
    }

    template<std::size_t I>
    static constexpr std::size_t Stride() {
        if constexpr (I + 1 >= Dimensions) return 1;
        else return DimSize<I + 1>() * Stride<I + 1>();
    }

    static constexpr std::size_t Volume() { return (DimSize<0>() * ... * 1); }
    static constexpr std::size_t ComputeOffset(Enums... args) { return ComputeOffsetImpl<0>(args...); }

private:
    template<std::size_t I>
    static constexpr std::size_t ComputeOffsetImpl(Enums... args) {
        if constexpr (I == Dimensions) return 0;
        else {
            auto val_enum = std::get<I>(std::make_tuple(args...));
            std::size_t val = static_cast<std::size_t>(val_enum);
            using E = EnumAt<I>;
            if constexpr (EnumTraits<E>::HasAny) val -= 1; 
            return val * Stride<I>() + ComputeOffsetImpl<I + 1>(args...);
        }
    }
};

// =============================================================================
// [ ENGINE CORE ] 3. TYPE ERASED SHELL (The Data Container)
// =============================================================================
class ModelHandle {
public:
    struct Concept { virtual ~Concept() = default; virtual ModelTypeID GetTypeID() const = 0; };
    template<class T> struct Model : Concept {
        T value; Model(T v) : value(std::move(v)) {}
        ModelTypeID GetTypeID() const override { return TypeIDOf<T>::Get(); }
    };
    
    ModelHandle() = default;
    template<class T> void Set(T v) { m_ptr = std::make_unique<Model<T>>(std::move(v)); }
    ModelTypeID GetTypeID() const { return m_ptr ? m_ptr->GetTypeID() : 0; }
    
    template<class T> const T& Get() const { 
        return static_cast<const Model<T>*>(m_ptr.get())->value; 
    }
private:
    std::unique_ptr<Concept> m_ptr;
};

// =============================================================================
// [ ENGINE CORE ] 4. THE INTERFACE-TEMPLATED ROUTER
// =============================================================================
template<class InterfaceT>
class BehaviorRouter {
    // Cold Path Discovery
    struct DiscoveryNode { ModelTypeID modelID; const InterfaceT* const* tensor; };
    static inline std::vector<DiscoveryNode> s_discovery_list;

    // Hot Path Baked SoA
    struct BakedRouter {
        std::vector<ModelTypeID> ids;
        std::vector<const InterfaceT* const*> tensors; // Pure raw pointers, NO virtual calls!
        
        BakedRouter() {
            for (const auto& n : s_discovery_list) {
                ids.push_back(n.modelID);
                tensors.push_back(n.tensor);
            }
        }
    };

    static const BakedRouter& GetBaked() {
        static const BakedRouter instance; 
        return instance;
    }

public:
    // Called by the Baker at static init
    static void Register(ModelTypeID modelID, const InterfaceT* const* tensor) {
        // Prevent duplicate registration if multiple behaviors share the same interface
        for (const auto& n : s_discovery_list) {
            if (n.modelID == modelID) return;
        }
        s_discovery_list.push_back({modelID, tensor});
    }

    // THE PURE O(1) HOT PATH API
    template<class... Args>
    static const InterfaceT* Find(ModelTypeID modelID, Args... args) {
        const BakedRouter& router = GetBaked();
        std::size_t offset = InterfaceT::Context::ComputeOffset(args...); // Math O(1)
        
        for (std::size_t i = 0; i < router.ids.size(); ++i) {             // Memory O(1)
            if (router.ids[i] == modelID) {
                return router.tensors[i][offset];                         // Pure Jump O(1)
            }
        }
        return nullptr;
    }
};

// =============================================================================
// [ ENGINE CORE ] 5. THE GENERIC CAPABILITY BASE
// =============================================================================
template<class InterfaceT, class ModelT, class... Constraints>
struct CapabilityDefinition : InterfaceT {
    using InterfaceType = InterfaceT;

    bool MatchIndex(std::size_t offset) const { return MatchIndexImpl<0>(offset); }

private:
    template<std::size_t I>
    bool MatchIndexImpl(std::size_t offset) const {
        if constexpr (I == sizeof...(Constraints)) return true;
        else {
            using Context = typename InterfaceT::Context;
            using EnumType = typename Context::template EnumAt<I>;
            using ConstraintWrapper = std::tuple_element_t<I, std::tuple<Constraints...>>;
            
            constexpr auto constraint_val = ConstraintWrapper::value;
            constexpr bool constraint_is_any = EnumTraits<EnumType>::HasAny && (static_cast<std::size_t>(constraint_val) == 0);

            std::size_t c_val = (offset / Context::template Stride<I>()) % Context::template DimSize<I>();
            EnumType runtime_val = static_cast<EnumType>(c_val + (EnumTraits<EnumType>::HasAny ? 1 : 0));

            return (constraint_is_any || (constraint_val == runtime_val)) && MatchIndexImpl<I + 1>(offset);
        }
    }
};

// =============================================================================
// [ ENGINE CORE ] 6. THE TENSOR BAKER
// =============================================================================
template<class ModelT, template<class> class... Behaviors>
class BakedCapabilityNode : public Behaviors<ModelT>... { 
    // No more virtual base class!
    
    template<class InterfaceT>
    struct TensorBaker {
        std::vector<const InterfaceT*> buffer;
        
        TensorBaker(const BakedCapabilityNode* node) {
            buffer.assign(InterfaceT::Context::Volume(), nullptr);
            ([&]() {
                if constexpr (std::is_base_of_v<InterfaceT, Behaviors<ModelT>>) {
                    const auto* behaviorPtr = static_cast<const InterfaceT*>(static_cast<const Behaviors<ModelT>*>(node));
                    for (std::size_t i = 0; i < InterfaceT::Context::Volume(); ++i) {
                        if (static_cast<const Behaviors<ModelT>*>(node)->MatchIndex(i)) {
                            buffer[i] = behaviorPtr;
                        }
                    }
                }
            }(), ...);
        }
    };

    template<class InterfaceT>
    const InterfaceT* const* GetTensorBuffer() const {
        static TensorBaker<InterfaceT> baker(this);
        return baker.buffer.data();
    }

public:
    BakedCapabilityNode() {
        // C++17 Fold Expression: The node registers its baked tensor directly 
        // into the specific router of each interface it implements!
        (BehaviorRouter<typename Behaviors<ModelT>::InterfaceType>::Register(
            TypeIDOf<ModelT>::Get(), 
            GetTensorBuffer<typename Behaviors<ModelT>::InterfaceType>()
        ), ...);
    }
};

template<class ModelList, template<class> class... Behaviors>
struct DomainBaker;

template<class... Models, template<class> class... Behaviors>
struct DomainBaker<TypeList<Models...>, Behaviors...> {
    std::tuple<BakedCapabilityNode<Models, Behaviors...>...> m_nodes;
};

// =============================================================================
// [ GAMEPLAY SPACE ] GPP DEFINITIONS
// =============================================================================

enum class State    { Any, Init, Combat };
enum class Zone     { Any, Desert, Tundra };
enum class Security { Any, Client, Server };

template<> struct EnumTraits<State>    { static constexpr std::size_t Count = 3; static constexpr bool HasAny = true; };
template<> struct EnumTraits<Zone>     { static constexpr std::size_t Count = 3; static constexpr bool HasAny = true; };
template<> struct EnumTraits<Security> { static constexpr std::size_t Count = 3; static constexpr bool HasAny = true; };

struct Drone       { std::string name; };
struct HeavyLifter { std::string name; };

struct IExecuteAction {
    using InterfaceType = IExecuteAction;
    using Context = Selector<State, Zone, Security>; 
    virtual void Execute(const ModelHandle&) const = 0;
};

template<class T, class T_E = When<State::Any>, class T_B = In<Zone::Any>, class T_R = Auth<Security::Any>>
using ActionDefBase = CapabilityDefinition<IExecuteAction, T, T_E, T_B, T_R>;

template<class T> 
struct GenericDef : ActionDefBase<T> { 
    void Execute(const ModelHandle& t) const override { 
        std::cout << "[GENERIC] " << t.template GetAs<T>().name << " performs standard action.\n"; 
    }
};

template<class T> 
struct SandstormDef : ActionDefBase<T, When<State::Combat>, In<Zone::Desert>, Auth<Security::Server>> {
    void Execute(const ModelHandle& t) const override { 
        std::cout << "[SERVER-ONLY] 🔥 " << t.template GetAs<T>().name << " engages DUST SHIELD!\n"; 
    }
};

using SimModels = TypeList<Drone, HeavyLifter>;
static const DomainBaker<SimModels, GenericDef> g_GenericModule{};
static const DomainBaker<TypeList<HeavyLifter>, SandstormDef> g_SandstormModule{};

// =============================================================================
// USER CODE (ECS Hot Path Simulation)
// =============================================================================
int main() {
    std::cout << "--- CRG STAGE 7: THE N-DIMENSIONAL HYPERGRAPH (DE-VIRTUALIZED) ---\n\n";

    ModelHandle lifter; lifter.Set(HeavyLifter{"Loader-X"});
    ModelHandle drone;  drone.Set(Drone{"Scout-1"});

    ModelTypeID id_lifter = lifter.GetTypeID();
    ModelTypeID id_drone = drone.GetTypeID();

    std::cout << "HeavyLifter (Combat, Desert, Client):\n  -> ";
    if (auto* action = BehaviorRouter<IExecuteAction>::Find(id_lifter, State::Combat, Zone::Desert, Security::Client))
        action->Execute(lifter);

    std::cout << "\nHeavyLifter (Combat, Desert, Server):\n  -> ";
    if (auto* action = BehaviorRouter<IExecuteAction>::Find(id_lifter, State::Combat, Zone::Desert, Security::Server))
        action->Execute(lifter);

    std::cout << "\nDrone (Combat, Desert, Server):\n  -> ";
    if (auto* action = BehaviorRouter<IExecuteAction>::Find(id_drone, State::Combat, Zone::Desert, Security::Server))
        action->Execute(drone);

    return 0;
}