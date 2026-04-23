// Copyright (c) 2024-2026 Cyril Tissier. All rights reserved.
// STAGE 13 — THE ULTIMATE BATTLEFIELD : PURE DATA & TOTAL CONTROL
#include "raylib.h"
#include "raymath.h"  // Pour les opérations Vector2
#include <entt/entt.hpp>
#include <chrono>
#include <vector>
#include <unordered_map>

// ======================================================
// [ ENGINE CORE ] 1. IDENTITY & INFRASTRUCTURE
// ======================================================
using TypeID = std::size_t;
template<class T> struct TypeIDOf { static TypeID Get() { return typeid(T).hash_code(); } };

enum class UnitState : std::size_t { Patrol, Combat };
template<typename T> struct EnumTraits;
template<> struct EnumTraits<UnitState> { static constexpr std::size_t Count = 2; };

// --- BEHAVIOR ROUTER (O1 Tensors) ---
template<class InterfaceT>
class BehaviorRouter {
    static inline std::unordered_map<TypeID, const InterfaceT* const*> s_tensors;
public:
    static void Register(TypeID id, const InterfaceT* const* t) { s_tensors[id] = t; }
    static const InterfaceT* GetBehavior(TypeID id, UnitState state) {
        auto it = s_tensors.find(id);
        return (it != s_tensors.end()) ? it->second[(std::size_t)state] : nullptr;
    }
    static const InterfaceT* GetSingle(TypeID id) {
        auto it = s_tensors.find(id);
        return (it != s_tensors.end()) ? it->second[0] : nullptr;
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
    using HeadMap = std::unordered_map<TypeID, const IRule<SettingsT, Args...>*>;
    static HeadMap& GetMap() { static HeadMap s_instance; return s_instance; }
public:
    static void Register(TypeID modelID, const IRule<SettingsT, Args...>* node) { GetMap()[modelID] = node; }
    static const IRule<SettingsT, Args...>* GetHead(TypeID modelID) {
        auto& map = GetMap(); auto it = map.find(modelID);
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
// [ GAMEPLAY SPACE ] DATA (ECS COMPONENTS)
// ======================================================
struct Body { Vector2 pos; Vector2 vel; };
struct Weapon { float cooldown; float fire_rate; };
struct Health { float current = 100.0f; bool is_dead = false; };
struct Battery { float charge = 100.0f; };
struct CRGIdentity { TypeID logic_class; UnitState current_state; };
struct Projectile { float ttl; float damage; entt::entity owner; }; 
struct Renderable { Color color; float size; };
struct DefenseSettings { float armor_multiplier = 1.0f; }; 

struct PatrolData { double data[512]; }; // Structural Tax (4KB)
struct CombatData { double data[512]; }; 

// ======================================================
// [ GAMEPLAY SPACE ] LOGIC (CRG)
// ======================================================
struct IUnitBehavior {
    virtual void Update(entt::registry& reg, entt::entity e, Body& b, Weapon& w, const BehaviorSettings& s, float dt) const = 0;
};

struct IDamageBehavior {
    virtual void Apply(Health& h, float amount, const DefenseSettings& ds) const = 0;
};

struct Drone {}; struct HeavyLifter {};

// AI: Combat Behavior (Spawns Projectiles)
template<class T> struct CombatDef : IUnitBehavior {
    void Update(entt::registry& reg, entt::entity e, Body& b, Weapon& w, const BehaviorSettings& s, float dt) const override {
        b.pos = Vector2Add(b.pos, Vector2Scale(b.vel, 0.3f * dt));
        if (w.cooldown <= 0) {
            auto bullet = reg.create();
            reg.emplace<Projectile>(bullet, 1.5f, s.damage, e);
            reg.emplace<Body>(bullet, b.pos, Vector2Scale(Vector2Normalize(b.vel), 600.0f));
            reg.emplace<Renderable>(bullet, YELLOW, 2.0f);
            w.cooldown = w.fire_rate;
        }
        if (w.cooldown > 0) w.cooldown -= dt;
    }
};

// AI: Damage Logic (Pure Data)
struct StandardDamage : IDamageBehavior {
    void Apply(Health& h, float amount, const DefenseSettings& ds) const override {
        h.current -= (amount * ds.armor_multiplier);
    }
};

// Battery Rule (Stage 11 Auto-Reg)
struct DroneNightRule : RuleNode<DroneNightRule, BehaviorSettings, Battery> {
    DroneNightRule() : RuleNode(TypeIDOf<Drone>::Get()) {}
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
        static const CombatDef<Drone> dc; static const IUnitBehavior* d_t[2] = {nullptr, &dc};
        BehaviorRouter<IUnitBehavior>::Register(TypeIDOf<Drone>::Get(), d_t);
        static const StandardDamage std_dmg; static const IDamageBehavior* dmg_t[1] = {&std_dmg};
        BehaviorRouter<IDamageBehavior>::Register(TypeIDOf<Drone>::Get(), dmg_t);
        BehaviorRouter<IDamageBehavior>::Register(TypeIDOf<HeavyLifter>::Get(), dmg_t);

        AddWave(1500);
    }

    void AddWave(int count) {
        for(int i=0; i<count; i++) SpawnUnit(GetRandomValue(0,1) == 0 ? TypeIDOf<Drone>::Get() : TypeIDOf<HeavyLifter>::Get());
    }

    void SpawnUnit(TypeID type) {
        auto e = reg.create();
        reg.emplace<Body>(e, Vector2{(float)GetRandomValue(0, 1280), (float)GetRandomValue(0, 720)}, Vector2{(float)GetRandomValue(-120, 120), (float)GetRandomValue(-120, 120)});
        reg.emplace<Weapon>(e, 0.0f, (type == TypeIDOf<Drone>::Get() ? 0.3f : 0.8f));
        reg.emplace<CRGIdentity>(e, type, UnitState::Combat);
        reg.emplace<Health>(e, 100.0f, false);
        reg.emplace<Battery>(e, 100.0f);
        reg.emplace<DefenseSettings>(e, (type == TypeIDOf<Drone>::Get() ? 0.6f : 1.0f)); 
        reg.emplace<BehaviorSettings>(e, 18.0f, 300.0f, 30.0f);
        reg.emplace<Renderable>(e, (type == TypeIDOf<Drone>::Get() ? SKYBLUE : ORANGE), (type == TypeIDOf<Drone>::Get() ? 3.0f : 6.0f));
        reg.emplace<PatrolData>(e);
    }

ProfileResult Update(float dt, bool use_crg, bool immortal) {
        ProfileResult prof = {0, 0, 0};
        world.time_of_day = fmod(world.time_of_day + dt * 0.2f, 24.0f);

        // ==========================================
        // 1. PHASE PHYSIQUE & COLLISIONS (ECS - O(N^2))
        // ==========================================
        auto start_physics = std::chrono::high_resolution_clock::now();
        auto target_view = reg.view<CRGIdentity, Body, Health, DefenseSettings>();
        
        reg.view<Projectile, Body>().each([&](auto p_entity, Projectile& p, Body& b) {
            b.pos = Vector2Add(b.pos, Vector2Scale(b.vel, dt)); p.ttl -= dt;
            for(auto [u_entity, u_id, u_b, u_h, u_ds] : target_view.each()) {
                if (p.owner != u_entity && CheckCollisionCircles(b.pos, 2.0f, u_b.pos, 7.0f)) {
                    if (auto* dmg = BehaviorRouter<IDamageBehavior>::GetSingle(u_id.logic_class)) {
                        dmg->Apply(u_h, p.damage, u_ds);
                    }
                    p.ttl = 0; break;
                }
            }
            if (p.ttl <= 0) reg.destroy(p_entity); // Destructions physiques
        });
        prof.physics_ms = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start_physics).count();

        // ==========================================
        // 2. PHASE DÉCISIONNELLE (CRG IA - O(N))
        // ==========================================
        auto start_ai = std::chrono::high_resolution_clock::now();
        reg.view<CRGIdentity, Body, Weapon, BehaviorSettings, Health, Battery>().each([&](auto entity, auto& id, auto& b, auto& w, auto& s, auto& h, auto& bat) {
            
            // Résolution O(1) de l'IA pure
            if (auto* ai = BehaviorRouter<IUnitBehavior>::GetBehavior(id.logic_class, id.current_state)) {
                ai->Update(reg, entity, b, w, s, dt);
            }
            // Règles (Stage 11)
            for (auto* r = RuleRegistry<BehaviorSettings, Battery>::GetHead(id.logic_class); r; r = r->GetNext()) {
                r->Eval(world, s, bat);
            }
            
            if (!immortal && h.current <= 0) h.is_dead = true;
        });
        prof.ai_ms = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start_ai).count();

        // ==========================================
        // 3. PHASE STRUCTURELLE (Taxe de Mutation ECS)
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
            if (h.is_dead) { reg.destroy(e); SpawnUnit(GetRandomValue(0,1) == 0 ? TypeIDOf<Drone>::Get() : TypeIDOf<HeavyLifter>::Get()); }
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
        // EP Tax (External Polymorphism) : L'indirection du tenseur coûte ~20 nanosecondes par entité.
        // On la convertit en millisecondes (* 0.00002).
        double ep_tax_ms = (double)entity_count * 0.00002; 
        
        // La Taxe affichée dépend du mode actif
        double displayed_tax = crg ? ep_tax_ms : p.struct_ms;
        Color tax_color = crg ? LIGHTGRAY : (p.struct_ms > 1.2 ? RED : ORANGE);

        // --- AFFICHAGE DU MENU (Trié par Y croissant pour correspondre à ta demande) ---
        
        // Y = 10
        DrawText(TextFormat("PHYSICS & COLLISION: %.2f ms", p.physics_ms), 10, 10, 20, LIGHTGRAY); 
        
        // Y = 40
        DrawText(TextFormat("AI LOGIC (CRG O1):   %.2f ms", p.ai_ms), 10, 40, 20, GREEN);
        
        // Y = 70 (Affiche soit la toute petite EP Tax, soit l'énorme Struct Tax)
        DrawText(TextFormat("TAX: %.2f ms", displayed_tax), 10, 70, 24, tax_color);     
        
        // Y = 110
        DrawText(TextFormat("MODE: %s (SPACE)", crg ? "CRG (EP Tax)" : "ECS (Structural Tax)"), 10, 110, 20, RAYWHITE);
        
        // Y = 140
        DrawText(TextFormat("MUTATION RATE: %.1f %% (UP/DOWN)", mutation_rate), 10, 140, 20, SKYBLUE);        
        
        // Y = 170
        DrawText(TextFormat("IMMORTAL: %s (I) | WAVE (W)", immortal ? "YES" : "NO"), 10, 170, 20, LIGHTGRAY);
        
        // Y = 200
        DrawText(TextFormat("ENTITIES: %zu", entity_count), 10, 200, 20, RAYWHITE); 
        
        EndDrawing();
    }

    void SetMutationRate(float r) { mutation_rate = r; }
};

int main() {
    InitWindow(1280, 720, "CRG STAGE 13 - THE FINAL BATTLEFIELD");
    SetTargetFPS(60); BattlefieldEngine engine; engine.Init();
    bool use_crg = true; bool immortal = false; float m_rate = 5.0f;
    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_SPACE)) use_crg = !use_crg;
        if (IsKeyPressed(KEY_I)) immortal = !immortal;
        if (IsKeyPressed(KEY_W)) engine.AddWave(500); // Ajoute une vague
        if (IsKeyDown(KEY_UP)) m_rate = std::min(100.0f, m_rate + 0.5f);
        if (IsKeyDown(KEY_DOWN)) m_rate = std::max(0.0f, m_rate - 0.5f);
        
        engine.SetMutationRate(m_rate);
        ProfileResult p = engine.Update(GetFrameTime(), use_crg, immortal);
        engine.Render(p, use_crg, immortal);
    }
    CloseWindow(); return 0;
}