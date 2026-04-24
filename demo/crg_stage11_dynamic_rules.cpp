// Copyright (c) 2024-2026 Cyril Tissier. All rights reserved.
// Licensed under the Apache License, Version 2.0.
//
// ======================================================
// STAGE 11 — DYNAMIC RULES (COLD PATH EXCEPTIONS)
// ======================================================
//
// @intent:
// Introduce a flexible, linked-list based rule system to handle "Cold Path" 
// logic and dynamic exceptions without bloating the O(1) Hot Path tensor.
//
// @what_changed:
// - Implementation of IRule and RuleNode for auto-registered linked lists.
// - Coexistence of the Baked Tensor (Hot Path) and the Rule-Chain (Cold Path).
// - Registry-Free injection of behavioral exceptions based on World Context.
// - Unified System Update: Sequentially executes projected capabilities 
//   then evaluates dynamic rules.
//
// @key_insight:
// Not all logic belongs in the N-Dimensional tensor. Rules handle the 
// "Sparse Logic" that depends on unpredictable external factors (Time, Events), 
// keeping the core tensor dense and efficient.
// ======================================================

#include <iostream>
#include <vector>
#include <string>
#include <tuple>
#include <typeinfo>
#include <unordered_map>

// ======================================================
// [ ENGINE CORE ] 1. IDENTITY & SEMANTICS 
// ======================================================
using TypeID = std::size_t;
template<class T> struct TypeIDOf { static TypeID Get() { return typeid(T).hash_code(); } };
template<typename... Ts> struct TypeList {};

template<auto V> using When = std::integral_constant<decltype(V), V>;
template<auto V> using In   = std::integral_constant<decltype(V), V>;

// ======================================================
// [ ENGINE CORE ] 2. TENSOR MATH (From Stage 10) 
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
// [ ENGINE CORE ] 3. THE TENSOR ROUTER (Hot Path) 
// ======================================================
template<class InterfaceT>
class BehaviorRouter {
    struct DiscoveryNode { TypeID modelID; const InterfaceT* const* tensor; };
    static inline std::vector<DiscoveryNode> s_discovery_list;

    struct BakedRouter {
        std::vector<TypeID> ids;
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
    static void Register(TypeID modelID, const InterfaceT* const* tensor) {
        for (const auto& n : s_discovery_list) if (n.modelID == modelID) return;
        s_discovery_list.push_back({modelID, tensor});
    }

    template<class... Args>
    static const InterfaceT* Find(TypeID modelID, Args... args) {
        const BakedRouter& router = GetBaked();
        std::size_t offset = InterfaceT::Context::ComputeOffset(args...); 
        for (std::size_t i = 0; i < router.ids.size(); ++i) {             
            if (router.ids[i] == modelID) return router.tensors[i][offset];                         
        }
        return nullptr;
    }
};

// ======================================================
// [ ENGINE CORE ] 4. DYNAMIC RULES (Cold Path) 
// ======================================================
struct WorldContext { float time_of_day; };

template<typename SettingsT, typename... Args>
struct IRule {
    virtual ~IRule() = default;
    virtual bool Eval(const WorldContext& w, const SettingsT& s, Args&... args) const = 0;
    virtual const IRule* GetNext() const = 0;
};

template<typename SettingsT, typename... Args>
class RuleRegistry {
    using HeadMap = std::unordered_map<TypeID, const IRule<SettingsT, Args...>*>;
    static HeadMap& GetMap() { static HeadMap s_instance; return s_instance; }
public:
    static void Register(TypeID modelID, const IRule<SettingsT, Args...>* node) { GetMap()[modelID] = node; }
    static const IRule<SettingsT, Args...>* GetHead(TypeID modelID) {
        auto& map = GetMap();
        auto it = map.find(modelID);
        return (it != map.end()) ? it->second : nullptr;
    }
};

template<typename Derived, typename SettingsT, typename... Args>
class RuleNode : public IRule<SettingsT, Args...> {
    const IRule<SettingsT, Args...>* m_next = nullptr;
public:
    RuleNode(TypeID modelID) {
        m_next = RuleRegistry<SettingsT, Args...>::GetHead(modelID);
        RuleRegistry<SettingsT, Args...>::Register(modelID, this);
    }
    const IRule<SettingsT, Args...>* GetNext() const override { return m_next; }
};

// ======================================================
// [ ENGINE CORE ] 5. BAKING INFRASTRUCTURE (From Stage 10) 
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
            std::size_t c_val = (offset / Context::template Stride<I>()) % Context::template DimSize<I>();
            EnumType runtime_val = static_cast<EnumType>(c_val + (EnumTraits<EnumType>::HasAny ? 1 : 0));
            return (EnumTraits<EnumType>::HasAny && (static_cast<std::size_t>(constraint_val) == 0) || (constraint_val == runtime_val)) && MatchIndexImpl<I + 1>(offset);
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
    template<class InterfaceT> const InterfaceT* const* GetTensorBuffer() const {
        static TensorBaker<InterfaceT> baker(this);
        return baker.buffer.data();
    }
public:
    BakedCapabilityNode() {
        (BehaviorRouter<typename Behaviors<ModelT>::InterfaceType>::Register(TypeIDOf<ModelT>::Get(), GetTensorBuffer<typename Behaviors<ModelT>::InterfaceType>()), ...);
    }
};

