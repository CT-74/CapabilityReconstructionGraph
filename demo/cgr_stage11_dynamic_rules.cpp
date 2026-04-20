// Copyright (c) 2024-2026 Cyril Tissier. All rights reserved.
//
// ======================================================
// BONUS STAGE 11 — THE HYBRID RULE ENGINE (O(1) Matrix + Dynamic Predicates)
// ======================================================
//
// @intent:
// Bridge the deterministic O(1) routing of the Behavior Matrix with the 
// dynamic flexibility of a Rule Engine.
//
// @what_changed:
// - Restored the IDCard and the dual-parameter CRTP for O(1) routing.
// - The "Behavior" definition now accepts lambdas (Predicates & Actions) at runtime.
// - The Matrix resolves the Type in O(1), then iterates only the local rules.
//
// @key_insight:
// The Primary Axis (Identity) is resolved by the compiler in O(1). 
// The Secondary Axes (Context) are evaluated at runtime by injected predicates.
// We avoid the O(N) trap of flat rule engines.
// ======================================================

#include <iostream>
#include <vector>
#include <typeinfo>

// ======================================================
// 1. PURE ECS DATA
// ======================================================
using TypeHash = std::size_t;
template<class T> struct TypeIDOf { static TypeHash Get() { return typeid(T).hash_code(); } };

struct Battery   { float charge = 100.0f; };
struct Equipment { bool nv_toggled = true; };
struct Biome     { uint32_t current_id; };
struct IDCard    { TypeHash logic_class; }; 
struct WorldState{ float time_of_day = 12.0f; };

// ======================================================
// 2. CRG INFRASTRUCTURE (O(1) Isolated Heads via CRTP)
// ======================================================
template<class Derived, class Interface>
class LinkedNode {
public:
    LinkedNode() {
        m_next = s_head;
        s_head = static_cast<const Interface*>(static_cast<const Derived*>(this));
    }
    const Interface* GetNext() const { return m_next; }
    static const Interface* GetHead() { return s_head; }
private:
    inline static const Interface* s_head = nullptr;
    const Interface* m_next = nullptr;
};

// ======================================================
// 3. THE RULE INTERFACE
// ======================================================
struct INightVisionRule {
    struct Selector {
        float time_of_day;
        uint32_t biome_id;
    };
    
    virtual bool Match(TypeHash logic_class, const Selector& sel) const = 0;
    virtual void Execute(Battery& bat, Equipment& eq) const = 0;
};

// ======================================================
// 4. THE DYNAMIC RULE NODE (Data-Driven Template)
// ======================================================
// Ce template crée une tête de liste UNIQUE par Modèle (Scout, Lifter),
// mais permet d'y injecter des Lambdas pour les règles !
template<class T>
struct NightRule : INightVisionRule, LinkedNode<NightRule<T>, INightVisionRule> {
    
    using Predicate = bool (*)(const Selector&);
    using Action = void (*)(Battery&, Equipment&);
    
    Predicate condition;
    Action action;

    NightRule(Predicate c, Action a) : condition(c), action(a) {}

    bool Match(TypeHash c, const Selector& sel) const override { 
        // Vérification de sécurité (Type) + Évaluation Dynamique (Prédicat)
        return c == TypeIDOf<T>::Get() && condition(sel); 
    }
    
    void Execute(Battery& bat, Equipment& eq) const override {
        action(bat, eq);
    }
};

// ======================================================
// 5. REGISTRATION (The Game Design Sandbox)
// ======================================================
struct Scout {};
struct HeavyLifter {};

constexpr uint32_t BIOME_CITY = 0;
constexpr uint32_t BIOME_ARCTIC = 1;

// Règle 1 : Scout en Ville (Nuit de 21h à 6h)
inline static const NightRule<Scout> rule_scout_city(
    [](const INightVisionRule::Selector& s) { return s.biome_id == BIOME_CITY && (s.time_of_day >= 21.0f || s.time_of_day <= 6.0f); },
    [](Battery& b, Equipment& e) { if (e.nv_toggled) b.charge -= 0.5f; }
);

