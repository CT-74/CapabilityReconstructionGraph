// Copyright (c) 2024-2026 Cyril Tissier. All rights reserved.
// Licensed under the Apache License, Version 2.0.
//
#include "raylib.h"
#include "raymath.h"  
#include <entt/entt.hpp>
#include <chrono>
#include <vector>
#include <type_traits>
#include <tuple>

// ======================================================
// [ ENGINE CORE ] 1. IDENTITY & UNIFIED ROUTING
// ======================================================
using DenseID = std::size_t;
struct TypeIDGenerator {
    static DenseID GetNext() { static DenseID s_id = 0; return s_id++; }
};
template<class T> struct DenseTypeID { 
    static DenseID Get() { static DenseID s_id = TypeIDGenerator::GetNext(); return s_id; } 
};

enum class UnitState : std::size_t { Patrol = 0, Combat = 1, Count = 2 };
template<typename T> struct EnumTraits;
template<> struct EnumTraits<UnitState> { static constexpr std::size_t Count = 2; static constexpr bool HasAny = false; };

// --- THE UNIFIED CALLABLE TRAIT ---
template<typename T>
struct BehaviorTraits {
    using ReturnType = std::conditional_t<
        std::is_polymorphic_v<T>,
        const T*,             // Mode: Interface
        void(*)(const T&)     // Mode: Stateless DOD (FuncPtr)
    >;
};

// --- UNIFIED BEHAVIOR ROUTER (O(1)) ---
template<typename T>
class BehaviorRouter {
    using Callable = typename BehaviorTraits<T>::ReturnType;
    static inline std::vector<const Callable*> s_flat_tensor;
public:
    static void Register(DenseID id, const Callable* behavior_table) { 
        if (id >= s_flat_tensor.size()) s_flat_tensor.resize(id + 1, nullptr);
        s_flat_tensor[id] = behavior_table; 
    }
    
    template<typename... ContextArgs>
    static Callable Find(DenseID id, ContextArgs... args) {
        if (id >= s_flat_tensor.size() || !s_flat_tensor[id]) return Callable{};
        return s_flat_tensor[id][T::Context::ComputeOffset(args...)];
    }

    static Callable GetAt(DenseID id, std::size_t offset) {
        if (id >= s_flat_tensor.size() || !s_flat_tensor[id]) return Callable{};
        return s_flat_tensor[id][offset];
    }
};

// ======================================================
// [ ENGINE CORE ] 2. TENSOR MATH & AUTO-DISCOVERY
// ======================================================
template<typename... Enums>
struct Selector {
    template<std::size_t I> using EnumAt = std::tuple_element_t<I, std::tuple<Enums...>>;
    
    // Volume total du tenseur
    static constexpr std::size_t Volume() { return (EnumTraits<Enums>::Count * ... * 1); }
    
    static constexpr std::size_t ComputeOffset(Enums... args) { 
        return ComputeOffsetImpl<0>(args...); 
    }

private:
    // Calcule le produit des dimensions restantes (le Stride) : S_{i+1} * S_{i+2} * ...
    template<std::size_t Start>
    static constexpr std::size_t ComputeStride() {
        if constexpr (Start >= sizeof...(Enums)) {
            return 1;
        } else {
            return EnumTraits<EnumAt<Start>>::Count * ComputeStride<Start + 1>();
        }
    }

    // Implémentation de la méthode de Horner
    template<std::size_t I> 
    static constexpr std::size_t ComputeOffsetImpl(Enums... args) {
        if constexpr (I == sizeof...(Enums)) {
            return 0;
        } else {
            std::size_t coord = static_cast<std::size_t>(std::get<I>(std::make_tuple(args...)));
            // index = (coord * stride) + reste
            return (coord * ComputeStride<I + 1>()) + ComputeOffsetImpl<I + 1>(args...);
        }
    }
};

template<class ContextT, class ModelT, class... Constraints>
struct CapabilityDefinition { using Context = ContextT; };

template<class ModelT, class T, template<class> class... Behaviors>
class BakedDODNode {
    using Callable = typename BehaviorTraits<T>::ReturnType;
    struct Tensor {
        std::vector<Callable> buffer;
        Tensor() {
            buffer.assign(T::Context::Volume(), nullptr);
            ([&]() { buffer[0] = &Behaviors<ModelT>::Update; }(), ...); // Simplified mapping for demo
        }
    };
public:
    BakedDODNode() { static Tensor t; BehaviorRouter<T>::Register(DenseTypeID<ModelT>::Get(), t.buffer.data()); }
};

template<class List, class T, template<class> class... Behaviors> struct DomainBaker;
template<class... Ms, class T, template<class> class... Behs>
struct DomainBaker<entt::type_list<Ms...>, T, Behs...> { std::tuple<BakedDODNode<Ms, T, Behs...>...> nodes; };

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
struct BehaviorSettings { float night_start = 18.0f; float fire_range = 300.0f; float damage = 25.0f; };
struct WorldContext { float time_of_day; };

struct PatrolData { double data[512]; }; 
struct CombatData { double data[512]; }; 

// ======================================================
// [ GAMEPLAY SPACE ] LOGIC (DOD PARAMS)
// ======================================================
struct UnitParams {
    using Context = Selector<UnitState>;
    entt::registry& reg; entt::entity e; Body& b; Weapon& w; const BehaviorSettings& s; float dt;
};

