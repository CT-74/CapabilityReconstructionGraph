// Copyright (c) 2024-2026 Cyril Tissier. All rights reserved.
// Licensed under the Apache License, Version 2.0.
// See LICENSE file in the project root for full license information.
//
// ======================================================
// STAGE 09 — THE N-DIMENSIONAL HYPERGRAPH (INTERFACE-TEMPLATED ROUTER)
// ======================================================
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
// ======================================================

#include <iostream>
#include <typeinfo>
#include <string>
#include <cstddef>
#include <new>
#include <tuple>
#include <vector>

// ======================================================
// [ ENGINE CORE ] 1. IDENTITY & SEMANTICS
// ======================================================
using TypeID = std::size_t;
template<class T> struct TypeIDOf { static TypeID Get() { return typeid(T).hash_code(); } };
template<typename... Ts> struct TypeList {};

template<auto V> using When = std::integral_constant<decltype(V), V>;
template<auto V> using In   = std::integral_constant<decltype(V), V>;
template<auto V> using Auth = std::integral_constant<decltype(V), V>;

// ======================================================
// [ ENGINE CORE ] 2. DIMENSION TRAITS & THE SELECTOR
// ======================================================
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

// ======================================================
// [ ENGINE CORE ] 3. TYPE ERASED SHELL (Unchanged)
// ======================================================
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
        Reset(); new (&m_buffer) Model<T>(std::move(v)); m_set = true;
    }
    TypeID GetTypeID() const { return m_set ? reinterpret_cast<const Concept*>(&m_buffer)->GetTypeID() : 0; }
    template<class T> const T& Get() const { return static_cast<const Model<T>*>(reinterpret_cast<const Concept*>(&m_buffer))->value; }
private:
    void Reset() { if(m_set) { reinterpret_cast<Concept*>(&m_buffer)->~Concept(); m_set = false; } }
    alignas(std::max_align_t) std::byte m_buffer[SBO_SIZE]; bool m_set = false;
};


// ======================================================
// [ ENGINE CORE ] 4. THE INTERFACE-TEMPLATED ROUTER
// ======================================================
template<class InterfaceT>
class BehaviorRouter {
    // Cold Path Discovery
    struct DiscoveryNode { TypeID modelID; const InterfaceT* const* tensor; };
    static inline std::vector<DiscoveryNode> s_discovery_list;

    // Hot Path Baked SoA
    struct BakedRouter {
        std::vector<TypeID> ids;
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
    static void Register(TypeID modelID, const InterfaceT* const* tensor) {
        // Prevent duplicate registration if multiple behaviors share the same interface
        for (const auto& n : s_discovery_list) {
            if (n.modelID == modelID) return;
        }
        s_discovery_list.push_back({modelID, tensor});
    }

    // THE PURE O(1) HOT PATH API
    template<class... Args>
    static const InterfaceT* Find(TypeID modelID, Args... args) {
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


// ======================================================
// [ ENGINE CORE ] 5. THE GENERIC CAPABILITY BASE (Unchanged)
// ======================================================
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


// ======================================================
// [ ENGINE CORE ] 6. THE TENSOR BAKER
// ======================================================
template<class ModelT, template<class> class... Behaviors>
class BakedCapabilityNode : public Behaviors<ModelT>... { // No more virtual base class!
    
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


// ======================================================
// [ GAMEPLAY SPACE ] GPP DEFINITIONS (Unchanged)
// ======================================================

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
    virtual void Execute(const HardwareHandle&) const = 0;
};

template<class T, class T_E = When<State::Any>, class T_B = In<Zone::Any>, class T_R = Auth<Security::Any>>
using ActionDefBase = CapabilityDefinition<IExecuteAction, T, T_E, T_B, T_R>;

template<class T> 
struct GenericDef : ActionDefBase<T> { 
    void Execute(const HardwareHandle& t) const override { std::cout << "[GENERIC] " << t.template Get<T>().name << " performs standard action.\n"; }
};

template<class T> 
struct SandstormDef : ActionDefBase<T, When<State::Combat>, In<Zone::Desert>, Auth<Security::Server>> {
    void Execute(const HardwareHandle& t) const override { std::cout << "[SERVER-ONLY] 🔥 " << t.template Get<T>().name << " engages DUST SHIELD!\n"; }
};

using SimModels = TypeList<Drone, HeavyLifter>;
static const DomainBaker<SimModels, GenericDef> g_GenericModule{};
static const DomainBaker<TypeList<HeavyLifter>, SandstormDef> g_SandstormModule{};


// ======================================================
// USER CODE (ECS Hot Path Simulation)
// ======================================================
int main() {
    HardwareHandle lifter; lifter.Set(HeavyLifter{"Loader-X"});
    HardwareHandle drone;  drone.Set(Drone{"Scout-1"});

    std::cout << "--- STAGE 09: THE N-DIMENSIONAL HYPERGRAPH (DE-VIRTUALIZED) ---\n\n";

    TypeID id_lifter = lifter.GetTypeID();
    TypeID id_drone = drone.GetTypeID();

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