// Règle 2 : Scout en Arctique (Nuit à partir de 15h, Froid = gros drain)
inline static const NightRule<Scout> rule_scout_arctic(
    [](const INightVisionRule::Selector& s) { return s.biome_id == BIOME_ARCTIC && (s.time_of_day >= 15.0f || s.time_of_day <= 9.0f); },
    [](Battery& b, Equipment& e) { if (e.nv_toggled) b.charge -= 0.8f; } 
);

// Règle 3 : HeavyLifter (Nuit générique)
inline static const NightRule<HeavyLifter> rule_lifter_all(
    [](const INightVisionRule::Selector& s) { return s.time_of_day >= 20.0f || s.time_of_day <= 7.0f; },
    [](Battery& b, Equipment& e) { if (e.nv_toggled) b.charge -= 2.5f; } // Consomme énormément
);


// ======================================================
// 6. THE BEHAVIOR MATRIX (Two-Stage Dispatch)
// ======================================================
template<class...> struct TypeList;
template<class ModelT, template<class> class... Defs> struct BehaviorMatrixSingle : public Defs<ModelT>... {};
template<class ModelList, template<class> class... Defs> struct BehaviorMatrix;

template<class... Models, template<class> class... Defs>
struct BehaviorMatrix<TypeList<Models...>, Defs...> : public BehaviorMatrixSingle<Models, Defs...>... {
    
    template<class Interface>
    static const Interface* Resolve(TypeHash logic_class, const typename Interface::Selector& sel) {
        const Interface* result = nullptr;
        
        // STAGE 1 : ROUTING O(1) PAR IDENTITÉ (Fold Expression)
        ([&]() {
            if (logic_class == TypeIDOf<Models>::Get()) {
                ([&]() {
                    if constexpr (std::is_base_of_v<Interface, Defs<Models>>) {
                        
                        // STAGE 2 : ÉVALUATION DES RÈGLES O(R) 
                        // On itère UNIQUEMENT sur les règles spécifiques à ce modèle !
                        for (const Interface* n = Defs<Models>::GetHead(); n != nullptr; n = n->GetNext()) {
                            if (n->Match(logic_class, sel)) {
                                result = n;
                                break; // Règle trouvée, on sort
                            }
                        }
                    }
                }(), ...);
            }
        }(), ...);
        
        return result;
    }
};

using AIDomain = BehaviorMatrix<TypeList<Scout, HeavyLifter>, NightRule>;
inline static const AIDomain g_behavior_matrix{};

// ======================================================
// 7. THE ECS SYSTEM
// ======================================================
struct MockRegistry {
    WorldState world;
    std::vector<Battery> batteries;
    std::vector<Equipment> equipments;
    std::vector<IDCard> id_cards;
    std::vector<Biome> biomes;

    void Update() {
        for (size_t i = 0; i < id_cards.size(); ++i) {
            
            // Construction du Selector
            INightVisionRule::Selector selector { world.time_of_day, biomes[i].current_id };

            // Résolution Magique (O(1) Jump -> O(R) Predicates)
            if (const auto* rule = AIDomain::Resolve<INightVisionRule>(id_cards[i].logic_class, selector)) {
                rule->Execute(batteries[i], equipments[i]);
            }
        }
    }
};

// ======================================================
// ENTRY POINT
// ======================================================
int main() {
    MockRegistry reg;
    
    // Entité 0 : Scout en Arctique
    reg.id_cards.push_back({ TypeIDOf<Scout>::Get() });
    reg.batteries.push_back({ 100.0f });
    reg.equipments.push_back({ true });
    reg.biomes.push_back({ BIOME_ARCTIC });

    std::cout << "--- FRAME 1: Time is 16:00 (4 PM) ---\n";
    reg.world.time_of_day = 16.0f;
    reg.Update();
    
    std::cout << "Scout (Arctic) Battery: " << reg.batteries[0].charge << "% (Arctic night starts at 15:00!)\\n";

    return 0;
}