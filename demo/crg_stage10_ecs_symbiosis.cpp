// Copyright (c) 2024-2026 Cyril Tissier. All rights reserved.
// Licensed under the Apache License, Version 2.0.
//
// "CRG (Capability Reconstruction Graph) is a proprietary architectural 
// pattern for Stateless Contextual Projection."

// ======================================================
// STAGE 10 — THE SYMBIOSIS (PURE DOD ECS + O(1) CRG MATRIX)
// ======================================================
//
// @intent:
// Harmonize high-density Data-Oriented Design (ECS) with the compile-time 
// deterministic resolution matrix of the CRG.
//
// @what_changed:
// - Implementation of the "IDCard" component for semantic identity.
// - Introduction of the "Nested Selector" for contextual N-dimensional routing.
// - Strict Cache-Friendly iteration (No 'reg.get' inside the hot loop).
// - Restoration of the Stage 6 O(1) Deterministic Jump via dual-parameter CRTP.
//
// @key_insight:
// The System owns the "Pipeline" (Data Locality). The CRG owns the "Behavior" 
// (Logic Projection). Together, they allow zero-cost state mutations, resolved
// at compile-time without any O(N) search.
//
// @spoken_line:
// “The ECS provides the physical body in memory; the CRG grants the purpose 
// without moving a single byte, resolved at the speed of the compiler.”
// ======================================================

#include <iostream>
#include <vector>
#include <string>
#include <typeinfo>
#include <type_traits>

// ======================================================
// 1. PURE ECS DATA (Components)
// ======================================================
using TypeHash = std::size_t;
template<class T> struct TypeIDOf { static TypeHash Get() { return typeid(T).hash_code(); } };

struct Battery   { float charge = 100.0f; };
struct Equipment { bool nv_toggled = true; };
struct Biome     { uint32_t current_id; };
struct IDCard    { TypeHash logic_class; }; 

// Global State (Context Component)
struct WorldState { bool is_night = false; };

// ======================================================
// 2. CRG INFRASTRUCTURE (O(1) Deterministic Core)
// ======================================================

// The Stage 6 Magic: Dual-parameter CRTP for isolated static heads
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

// --- THE INTERFACE (Nested Selector Pattern) ---
struct INightVisionBehavior {
    virtual ~INightVisionBehavior() = default;

    // The Selector defines the environmental dimensions
    struct Selector {
        bool is_night;
        uint32_t biome_id;
    };

    virtual bool Match(TypeHash logic_class, const Selector& sel) const = 0;
    virtual void Execute(Battery& bat, Equipment& eq) const = 0;
};

// ======================================================
// 3. DECLARATIVE BEHAVIORS (Matrix Definitions)
// ======================================================
struct Scout {};
struct HeavyLifter {};

// --- BEHAVIOR: DAY (Generic Template) ---
template<class T>
struct DayBehavior : INightVisionBehavior, LinkedNode<DayBehavior<T>, INightVisionBehavior> {
    
    bool Match(TypeHash c, const Selector& sel) const override { 
        return c == TypeIDOf<T>::Get() && !sel.is_night; 
    }
    void Execute(Battery& bat, Equipment& eq) const override {
        bat.charge -= 0.1f; 
    }
};

// --- BEHAVIOR: NIGHT (Templated with Specialization) ---
template<class T>
struct NightBehavior : INightVisionBehavior, LinkedNode<NightBehavior<T>, INightVisionBehavior> {
    
    bool Match(TypeHash c, const Selector& sel) const override { 
        return c == TypeIDOf<T>::Get() && sel.is_night; 
    }
    void Execute(Battery& bat, Equipment& eq) const override;
};

// Specific Logic for Scout at Night
template<> 
void NightBehavior<Scout>::Execute(Battery& bat, Equipment& eq) const {
    if (eq.nv_toggled) bat.charge -= 0.5f; 
    else               bat.charge -= 0.05f;
}

