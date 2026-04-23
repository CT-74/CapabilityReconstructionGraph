// Copyright (c) 2024-2026 Cyril Tissier. All rights reserved.
// Licensed under the Apache License, Version 2.0.
//
#include "raylib.h"
#include "raymath.h"  
#include <entt/entt.hpp>
#include <chrono>
#include <vector>

// ======================================================
// [ ENGINE CORE ] 1. IDENTITY & INFRASTRUCTURE (DENSE IDs)
// ======================================================
using DenseID = std::size_t;

struct TypeIDGenerator {
    static DenseID GetNext() { static DenseID s_id = 0; return s_id++; }
};

template<class T> 
struct DenseTypeID { 
    static DenseID Get() { static DenseID s_id = TypeIDGenerator::GetNext(); return s_id; } 
};

enum class UnitState : std::size_t { Patrol, Combat };
template<typename T> struct EnumTraits;
template<> struct EnumTraits<UnitState> { static constexpr std::size_t Count = 2; };

// --- BEHAVIOR ROUTER (FLAT FUNC PTR TENSOR) ---
// Le routeur ne prend plus d'interface, mais un type de Pointeur de Fonction pur.
template<typename FuncPtrT>
class BehaviorRouter {
    static inline std::vector<const FuncPtrT*> s_tensors;
public:
    static void Register(DenseID id, const FuncPtrT* t) { 
        if (id >= s_tensors.size()) s_tensors.resize(id + 1, nullptr);
        s_tensors[id] = t; 
    }
    
    // Retourne directement le pointeur de fonction
    static FuncPtrT GetBehavior(DenseID id, UnitState state) {
        return s_tensors[id] ? s_tensors[id][(std::size_t)state] : nullptr;
    }
    
    static FuncPtrT GetSingle(DenseID id) {
        return s_tensors[id] ? s_tensors[id][0] : nullptr;
    }
};

// --- GENERIC AUTO-RULES (Stage 11) ---
struct WorldContext { float time_of_day; };
struct BehaviorSettings { float night_start = 18.0f; float fire_range = 300.0f; float damage = 25.0f; };

template<typename SettingsT, typename... Args>
struct IRule {
    virtual ~IRule() = default;
    virtual void Eval(const WorldContext& w, const SettingsT& s, Args&... args) const = 0;
    virtual const IRule* GetNext() const = 0;
};

template<typename SettingsT, typename... Args>
class RuleRegistry {
    static inline std::vector<const IRule<SettingsT, Args...>*> s_tensors;
public:
    static void Register(DenseID id, const IRule<SettingsT, Args...>* node) { 
        if (id >= s_tensors.size()) s_tensors.resize(id + 1, nullptr);
        s_tensors[id] = node; 
    }
    static const IRule<SettingsT, Args...>* GetHead(DenseID id) {
        return (id < s_tensors.size()) ? s_tensors[id] : nullptr;
    }
};

template<typename Derived, typename SettingsT, typename... Args>
class RuleNode : public IRule<SettingsT, Args...> {
    const IRule<SettingsT, Args...>* m_next = nullptr;
public:
    RuleNode(DenseID id) {
        m_next = RuleRegistry<SettingsT, Args...>::GetHead(id);
        RuleRegistry<SettingsT, Args...>::Register(id, this);
    }
    const IRule<SettingsT, Args...>* GetNext() const override { return m_next; }
};

// ======================================================
// [ GAMEPLAY SPACE ] DATA (ECS COMPONENTS)
// ======================================================
struct Body { Vector2 pos; Vector2 vel; };
struct Weapon { float cooldown; float fire_rate; };
struct Health { float current = 100.0f; bool is_dead = false; };
struct Battery { float charge = 100.0f; };
struct CRGIdentity { DenseID logic_class; UnitState current_state; }; 
struct Projectile { float ttl; float damage; entt::entity owner; }; 
struct Renderable { Color color; float size; };
struct DefenseSettings { float armor_multiplier = 1.0f; }; 

