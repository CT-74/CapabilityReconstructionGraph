// Copyright (c) 2024-2026 Cyril Tissier. All rights reserved.
// Licensed under the Apache License, Version 2.0.
//
#include <iostream>
#include <typeinfo>
#include <unordered_map>
#include <vector>
#include <string>

// ======================================================
// [ ENGINE CORE ] 1. IDENTITY & WORLD CONTEXT
// ======================================================
using TypeID = std::size_t;
template<class T> struct TypeIDOf { static TypeID Get() { return typeid(T).hash_code(); } };

struct WorldContext { 
    float time_of_day; 
};

// ======================================================
// [ ENGINE CORE ] 2. TYPE-SAFE RULE INTERFACE
// ======================================================
// L'interface définit la signature exacte des composants ECS mutés (Args...) 
// et du bloc de réglages (SettingsT) associé.
template<typename SettingsT, typename... Args>
struct IRule {
    virtual ~IRule() = default;
    virtual bool Eval(const WorldContext& w, const SettingsT& s, Args&... args) const = 0;
    virtual const IRule* GetNext() const = 0;
};

// ======================================================
// [ ENGINE CORE ] 3. THE AUTO-REGISTRY (Lazy Magic Static)
// ======================================================
template<typename SettingsT, typename... Args>
class RuleRegistry {
    // La Map est initialisée au premier accès (Lazy).
    using HeadMap = std::unordered_map<TypeID, const IRule<SettingsT, Args...>*>;
    
    static HeadMap& GetMap() {
        static HeadMap s_instance; 
        return s_instance;
    }

public:
    static void Register(TypeID modelID, const IRule<SettingsT, Args...>* node) {
        GetMap()[modelID] = node;
    }

    // Retourne la tête de la liste chaînée pour un modèle donné.
    static const IRule<SettingsT, Args...>* GetHead(TypeID modelID) {
        auto& map = GetMap();
        auto it = map.find(modelID);
        return (it != map.end()) ? it->second : nullptr;
    }
};

// ======================================================
// [ ENGINE CORE ] 4. THE CRTP RULE NODE (LinkedNode)
// ======================================================
// Chaque instance de règle s'insère d'elle-même dans la chaîne du registre.
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
// [ GAMEPLAY SPACE ] 5. DECLARATIVE RULES (Plug & Play)
// ======================================================
struct Battery       { float charge = 100.0f; };
struct ScoutSettings { float night_start = 18.0f; float drain_rate = 2.0f; };
struct Scout {};

// Règle 1 : Drain nocturne du Scout (Auto-enregistrée via statique)
struct ScoutNightDrainRule : RuleNode<ScoutNightDrainRule, ScoutSettings, Battery> {
    ScoutNightDrainRule() : RuleNode(TypeIDOf<Scout>::Get()) {}

    bool Eval(const WorldContext& w, const ScoutSettings& s, Battery& b) const override {
        if (w.time_of_day >= s.night_start) {
            b.charge -= s.drain_rate;
            std::cout << " [CRG Rule] Scout night drain: -" << s.drain_rate << "%\n";
            return true;
        }
        return false;
    }
};

// L'instanciation physique de la règle suffit à l'activer dans le moteur.
static ScoutNightDrainRule g_scout_night_rule;

// ======================================================
// [ THE SYSTEM ] 6. UNIFIED ECS UPDATE (Hot Path)
// ======================================================
struct RuleSystem {
    void Update(const WorldContext& world, TypeID modelID, Battery& bat, ScoutSettings& settings) {
        
        // Traversée de la Rule-Chain spécifique au modèle.
        // On n'évalue que les exceptions dynamiques (O(R)).
        for (auto* rule = RuleRegistry<ScoutSettings, Battery>::GetHead(modelID); 
             rule != nullptr; 
             rule = rule->GetNext()) 
        {
            rule->Eval(world, settings, bat);
        }
    }
};

// ======================================================
// ENTRY POINT
// ======================================================
int main() {
    WorldContext world { 20.0f }; // 20h00
    Battery bat;
    ScoutSettings settings { 18.0f, 5.0f }; // La nuit commence à 18h

    std::cout << "--- STAGE 11: ULTIMATE AUTO-REGISTERED RULES ---\n";
    std::cout << "Initial Battery: " << bat.charge << "%\n\n";

    RuleSystem system;
    // Plus besoin d'appeler de fonction 'Init' ou 'Register'.
    system.Update(world, TypeIDOf<Scout>::Get(), bat, settings);

    std::cout << "\nFinal Battery: " << bat.charge << "%\n";
    return 0;
}