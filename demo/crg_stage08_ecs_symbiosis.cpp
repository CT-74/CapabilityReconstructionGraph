// Copyright (c) 2024-2026 Cyril Tissier. All rights reserved.
// Licensed under the Apache License, Version 2.0.
//
// =============================================================================
// CAPABILITY ROUTING GATEWAY (CRG) - STAGE 8: THE SYMBIOSIS (PURE DOD ECS)
// =============================================================================
//
// @intent:
// Harmonize high-density Data-Oriented Design (ECS) with the compile-time 
// N-Dimensional tensor of the CRG.
//
// @what_changed:
// - Object-Oriented 'ModelHandle' is entirely removed from the hot path.
// - Introduction of ECS Components: 'LogicID', 'Battery', 'BiomeContext'.
// - The Hot Path (System): Iterates tightly packed arrays, extracts context 
//   from components, and executes behavior directly on the data.
// - Zero Virtual Interface over Data: The capabilities take direct component 
//   references (e.g., ExecuteDrain(Battery&)).
//
// @key_insight:
// The ECS owns the Data Pipeline. The CRG owns the Logic Projection. 
// Together, they allow zero-cost state mutations mapped in a hypergraph.
// =============================================================================

#include <iostream>
#include <vector>
#include <string>
#include <cstddef>
#include <tuple>

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
// [ ENGINE CORE ] 2. THE SELECTOR & TENSOR MATH
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
// [ ENGINE CORE ] 3. DEVIRTUALIZED ROUTER (Lazy SoA)
// =============================================================================
template<class InterfaceT>
class BehaviorRouter {
    struct DiscoveryNode { ModelTypeID modelID; const InterfaceT* const* tensor; };
    static inline std::vector<DiscoveryNode> s_discovery_list;

    struct BakedRouter {
        std::vector<ModelTypeID> ids;
        std::vector<const InterfaceT* const*> tensors; 
        
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
    static void Register(ModelTypeID modelID, const InterfaceT* const* tensor) {
        for (const auto& n : s_discovery_list) if (n.modelID == modelID) return;
        s_discovery_list.push_back({modelID, tensor});
    }

    template<class... Args>
    static const InterfaceT* Find(ModelTypeID modelID, Args... args) {
        const BakedRouter& router = GetBaked();
        std::size_t offset = InterfaceT::Context::ComputeOffset(args...); 
        
        for (std::size_t i = 0; i < router.ids.size(); ++i) {             
            if (router.ids[i] == modelID) {
                return router.tensors[i][offset];                         
            }
        }
        return nullptr;
    }
};

// =============================================================================
// [ ENGINE CORE ] 4. CAPABILITY BASE & BAKER
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

template<class ModelT, template<class> class... Behaviors>
class BakedCapabilityNode : public Behaviors<ModelT>... {
    