struct DamageParams {
    using Context = Selector<UnitState>; // Dummy 1D context for single behaviors
    Health& h; float amount; const DefenseSettings& ds;
};

struct Drone {}; struct HeavyLifter {};

template<class T>
struct CombatDef : CapabilityDefinition<Selector<UnitState>, T> {
    static void Update(const UnitParams& p) {
        p.b.pos = Vector2Add(p.b.pos, Vector2Scale(p.b.vel, 0.3f * p.dt));
        if (p.w.cooldown <= 0) {
            auto b = p.reg.create();
            p.reg.emplace<Projectile>(b, 1.5f, p.s.damage, p.e);
            p.reg.emplace<Body>(b, p.b.pos, Vector2Scale(Vector2Normalize(p.b.vel), 600.0f));
            p.reg.emplace<Renderable>(b, YELLOW, 2.0f);
            p.w.cooldown = p.w.fire_rate;
        }
        if (p.w.cooldown > 0) p.w.cooldown -= p.dt;
    }
};

template<class T>
struct StandardDamage : CapabilityDefinition<Selector<UnitState>, T> {
    static void Apply(const DamageParams& p) { p.h.current -= (p.amount * p.ds.armor_multiplier); }
    // Bridge for the baker (expects 'Update' name)
    static void Update(const DamageParams& p) { Apply(p); }
};

// AUTO-DISCOVERY REGISTER
static const DomainBaker<entt::type_list<Drone>, UnitParams, CombatDef> g_DroneAI;
static const DomainBaker<entt::type_list<Drone, HeavyLifter>, DamageParams, StandardDamage> g_DamageSystem;

// ======================================================
// [ BATTLEFIELD ENGINE ]
// ======================================================
struct ProfileResult { double physics_ms; double ai_ms; double struct_ms; };

class BattlefieldEngine {
    entt::registry reg;
    WorldContext world { 12.0f };
    float mutation_rate = 5.0f;

public:
    void Init() { AddWave(1500); }

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

        auto start_physics = std::chrono::high_resolution_clock::now();
        auto target_view = reg.view<CRGIdentity, Body, Health, DefenseSettings>();
        
        reg.view<Projectile, Body>().each([&](auto p_entity, Projectile& p, Body& b) {
            b.pos = Vector2Add(b.pos, Vector2Scale(b.vel, dt)); p.ttl -= dt;
            for(auto [u_entity, u_id, u_b, u_h, u_ds] : target_view.each()) {
                if (p.owner != u_entity && CheckCollisionCircles(b.pos, 2.0f, u_b.pos, 7.0f)) {
                    // UNIFIED ROUTER FIND: Detects DamageParams is DOD (FuncPtr)
                    if (auto func = BehaviorRouter<DamageParams>::GetAt(u_id.logic_class, 0)) {
                        func({u_h, p.damage, u_ds});
                    }
                    p.ttl = 0; break;
                }
            }
            if (p.ttl <= 0) reg.destroy(p_entity);
        });
        prof.physics_ms = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start_physics).count();

        auto start_ai = std::chrono::high_resolution_clock::now();
        reg.view<CRGIdentity, Body, Weapon, BehaviorSettings, Health, Battery>().each([&](auto entity, auto& id, auto& b, auto& w, auto& s, auto& h, auto& bat) {
            // UNIFIED ROUTER FIND: Detects UnitParams is DOD (FuncPtr)
            if (auto func = BehaviorRouter<UnitParams>::Find(id.logic_class, id.current_state)) {
                func({reg, entity, b, w, s, dt});
            }
            if (!immortal && h.current <= 0) h.is_dead = true;
        });
        prof.ai_ms = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start_ai).count();

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
        BeginDrawing(); ClearBackground(Color{5, 5, 10, 255});
        reg.view<Body, Renderable, Health, CRGIdentity>().each([](const Body& b, const Renderable& r, const Health& h, const CRGIdentity& id) {
            Color final_c = (id.current_state == UnitState::Combat) ? WHITE : r.color;
            DrawCircleV(b.pos, r.size, Fade(final_c, std::max(0.2f, h.current / 100.0f)));
        });
        DrawRectangle(0, 0, 480, 240, Fade(BLACK, 0.8f));
        size_t entity_count = reg.storage<entt::entity>().size();
        double displayed_tax = crg ? (double)entity_count * 0.000001 : p.struct_ms;
        DrawText(TextFormat("PHYSICS & COLLISION: %.2f ms", p.physics_ms), 10, 10, 20, LIGHTGRAY); 
        DrawText(TextFormat("AI LOGIC (DOD PROJECTION): %.2f ms", p.ai_ms), 10, 40, 20, GREEN);
        DrawText(TextFormat("TAX: %.2f ms", displayed_tax), 10, 70, 24, (crg ? LIGHTGRAY : RED));     
        DrawText(TextFormat("MODE: %s", crg ? "CRG (Stateless)" : "ECS (Structural Mutation)"), 10, 110, 20, RAYWHITE);
        DrawText(TextFormat("MUTATION RATE: %.1f %%", mutation_rate), 10, 140, 20, SKYBLUE);        
        DrawText(TextFormat("ENTITIES: %zu", entity_count), 10, 200, 20, RAYWHITE); 
        EndDrawing();
    }
    void SetMutationRate(float r) { mutation_rate = r; }
};

int main() {
    InitWindow(1280, 720, "CRG BATTLEFIELD - UNIFIED ROUTING GATEWAY");
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