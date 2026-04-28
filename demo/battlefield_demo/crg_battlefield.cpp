// Copyright (c) 2024-2026 Cyril Tissier. All rights reserved.
#include "raylib.h"
#include "raymath.h"  
#include <entt/entt.hpp>
#include <vector>
#include <typeinfo>
#include <iostream>

// =============================================================================
// [ ENGINE CORE ] 1. INFRASTRUCTURE DLL-SAFE (STAGE 09/10 COMPLIANT)
// =============================================================================

#ifndef CRG_DLL_ENABLED
#define CRG_DLL_ENABLED 1
#endif

template<class T> struct RegistrySlot {
#if !CRG_DLL_ENABLED
    static inline T s_Value{}; 
#else
    static T s_Value;
#endif
};

#if CRG_DLL_ENABLED
    #define CRG_BIND_SLOT(T) template<> T RegistrySlot<T>::s_Value{};
#else
    #define CRG_BIND_SLOT(T) 
#endif

template<class TNode> using NodeListAnchor = RegistrySlot<const TNode*>;

template<class TNode, class TInterface>
struct NodeList : public TInterface {
    const TNode* m_Next = nullptr;
    NodeList() {
        const TNode* derivedThis = static_cast<const TNode*>(this);
        m_Next = NodeListAnchor<TNode>::s_Value;
        NodeListAnchor<TNode>::s_Value = derivedThis;
    }
};

using DenseID = std::size_t;
struct TypeIDGenerator { static DenseID GetNext() { static DenseID s_id = 0; return s_id++; } };
template<class T> struct DenseTypeID { static DenseID Get() { static DenseID s_id = TypeIDGenerator::GetNext(); return s_id; } };

// =============================================================================
// [ ENGINE CORE ] 2. TENSOR MATH (HORNER)
// =============================================================================

template<typename T> struct EnumTraits;

template<class... TAxes>
struct Space {
    static constexpr std::size_t Volume = (1 * ... * EnumTraits<TAxes>::Count);
    static constexpr std::size_t ComputeOffset(TAxes... coords) {
        std::size_t offset = 0;
        ((offset = offset * EnumTraits<TAxes>::Count + static_cast<std::size_t>(coords)), ...);
        return offset;
    }
};

template<class TInterface> struct CapabilityTopology;

// =============================================================================
// [ ENGINE CORE ] 3. ROUTING ENGINE (STATELESS DOD)
// =============================================================================

template<class InterfaceT>
struct RouteNode {
    using ContextT = typename InterfaceT::RuleContext;
    InterfaceT   descriptor; 
    bool (*predicate)(const ContextT&) = nullptr;
    int          priority = 0;
    RouteNode* next = nullptr;
};

template<class InterfaceT>
struct Bucket {
    RouteNode<InterfaceT>* head = nullptr;
    InterfaceT             fallback;
};

template<class InterfaceT>
using TensorArena = RegistrySlot<std::vector<Bucket<InterfaceT>>>;

struct IBakerNode;
struct IAssembler { virtual void Bake() const = 0; };
struct IBakerNode : public NodeList<IBakerNode, IAssembler> {};

CRG_BIND_SLOT(const IBakerNode*)

template<class InterfaceT>
class BehaviorRouter {
public:
    static void EnsureBaked() {
        static struct StaticGuard {
            StaticGuard() {
                for (const IBakerNode* b = NodeListAnchor<IBakerNode>::s_Value; b; b = b->m_Next) b->Bake();
            }
        } s_guard;
    }

    static void Register(DenseID modelID, std::size_t localOffset, RouteNode<InterfaceT>* node) {
        auto& arena = TensorArena<InterfaceT>::s_Value;
        using TSpace = typename CapabilityTopology<InterfaceT>::SpaceType;
        std::size_t globalIdx = (modelID * TSpace::Volume) + localOffset;
        if (globalIdx >= arena.size()) arena.resize(globalIdx + 1, {});
        auto& b = arena[globalIdx];
        if (!node->predicate) b.fallback = node->descriptor;
        if (!b.head || node->priority > b.head->priority) { node->next = b.head; b.head = node; }
        else { auto* c = b.head; while (c->next && c->next->priority >= node->priority) c = c->next; node->next = c->next; c->next = node; }
    }

    template<class... Args>
    static const InterfaceT* Find(DenseID modelID, const typename InterfaceT::RuleContext& ctx, Args... coords) {
        EnsureBaked();
        auto& arena = TensorArena<InterfaceT>::s_Value;
        using TSpace = typename CapabilityTopology<InterfaceT>::SpaceType;
        std::size_t idx = (modelID * TSpace::Volume) + TSpace::ComputeOffset(coords...);
        if (idx >= arena.size()) return nullptr;
        for (auto* n = arena[idx].head; n; n = n->next) if (!n->predicate || n->predicate(ctx)) return &n->descriptor;
        return &arena[idx].fallback;
    }
};

// =============================================================================
// [ ENGINE CORE ] 4. BAKER BRIDGE
// =============================================================================