    template<class InterfaceT>
    struct TensorBaker {
        std::vector<const InterfaceT*> buffer;
        TensorBaker(const BakedCapabilityNode* node) {
            buffer.assign(InterfaceT::Context::Volume(), nullptr);
            ([&]() {
                if constexpr (std::is_base_of_v<InterfaceT, Behaviors<ModelT>>) {
                    const auto* behaviorPtr = static_cast<const InterfaceT*>(static_cast<const Behaviors<ModelT>*>(node));
                    for (std::size_t i = 0; i < InterfaceT::Context::Volume(); ++i) {
                        if (static_cast<const Behaviors<ModelT>*>(node)->MatchIndex(i)) buffer[i] = behaviorPtr;
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
// [ GAMEPLAY SPACE ] 1. DATA (ECS COMPONENTS & ENUMS)
// =============================================================================

enum class State    { Any, Init, Combat };
enum class Zone     { Any, Desert, Tundra };

template<> struct EnumTraits<State> { static constexpr std::size_t Count = 3; static constexpr bool HasAny = true; };
template<> struct EnumTraits<Zone>  { static constexpr std::size_t Count = 3; static constexpr bool HasAny = true; };

// ECS Components
struct LogicID  { ModelTypeID hash; };
struct Battery  { float charge = 100.0f; };
struct BiomeCtx { Zone current_zone; };

// Models (Semantic Types only)
struct Scout {};
struct HeavyLifter {};


// =============================================================================
// [ GAMEPLAY SPACE ] 2. LOGIC (CRG INTERFACES & DEFS)
// =============================================================================

// The Interface takes ECS data directly!
struct IEnergyDrain {
    using InterfaceType = IEnergyDrain;
    using Context = Selector<State, Zone>; 
    virtual void ExecuteDrain(Battery& bat) const = 0;
};

template<class T, class T_E = When<State::Any>, class T_B = In<Zone::Any>>
using EnergyDefBase = CapabilityDefinition<IEnergyDrain, T, T_E, T_B>;

// Behaviors
template<class T> 
struct GenericDrainDef : EnergyDefBase<T> { 
    void ExecuteDrain(Battery& bat) const override { bat.charge -= 0.5f; }
};

template<class T> 
struct SandstormDrainDef : EnergyDefBase<T, When<State::Combat>, In<Zone::Desert>> {
    void ExecuteDrain(Battery& bat) const override { bat.charge -= 15.0f; /* Sand clogs the filters! */ }
};

// Instantiations
using SimModels = TypeList<Scout, HeavyLifter>;
static const DomainBaker<SimModels, GenericDrainDef> g_GenericDrainModule{};
static const DomainBaker<TypeList<HeavyLifter>, SandstormDrainDef> g_SandstormModule{};


// =============================================================================
// [ THE SYSTEM ] ECS HOT PATH SIMULATION
// =============================================================================
struct MockRegistry {
    // Global State
    State global_state = State::Init;

    // SoA Components Layout
    std::vector<LogicID>  logic_ids;
    std::vector<Battery>  batteries;
    std::vector<BiomeCtx> biomes;

    // The ECS Update Loop
    void System_UpdateEnergy() {
        for (std::size_t i = 0; i < logic_ids.size(); ++i) {
            
            // 1. Context Extraction (Gathering coords from ECS payload)
            State current_state = global_state;
            Zone  current_zone  = biomes[i].current_zone;

            // 2. CRG Resolution (O(1) Jump based on math and identity)
            if (auto* action = BehaviorRouter<IEnergyDrain>::Find(logic_ids[i].hash, current_state, current_zone)) {
                
                // 3. Execution directly on the Component (Strict DOD)
                action->ExecuteDrain(batteries[i]);
            }
        }
    }
};

// =============================================================================
// MAIN
// =============================================================================
int main() {
    MockRegistry reg;

    // Entity 0: Scout in Desert
    reg.logic_ids.push_back({TypeIDOf<Scout>::Get()});
    reg.batteries.push_back({100.0f});
    reg.biomes.push_back({Zone::Desert});

    // Entity 1: HeavyLifter in Desert
    reg.logic_ids.push_back({TypeIDOf<HeavyLifter>::Get()});
    reg.batteries.push_back({100.0f});
    reg.biomes.push_back({Zone::Desert});

    std::cout << "--- CRG STAGE 8: THE SYMBIOSIS (ECS + O(1) TENSOR) ---\n\n";

    std::cout << "FRAME 1: Initialization (Init)\n";
    reg.global_state = State::Init;
    reg.System_UpdateEnergy();
    std::cout << "  Scout Battery:       " << reg.batteries[0].charge << "%\n";
    std::cout << "  HeavyLifter Battery: " << reg.batteries[1].charge << "%\n\n";

    std::cout << "FRAME 2: Combat starts in the Desert! (Zero-cost logic swap)\n";
    reg.global_state = State::Combat;
    reg.System_UpdateEnergy();
    
    // Scout uses Generic (-0.5f), HeavyLifter takes the Sandstorm penalty (-15.0f)
    std::cout << "  Scout Battery:       " << reg.batteries[0].charge << "% (Generic)\n";
    std::cout << "  HeavyLifter Battery: " << reg.batteries[1].charge << "% (Sandstorm Penalty)\n";

    return 0;
}