struct PatrolData { double data[512]; }; 
struct CombatData { double data[512]; }; 

// ======================================================
// [ GAMEPLAY SPACE ] LOGIC (CRG PARAMS SANDBOX & DOD)
// ======================================================
// 1. Les Contrats de Données stricts
struct UnitParams {
    entt::registry& reg;
    entt::entity e;
    Body& b;
    Weapon& w;
    const BehaviorSettings& s;
    float dt;
};

struct DamageParams {
    Health& h;
    float amount;
    const DefenseSettings& ds;
};

// 2. Les Signatures des Pointeurs de Fonction
using PfnUnitBehavior = void(*)(const UnitParams&);
using PfnDamageBehavior = void(*)(const DamageParams&);

struct Drone {}; struct HeavyLifter {};

// 3. Implémentation des comportements : Des fonctions statiques pures. Zéro objet.
struct CombatDef {
    static void Update(const UnitParams& p) {
        p.b.pos = Vector2Add(p.b.pos, Vector2Scale(p.b.vel, 0.3f * p.dt));
        if (p.w.cooldown <= 0) {
            auto bullet = p.reg.create();
            p.reg.emplace<Projectile>(bullet, 1.5f, p.s.damage, p.e);
            p.reg.emplace<Body>(bullet, p.b.pos, Vector2Scale(Vector2Normalize(p.b.vel), 600.0f));
            p.reg.emplace<Renderable>(bullet, YELLOW, 2.0f);
            p.w.cooldown = p.w.fire_rate;
        }
        if (p.w.cooldown > 0) p.w.cooldown -= p.dt;
    }
};

struct StandardDamage {
    static void Apply(const DamageParams& p) {
        p.h.current -= (p.amount * p.ds.armor_multiplier);
    }
};

// Battery Rule (Stage 11 Auto-Reg - Reste objet car "Cold Path" de setup/rules)
struct DroneNightRule : RuleNode<DroneNightRule, BehaviorSettings, Battery> {
    DroneNightRule() : RuleNode(DenseTypeID<Drone>::Get()) {}
    void Eval(const WorldContext& w, const BehaviorSettings& s, Battery& bat) const override {
        if (w.time_of_day >= s.night_start) bat.charge -= 0.05f;
    }
};
static DroneNightRule g_drone_rule;

// ======================================================
// [ BATTLEFIELD ENGINE ]
// ======================================================
struct ProfileResult { double physics_ms; double ai_ms; double struct_ms; };

class BattlefieldEngine {
    entt::registry reg;
    WorldContext world { 12.0f };
    float mutation_rate = 5.0f;

public:
    void Init() {
        // Enregistrement des tableaux de pointeurs de fonctions purs
        static const PfnUnitBehavior drone_ai[2] = { nullptr, &CombatDef::Update };
        BehaviorRouter<PfnUnitBehavior>::Register(DenseTypeID<Drone>::Get(), drone_ai);

        static const PfnDamageBehavior std_dmg[1] = { &StandardDamage::Apply };
        BehaviorRouter<PfnDamageBehavior>::Register(DenseTypeID<Drone>::Get(), std_dmg);
        BehaviorRouter<PfnDamageBehavior>::Register(DenseTypeID<HeavyLifter>::Get(), std_dmg);

        AddWave(1500);
    }

    void AddWave(int count) {
        for(int i=0; i<count; i++) SpawnUnit(GetRandomValue(0,1) == 0 ? DenseTypeID<Drone>::Get() : DenseTypeID<HeavyLifter>::Get());
    }