template<class TInterface> struct Capability : public TInterface { using InterfaceType = TInterface; };
template<typename T, typename = void> struct PriorityOf { static constexpr int Value = 0; };
template<typename T> struct PriorityOf<T, std::void_t<decltype(T::Priority)>> { static constexpr int Value = T::Priority; };

template<class TModel, template<class> class... TCapabilities>
struct CapabilityBaker : public IBakerNode {
    void Bake() const override { (BakeOne<TCapabilities<TModel>>(), ...); }
private:
    template<class TImpl> void BakeOne() const {
        using Intf = typename TImpl::InterfaceType;
        using TSpace = typename CapabilityTopology<Intf>::SpaceType;
        for (std::size_t i = 0; i < TSpace::Volume; ++i) {
            auto* node = new RouteNode<Intf>();
            node->descriptor.pfnExecute = &TImpl::Execute;
            node->priority = PriorityOf<TImpl>::Value;
            // node->predicate = ... (SFINAE detection for Condition)
            BehaviorRouter<Intf>::Register(DenseTypeID<TModel>::Get(), i, node);
        }
    }
};

// =============================================================================
// [ GAMEPLAY ] 1. DATA (ECS COMPONENTS)
// =============================================================================

struct Body { Vector2 pos; Vector2 vel; };
struct Weapon { float cooldown; float fire_rate; };
struct Health { float current = 100.0f; bool is_dead = false; };
struct CRGIdentity { DenseID logic_class; }; 
struct Renderable { Color color; float size; };
struct BehaviorSettings { float damage = 25.0f; };

enum class CombatState { Idle, Aggressive };
template<> struct EnumTraits<CombatState> { static constexpr std::size_t Count = 2; };

// =============================================================================
// [ GAMEPLAY ] 2. CONTRACTS (STATELESS DOD)
// =============================================================================

struct IUnitAI {
    struct Params { entt::registry& reg; entt::entity e; Body& b; Weapon& w; const BehaviorSettings& s; float dt; };
    struct RuleContext { }; // Decision context
    void (*pfnExecute)(Params&);
    void Execute(Params& p) const { pfnExecute(p); }
};

template<> struct CapabilityTopology<IUnitAI> { using SpaceType = Space<CombatState>; };
CRG_BIND_SLOT(std::vector<Bucket<IUnitAI>>)

// =============================================================================
// [ GAMEPLAY ] 3. LOGIC IMPLEMENTATIONS
// =============================================================================

template<class T>
struct DroneStrike : Capability<IUnitAI> {
    static void Execute(Params& p) {
        p.b.pos = Vector2Add(p.b.pos, Vector2Scale(p.b.vel, 0.3f * p.dt));
        if (p.w.cooldown <= 0) {
            p.w.cooldown = p.w.fire_rate; // Simple fire logic
        }
        if (p.w.cooldown > 0) p.w.cooldown -= p.dt;
    }
};

struct Drone {};
static CapabilityBaker<Drone, DroneStrike> g_DroneBaker;

// =============================================================================
// [ GAMEPLAY ] 4. BATTLEFIELD ENGINE
// =============================================================================

class BattlefieldEngine {
    entt::registry reg;
public:
    void Init() {
        for(int i=0; i<1000; i++) {
            auto e = reg.create();
            reg.emplace<Body>(e, Vector2{(float)GetRandomValue(0, 1280), (float)GetRandomValue(0, 720)}, Vector2{(float)GetRandomValue(-100, 100), (float)GetRandomValue(-100, 100)});
            reg.emplace<Weapon>(e, 0.0f, 0.5f);
            reg.emplace<CRGIdentity>(e, DenseTypeID<Drone>::Get());
            reg.emplace<Health>(e, 100.0f);
            reg.emplace<BehaviorSettings>(e, 25.0f);
            reg.emplace<Renderable>(e, SKYBLUE, 3.0f);
        }
    }

    void Update(float dt) {
        // Hot path: Logic Projection
        reg.view<CRGIdentity, Body, Weapon, BehaviorSettings>().each([&](auto entity, auto& id, auto& b, auto& w, auto& s) {
            IUnitAI::RuleContext ctx {}; 
            // O(1) Stateless Find
            if (auto* logic = BehaviorRouter<IUnitAI>::Find(id.logic_class, ctx, CombatState::Aggressive)) {
                IUnitAI::Params p { reg, entity, b, w, s, dt };
                logic->Execute(p);
            }
        });
    }

    void Render() {
        BeginDrawing(); ClearBackground(Color{10, 10, 20, 255});
        reg.view<Body, Renderable>().each([](const Body& b, const Renderable& r) {
            DrawCircleV(b.pos, r.size, r.color);
        });
        DrawFPS(10, 10);
        EndDrawing();
    }
};

int main() {
    InitWindow(1280, 720, "CRG BATTLEFIELD - STAGE 11");
    SetTargetFPS(60); 
    BattlefieldEngine engine; 
    engine.Init();
    
    while (!WindowShouldClose()) {
        engine.Update(GetFrameTime());
        engine.Render();
    }
    CloseWindow(); 
    return 0;
}