template<class ModelList, template<class> class... Behaviors> struct DomainBaker;
template<class... Models, template<class> class... Behaviors>
struct DomainBaker<TypeList<Models...>, Behaviors...> {
    std::tuple<BakedCapabilityNode<Models, Behaviors...>...> m_nodes;
};

// ======================================================
// [ GAMEPLAY SPACE ] 6. DATA & ENUMS
// ======================================================
enum class State { Any, Init, Combat };
enum class Zone  { Any, Desert, Tundra };
template<> struct EnumTraits<State> { static constexpr std::size_t Count = 3; static constexpr bool HasAny = true; };
template<> struct EnumTraits<Zone>  { static constexpr std::size_t Count = 3; static constexpr bool HasAny = true; };

struct Battery { float charge = 100.0f; };
struct ScoutSettings { float night_start = 18.0f; float drain_rate = 2.0f; };
struct Scout {};

// ======================================================
// [ GAMEPLAY SPACE ] 7. HOT PATH BEHAVIORS (Tensor) 
// ======================================================
struct IEnergyDrain {
    using InterfaceType = IEnergyDrain;
    using Context = Selector<State, Zone>; 
    virtual void ExecuteDrain(Battery& bat) const = 0;
};

template<class T> struct GenericDrainDef : CapabilityDefinition<IEnergyDrain, T, When<State::Any>, In<Zone::Any>> { 
    void ExecuteDrain(Battery& bat) const override { bat.charge -= 0.5f; }
};

// Enregistrement via DomainBaker
using SimModels = TypeList<Scout>;
static const DomainBaker<SimModels, GenericDrainDef> g_BehaviorModule{};

// ======================================================
// [ GAMEPLAY SPACE ] 8. COLD PATH RULES (Dynamic) 
// ======================================================
struct ScoutNightDrainRule : RuleNode<ScoutNightDrainRule, ScoutSettings, Battery> {
    ScoutNightDrainRule() : RuleNode(TypeIDOf<Scout>::Get()) {}
    bool Eval(const WorldContext& w, const ScoutSettings& s, Battery& b) const override {
        if (w.time_of_day >= s.night_start) {
            b.charge -= s.drain_rate;
            std::cout << " [CRG Rule] Night drain applied!\n";
            return true;
        }
        return false;
    }
};
static ScoutNightDrainRule g_scout_night_rule;

// ======================================================
// [ THE SYSTEM ] UNIFIED UPDATE
// ======================================================
void UnifiedUpdate(TypeID modelID, Battery& bat, ScoutSettings& settings, WorldContext& world, State s, Zone z) {
    // 1. HOT PATH: Tenseur O(1) 
    if (auto* action = BehaviorRouter<IEnergyDrain>::Find(modelID, s, z)) {
        action->ExecuteDrain(bat);
    }
    // 2. COLD PATH: Rules-Chain 
    for (auto* rule = RuleRegistry<ScoutSettings, Battery>::GetHead(modelID); rule; rule = rule->GetNext()) {
        rule->Eval(world, settings, bat);
    }
}

int main() {
    WorldContext world { 20.0f }; // Il est 20h
    Battery bat;
    ScoutSettings settings { 18.0f, 5.0f };

    std::cout << "--- STAGE 11: SYMBIOSIS OF BAKED TENSORS & DYNAMIC RULES ---\n";
    std::cout << "Initial Battery: " << bat.charge << "%\n\n";

    UnifiedUpdate(TypeIDOf<Scout>::Get(), bat, settings, world, State::Combat, Zone::Desert);

    std::cout << "\nFinal Battery: " << bat.charge << "%\n";
    return 0;
}