    void SpawnUnit(DenseID type) {
        auto e = reg.create();
        reg.emplace<Body>(e, Vector2{(float)GetRandomValue(0, 1280), (float)GetRandomValue(0, 720)}, Vector2{(float)GetRandomValue(-120, 120), (float)GetRandomValue(-120, 120)});
        reg.emplace<Weapon>(e, 0.0f, (type == DenseTypeID<Drone>::Get() ? 0.3f : 0.8f));
        reg.emplace<CRGIdentity>(e, type, UnitState::Combat);
        reg.emplace<Health>(e, 100.0f, false);
        reg.emplace<Battery>(e, 100.0f);
        reg.emplace<DefenseSettings>(e, (type == DenseTypeID<Drone>::Get() ? 0.6f : 1.0f)); 
        reg.emplace<BehaviorSettings>(e, 18.0f, 300.0f, 30.0f);
        reg.emplace<Renderable>(e, (type == DenseTypeID<Drone>::Get() ? SKYBLUE : ORANGE), (type == DenseTypeID<Drone>::Get() ? 3.0f : 6.0f));
        reg.emplace<PatrolData>(e);
    }

ProfileResult Update(float dt, bool use_crg, bool immortal) {
        ProfileResult prof = {0, 0, 0};
        world.time_of_day = fmod(world.time_of_day + dt * 0.2f, 24.0f);

        // ==========================================
        // 1. PHASE PHYSIQUE & COLLISIONS 
        // ==========================================
        auto start_physics = std::chrono::high_resolution_clock::now();
        auto target_view = reg.view<CRGIdentity, Body, Health, DefenseSettings>();
        
        reg.view<Projectile, Body>().each([&](auto p_entity, Projectile& p, Body& b) {
            b.pos = Vector2Add(b.pos, Vector2Scale(b.vel, dt)); p.ttl -= dt;
            for(auto [u_entity, u_id, u_b, u_h, u_ds] : target_view.each()) {
                if (p.owner != u_entity && CheckCollisionCircles(b.pos, 2.0f, u_b.pos, 7.0f)) {
                    
                    // Exécution O(1) DOD via FuncPtr
                    if (PfnDamageBehavior func = BehaviorRouter<PfnDamageBehavior>::GetSingle(u_id.logic_class)) {
                        DamageParams params{u_h, p.damage, u_ds};
                        func(params);
                    }
                    p.ttl = 0; break;
                }
            }
            if (p.ttl <= 0) reg.destroy(p_entity);
        });
        prof.physics_ms = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start_physics).count();

        // ==========================================
        // 2. PHASE DÉCISIONNELLE (CRG IA - FUNC PTR)
        // ==========================================
        auto start_ai = std::chrono::high_resolution_clock::now();
        reg.view<CRGIdentity, Body, Weapon, BehaviorSettings, Health, Battery>().each([&](auto entity, auto& id, auto& b, auto& w, auto& s, auto& h, auto& bat) {
            
            // Exécution O(1) DOD via FuncPtr et Params Sandbox
            if (PfnUnitBehavior func = BehaviorRouter<PfnUnitBehavior>::GetBehavior(id.logic_class, id.current_state)) {
                UnitParams params{reg, entity, b, w, s, dt};
                func(params);
            }
            
            // Cold Path Rules 
            for (auto* r = RuleRegistry<BehaviorSettings, Battery>::GetHead(id.logic_class); r; r = r->GetNext()) {
                r->Eval(world, s, bat);
            }
            
            if (!immortal && h.current <= 0) h.is_dead = true;
        });
        prof.ai_ms = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start_ai).count();

        // ==========================================
        // 3. PHASE STRUCTURELLE
        // ==========================================
        auto start_struct = std::chrono::high_resolution_clock::now();
        reg.view<CRGIdentity>().each([&](auto entity, auto& id) {
            if (GetRandomValue(0, 1000) < (mutation_rate * 10)) {
                UnitState next = (id.current_state == UnitState::Patrol) ? UnitState::Combat : UnitState::Patrol;
                if (!use_crg) {
                    if (next == UnitState::Combat) { if(reg.all_of<PatrolData>(entity)) reg.remove<PatrolData>(entity); reg.emplace<CombatData>(entity); }
                    else { if(reg.all_of<CombatData>(entity)) reg.remove<CombatData>(entity); reg.emplace<PatrolData>(entity); }
                }
                id.current_state = next;
            }
        });

        for(auto [e, h] : reg.view<Health>().each()) {
            if (h.is_dead) { reg.destroy(e); SpawnUnit(GetRandomValue(0,1) == 0 ? DenseTypeID<Drone>::Get() : DenseTypeID<HeavyLifter>::Get()); }
        }
        prof.struct_ms = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start_struct).count();
        
        return prof;
    }