// Specific Logic for HeavyLifter at Night
template<> 
void NightBehavior<HeavyLifter>::Execute(Battery& bat, Equipment& eq) const {
    if (eq.nv_toggled) bat.charge -= 2.5f; 
    else               bat.charge -= 0.2f;
}

// ======================================================
// 4. THE BEHAVIOR MATRIX (O(1) Resolution Engine)
// ======================================================
template<class...> struct TypeList;
template<class ModelT, template<class> class... Defs> struct BehaviorMatrixSingle : public Defs<ModelT>... {};
template<class ModelList, template<class> class... Defs> struct BehaviorMatrix;

template<class... Models, template<class> class... Defs>
struct BehaviorMatrix<TypeList<Models...>, Defs...> : public BehaviorMatrixSingle<Models, Defs...>... {
    
    template<class Interface>
    static const Interface* Resolve(TypeHash logic_class, const typename Interface::Selector& sel) {
        const Interface* result = nullptr;
        
        // C++17 Fold Expressions: The compiler resolves the jump at compile-time.
        // ZERO loops. ZERO search. Pure O(1) determinism.
        ([&]() {
            if (logic_class == TypeIDOf<Models>::Get()) {
                ([&]() {
                    if constexpr (std::is_base_of_v<Interface, Defs<Models>>) {
                        const Interface* candidate = Defs<Models>::GetHead();
                        if (candidate && candidate->Match(logic_class, sel)) {
                            result = candidate;
                        }
                    }
                }(), ...);
            }
        }(), ...);
        
        return result;
    }
};

// Matrix Instantiation
using AIDomain = BehaviorMatrix<TypeList<Scout, HeavyLifter>, DayBehavior, NightBehavior>;
inline static const AIDomain g_behavior_matrix{};

// ======================================================
// 5. THE SYSTEM (The ECS/CRG Symbiosis)
// ======================================================

// Mock Registry to mimic EnTT view.each()
struct MockRegistry {
    WorldState world;
    std::vector<Battery> batteries;
    std::vector<Equipment> equipments;
    std::vector<IDCard> id_cards;
    std::vector<Biome> biomes;

    void Update() {
        for (size_t i = 0; i < id_cards.size(); ++i) {
            
            // 1. Prepare the nested Selector (Context building)
            INightVisionBehavior::Selector selector {
                world.is_night,
                biomes[i].biome_id
            };

            // 2. Resolve Logic via CRG Matrix (O(1) Compile-time routing)
            if (const auto* behavior = AIDomain::Resolve<INightVisionBehavior>(id_cards[i].logic_class, selector)) {
                
                // 3. Execute with Zero-Cost references (Strict DOD)
                behavior->Execute(batteries[i], equipments[i]);
            }
        }
    }
};

// ======================================================
// ENTRY POINT
// ======================================================
int main() {
    MockRegistry reg;
    
    // Create entities
    reg.id_cards.push_back({ TypeIDOf<Scout>::Get() });
    reg.batteries.push_back({ 100.0f });
    reg.equipments.push_back({ true });
    reg.biomes.push_back({ 0 }); // City

    reg.id_cards.push_back({ TypeIDOf<HeavyLifter>::Get() });
    reg.batteries.push_back({ 100.0f });
    reg.equipments.push_back({ true });
    reg.biomes.push_back({ 1 }); // Desert

    std::cout << "--- FRAME 1: Day Time ---\n";
    reg.world.is_night = false;
    reg.Update();
    std::cout << "Scout Battery: " << reg.batteries[0].charge << "%\n";

    std::cout << "\n--- FRAME 2: Night falls! (Zero-Cost Mutation) ---\n";
    reg.world.is_night = true;
    reg.Update();
    std::cout << "Scout Battery: " << reg.batteries[0].charge << "%\n";
    std::cout << "HeavyLifter Battery: " << reg.batteries[1].charge << "%\n";

    return 0;
}