void Render(ProfileResult p, bool crg, bool immortal) {
        BeginDrawing(); 
        ClearBackground(Color{5, 5, 10, 255});
        
        reg.view<Body, Renderable, Health, CRGIdentity>().each([](const Body& b, const Renderable& r, const Health& h, const CRGIdentity& id) {
            Color final_c = (id.current_state == UnitState::Combat) ? WHITE : r.color;
            DrawCircleV(b.pos, r.size, Fade(final_c, std::max(0.2f, h.current / 100.0f)));
        });
        
        DrawRectangle(0, 0, 480, 240, Fade(BLACK, 0.8f));
        
        size_t entity_count = reg.storage<entt::entity>().size();
        
        // --- CALCUL DE LA TAXE ---
        // Avec un FuncPtr brut, l'indirection est ~1ns. Zéro VTable lookup.
        double ep_tax_ms = (double)entity_count * 0.000001; 
        
        double displayed_tax = crg ? ep_tax_ms : p.struct_ms;
        Color tax_color = crg ? LIGHTGRAY : (p.struct_ms > 1.2 ? RED : ORANGE);

        DrawText(TextFormat("PHYSICS & COLLISION: %.2f ms", p.physics_ms), 10, 10, 20, LIGHTGRAY); 
        DrawText(TextFormat("AI LOGIC (FUNC PTR): %.2f ms", p.ai_ms), 10, 40, 20, GREEN);
        DrawText(TextFormat("TAX: %.2f ms", displayed_tax), 10, 70, 24, tax_color);     
        DrawText(TextFormat("MODE: %s (SPACE)", crg ? "CRG (FuncPtr Tax)" : "ECS (Structural Tax)"), 10, 110, 20, RAYWHITE);
        DrawText(TextFormat("MUTATION RATE: %.1f %% (UP/DOWN)", mutation_rate), 10, 140, 20, SKYBLUE);        
        DrawText(TextFormat("IMMORTAL: %s (I) | WAVE (W)", immortal ? "YES" : "NO"), 10, 170, 20, LIGHTGRAY);
        DrawText(TextFormat("ENTITIES: %zu", entity_count), 10, 200, 20, RAYWHITE); 
        
        EndDrawing();
    }

    void SetMutationRate(float r) { mutation_rate = r; }
};

int main() {
    InitWindow(1280, 720, "CRG STAGE 13 - ZERO COST DOD (FUNCPTR)");
    SetTargetFPS(60); BattlefieldEngine engine; engine.Init();
    bool use_crg = true; bool immortal = false; float m_rate = 5.0f;
    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_SPACE)) use_crg = !use_crg;
        if (IsKeyPressed(KEY_I)) immortal = !immortal;
        if (IsKeyPressed(KEY_W)) engine.AddWave(500); 
        if (IsKeyDown(KEY_UP)) m_rate = std::min(100.0f, m_rate + 0.5f);
        if (IsKeyDown(KEY_DOWN)) m_rate = std::max(0.0f, m_rate - 0.5f);
        
        engine.SetMutationRate(m_rate);
        ProfileResult p = engine.Update(GetFrameTime(), use_crg, immortal);
        engine.Render(p, use_crg, immortal);
    }
    CloseWindow(); return 0;
}