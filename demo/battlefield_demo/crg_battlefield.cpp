// Copyright (c) 2026 Cyril Tissier. All rights reserved.
// Licensed under the Apache License, Version 2.0.
//
// =============================================================================
// CAPABILITY ROUTING GATEWAY (CRG) - FINAL STAGE: BATTLEFIELD DEMO
// =============================================================================
//
// @intent:
// Visual demonstration of the Stage 11 Pure DOD Architecture vs Traditional ECS.
// - SPACE: Toggle between CRG Stateless Routing and ECS Structural Mutation.
// - UP/DOWN: Adjust the mutation rate (how many entities change state per frame).
// - W: Spawn 500 more entities.
// - I: Toggle immortality (visual test for the Panic/Flee capability).
// - S: Toggle Global Swarm Strategy (Formation / Scatter).
// =============================================================================

#include "raylib.h"
#include "raymath.h"  
#include <entt/entt.hpp>
#include <vector>
#include <tuple>
#include <typeinfo>
#include <algorithm>
#include <unordered_map>
#include <iostream>
#include <chrono>
#include <type_traits>
#include <utility>

// =============================================================================
// [ ENGINE CORE ] 1. IDENTITY & DLL-SAFE INFRASTRUCTURE
// =============================================================================

#ifndef CRG_DLL_ENABLED
#define CRG_DLL_ENABLED 0 
#endif

template<class T>
struct UniversalAnchor {
#if !CRG_DLL_ENABLED
    static T& Get() {
        static T s_Value{};
        return s_Value;
    }
#else
    static T& Get();
#endif
};

#if CRG_DLL_ENABLED
    #define CRG_DEFINE_UNIVERSAL_ANCHOR(T) \
        template<> T& UniversalAnchor<T>::Get() { \
            static T s_Value{}; \
            return s_Value; \
        }
#else
    #define CRG_DEFINE_UNIVERSAL_ANCHOR(T) 
#endif

using ModelTypeID = std::size_t; 

template<class T> 
struct TypeIDOf { 
    static ModelTypeID Get() { return typeid(T).hash_code(); } 
};

struct DenseModelID {
    std::size_t index;
    explicit DenseModelID(std::size_t i) : index(i) {}
    static constexpr std::size_t Invalid = static_cast<std::size_t>(-1);
    bool IsValid() const { return index != Invalid; }
    operator std::size_t() const { return index; }
};

using ModelMap = std::unordered_map<ModelTypeID, std::size_t>; 
CRG_DEFINE_UNIVERSAL_ANCHOR(ModelMap)

class CapabilityRouter; 

struct ModelHandle {
    DenseModelID denseID;
    explicit ModelHandle(ModelTypeID hash);
    template<class T> static ModelHandle FromType() { return ModelHandle(TypeIDOf<T>::Get()); }
};

template<class TNode> using NodeListAnchor = UniversalAnchor<const TNode*>; 
template<class TNode, class TInterface>
struct NodeList : public TInterface {
    const TNode* m_Next = nullptr;
    NodeList() {
        m_Next = NodeListAnchor<TNode>::Get();
        NodeListAnchor<TNode>::Get() = static_cast<const TNode*>(this);
    }
};

// =============================================================================
// [ ENGINE CORE ] 2. TENSOR MATH (HORNER'S METHOD)
// =============================================================================

template<typename T> struct EnumTraits;
struct GlobalState { constexpr operator std::size_t() const { return 0; } };
template<> struct EnumTraits<GlobalState> { static constexpr std::size_t Count = 1; };

template<class... TAxes>
struct CapabilitySpace {
    using AxisTuple = std::tuple<TAxes...>;
    static constexpr std::size_t Dimensions = sizeof...(TAxes);
    static constexpr std::size_t Volume = (Dimensions == 0) ? 1 : (EnumTraits<TAxes>::Count * ... * 1);

    template<std::size_t DimIdx>
    static constexpr std::size_t GetStride() {
        if constexpr (Dimensions == 0) return 1;
        else {
            constexpr std::size_t dims[] = { EnumTraits<TAxes>::Count... };
            std::size_t stride = 1;
            for (std::size_t i = DimIdx + 1; i < Dimensions; ++i) stride *= dims[i];
            return stride;
        }
    }

    template<std::size_t DimIdx>
    static constexpr auto GetCoordAtIndex(std::size_t index) {
        if constexpr (Dimensions == 0) return 0;
        else {
            using AxisT = std::tuple_element_t<DimIdx, AxisTuple>;
            return static_cast<AxisT>((index / CapabilitySpace<TAxes...>::template GetStride<DimIdx>()) % EnumTraits<AxisT>::Count);
        }
    }

    template<typename... TArgs>
    static constexpr std::size_t ComputeOffset(const TArgs&... args) {
        if constexpr (Dimensions == 0) return 0;
        else return ComputeInternal(std::tie(args...), std::make_index_sequence<Dimensions>{});
    }

private:
    template<typename TupleT, std::size_t... Is>
    static constexpr std::size_t ComputeInternal(const TupleT& t, std::index_sequence<Is...>) {
        const std::size_t coords[] = { static_cast<std::size_t>(std::get<const std::tuple_element_t<Is, AxisTuple>&>(t))... };
        constexpr std::size_t dims[] = { EnumTraits<std::tuple_element_t<Is, AxisTuple>>::Count... };
        std::size_t offset = 0;
        for (std::size_t i = 0; i < Dimensions; ++i) offset = offset * dims[i] + coords[i];
        return offset;
    }
};

template<class TContract> struct CapabilityRoutingTraits { using SpaceType = CapabilitySpace<GlobalState>; }; 
template<auto... Values> struct At {};

template<class TSpace, std::size_t Index, class IdxSeq> struct MakeAt;
template<class TSpace, std::size_t Index, std::size_t... DimIs>
struct MakeAt<TSpace, Index, std::index_sequence<DimIs...>> {
    using Type = At<TSpace::template GetCoordAtIndex<DimIs>(Index)...>;
};
template<class TSpace, std::size_t Index>
using MakeAt_t = typename MakeAt<TSpace, Index, std::make_index_sequence<TSpace::Dimensions>>::Type;

// =============================================================================
// [ ENGINE CORE ] 3. ROUTING TYPES & DOD DESCRIPTOR
// =============================================================================

struct NullContext {
    NullContext() = default;
    template<typename... Args> NullContext(const Args&...) {} 
};

template<typename T, typename = void> struct ContextSelector { using Type = NullContext; };
template<typename T> struct ContextSelector<T, std::void_t<typename CapabilityRoutingTraits<T>::RuleContext>> {
    using Type = typename CapabilityRoutingTraits<T>::RuleContext;
};

template<typename T> using ContextTypeOf = typename ContextSelector<T>::Type;

template<typename T, typename = void> 
struct IsDODContract : std::false_type {};

template<typename T> 
struct IsDODContract<T, std::void_t<typename T::Params>> 
    : std::bool_constant<!std::is_polymorphic_v<T>> {};

template<class TContract>
struct DODDescriptor {
    using ParamsT = typename TContract::Params;
    void (*pfnExecute)(ParamsT&); 
    inline void Execute(ParamsT& p) const { if (pfnExecute) pfnExecute(p); } 
};

template<class TContract>
struct Rule {
    using ContextT = ContextTypeOf<TContract>;
    using PredicatePtr = bool (*)(const void*, const ContextT&);

    DODDescriptor<TContract> descriptor; 
    const void* configData;
    PredicatePtr             predicate;
    int                      priority;

    bool Matches(const ContextT& ctx) const { return !predicate || predicate(configData, ctx); }
};

template<class TContract>
struct DispatchCell {
    std::vector<Rule<TContract>> dynamicRules;      
    DODDescriptor<TContract>     fallback;
    bool                         hasFallback = false;
};

template<class TContract> using TensorArena = UniversalAnchor<std::vector<DispatchCell<TContract>>>; 

template<class TContract, class TConfig = void> 
struct Capability { using InterfaceType = TContract; using ConfigType = TConfig; TConfig config; };
template<class TContract> 
struct Capability<TContract, void> { using InterfaceType = TContract; using ConfigType = void; };

template<typename T, typename = void> struct HasConfigType : std::false_type {};
template<typename T> struct HasConfigType<T, std::void_t<typename T::ConfigType>> { static constexpr bool value = !std::is_same_v<typename T::ConfigType, void>; };

// =============================================================================
// [ ENGINE CORE ] 4. THE BAKER (AUTO-EXTRACTION COMPILER)
// =============================================================================

template<typename... Ts> struct TypeList {}; 
struct IAssembler { virtual void Bake() const = 0; };
struct IBindingNode : public NodeList<IBindingNode, IAssembler> {};
CRG_DEFINE_UNIVERSAL_ANCHOR(const IBindingNode*) 

template<class TModel, template<class, class> class Cap, class TIdxSeq> 
struct CapabilityNode;

template<class TModel, template<class, class> class Cap, std::size_t... Is>
struct CapabilityNode<TModel, Cap, std::index_sequence<Is...>> 
    : public Cap<TModel, MakeAt_t<typename CapabilityRoutingTraits<typename Cap<TModel, At<>>::InterfaceType>::SpaceType, Is>>... 
{
    using ContractT = typename Cap<TModel, At<>>::InterfaceType;
    using TSpace = typename CapabilityRoutingTraits<ContractT>::SpaceType;
    using ContextT = ContextTypeOf<ContractT>;

    void FillArena(DenseModelID denseID) const {
        auto& arena = TensorArena<ContractT>::Get();
        std::size_t baseIdx = denseID.index * TSpace::Volume;
        if (arena.size() < baseIdx + TSpace::Volume) arena.resize(baseIdx + TSpace::Volume);

        ([&]() {
            using Impl = Cap<TModel, MakeAt_t<TSpace, Is>>;
            auto& cell = arena[baseIdx + Is];

            using ParamsT = typename ContractT::Params;
            DODDescriptor<ContractT> desc { &Impl::Execute };

            if constexpr (HasConfigType<Impl>::value) {
                auto trampoline = [](const void* obj, const ContextT& ctx) -> bool { return static_cast<const Impl*>(obj)->config.Condition(ctx); };
                cell.dynamicRules.push_back({ desc, &static_cast<const Impl*>(this)->config, trampoline, static_cast<const Impl*>(this)->config.priority });
                std::sort(cell.dynamicRules.begin(), cell.dynamicRules.end(), [](auto& a, auto& b){ return a.priority > b.priority; });
            } else {
                cell.fallback = desc;
                cell.hasFallback = true;
            }
        }(), ...);
    }
};

template<class TModel, template<class, class> class... TCapabilities>
struct CapabilityBinding : public IBindingNode {
    struct Unit : public CapabilityNode<TModel, TCapabilities, std::make_index_sequence<CapabilityRoutingTraits<typename TCapabilities<TModel, At<>>::InterfaceType>::SpaceType::Volume>>... {
        void Fill(DenseModelID slot) const { (CapabilityNode<TModel, TCapabilities, std::make_index_sequence<CapabilityRoutingTraits<typename TCapabilities<TModel, At<>>::InterfaceType>::SpaceType::Volume>>::FillArena(slot), ...); }
    } m_unit{};

    void Bake() const override {
        ModelTypeID hash = TypeIDOf<TModel>::Get();
        auto& map = UniversalAnchor<ModelMap>::Get();
        if (map.find(hash) == map.end()) map[hash] = map.size();
        m_unit.Fill(DenseModelID(map[hash]));
    }
};

template<class... Models, template<class, class> class... TCapabilities>
struct CapabilityBinding<TypeList<Models...>, TCapabilities...> : public CapabilityBinding<Models, TCapabilities...>... {
    void Bake() const override { (CapabilityBinding<Models, TCapabilities...>::Bake(), ...); }
};

// =============================================================================
// [ ENGINE CORE ] 5. BEHAVIOR ROUTER (O(1) DISPATCH)
// =============================================================================

class CapabilityRouter {
public:
    static void EnsureBaked() {
        static struct StaticGuard { 
            StaticGuard() { for (auto* b = NodeListAnchor<IBindingNode>::Get(); b; b = b->m_Next) b->Bake(); } 
        } s_guard;
    } 

    template<class TContract, class... Coords>
    static const DODDescriptor<TContract>* Find(ModelHandle handle, const ContextTypeOf<TContract>& ctx, Coords... coords) {
        EnsureBaked();
        if (!handle.denseID.IsValid()) return nullptr;

        const auto& arena = TensorArena<TContract>::Get(); 
        using TSpace = typename CapabilityRoutingTraits<TContract>::SpaceType;
        using TFullCtx = ContextTypeOf<TContract>;
        
        std::size_t idx = (handle.denseID.index * TSpace::Volume) + TSpace::ComputeOffset(coords...);
        if (idx >= arena.size()) return nullptr;

        const auto& cell = arena[idx];

        TFullCtx activeCtx = ctx; 
        for (const auto& rule : cell.dynamicRules) if (rule.Matches(activeCtx)) return &rule.descriptor;
        return cell.hasFallback ? &cell.fallback : nullptr;
    }
};

// =============================================================================
// [ ENGINE CORE ] 6. THE MODEL ROUTER (STATIC PROXY)
// =============================================================================

template<class ModelT>
class ModelRouter {
public:
    template<class TContract, class... Coords>
    static const DODDescriptor<TContract>* Find(const ContextTypeOf<TContract>& ctx, Coords... coords) {
        CapabilityRouter::EnsureBaked(); 
        return CapabilityRouter::Find<TContract>(ModelHandle::FromType<ModelT>(), ctx, coords...);
    }
};

// =============================================================================
// [ ENGINE CORE ] 7. ACTIVE CAPABILITY (ECS COMPONENT / GPP BRIDGE)
// =============================================================================

template<class T, bool IsDOD = IsDODContract<T>::value>
struct ActiveCapability;

template<class T>
struct ActiveCapability<T, false> {
    const T* resolvedPtr = nullptr;
    inline ActiveCapability& operator=(const T* ptr) { resolvedPtr = ptr; return *this; }
    template<auto FuncPtr, class... TArgs> inline void Invoke(TArgs&&... args) const { if (resolvedPtr) (resolvedPtr->*FuncPtr)(std::forward<TArgs>(args)...); }
    template<class... TArgs, typename = decltype(std::declval<const T&>()(std::declval<TArgs>()...))>
    inline void operator()(TArgs&&... args) const { if (resolvedPtr) (*resolvedPtr)(std::forward<TArgs>(args)...); }
    inline explicit operator bool() const { return resolvedPtr != nullptr; }
};

template<class T>
struct ActiveCapability<T, true> {
    void (*pfnResolved)(typename T::Params&) = nullptr; 
    inline ActiveCapability& operator=(const DODDescriptor<T>* desc) { pfnResolved = desc ? desc->pfnExecute : nullptr; return *this; }
    inline void Invoke(typename T::Params& params) const { if (pfnResolved) pfnResolved(params); }
    inline void operator()(typename T::Params& params) const { Invoke(params); }
    inline explicit operator bool() const { return pfnResolved != nullptr; }
};

inline ModelHandle::ModelHandle(ModelTypeID hash) : denseID(DenseModelID::Invalid) {
    CapabilityRouter::EnsureBaked(); 
    const auto& map = UniversalAnchor<ModelMap>::Get();
    auto it = map.find(hash);
    if (it != map.end()) denseID = DenseModelID(it->second);
}

// =============================================================================
// [ GAMEPLAY ] 1. DATA & PROFILING (ECS COMPONENTS)
// =============================================================================

struct ProfileResult { double physics_ms = 0; double ai_ms = 0; double struct_ms = 0; };
struct Body { Vector2 pos; Vector2 vel; };
struct Weapon { float cooldown; float fire_rate; };
struct Health { float current = 100.0f; bool is_dead = false; };
struct Renderable { Color color; float size; };
struct BehaviorSettings { float damage = 25.0f; };
struct Projectile { float lifespan = 1.5f; };

enum class CombatState { Idle, Aggressive };
template<> struct EnumTraits<CombatState> { static constexpr std::size_t Count = 2; };

enum class PerceptionRange { Melee, Ranged, Safe };
template<> struct EnumTraits<PerceptionRange> { static constexpr std::size_t Count = 3; };

enum class GroupStrategy { Formation, Scatter };
template<> struct EnumTraits<GroupStrategy> { static constexpr std::size_t Count = 2; };

struct CRGIdentity { 
    CombatState current_state = CombatState::Idle; 
    PerceptionRange current_perception = PerceptionRange::Safe;
    GroupStrategy current_strategy = GroupStrategy::Formation;
    ModelHandle handle = ModelHandle(TypeIDOf<void>::Get()); 
};

struct AggressiveTag {}; 
struct CommanderTag {}; // Identifying the leader

// =============================================================================
// [ GAMEPLAY ] 2. CONTRACTS & TRAITS
// =============================================================================

struct UnitAIContract {
    struct Params { entt::registry& reg; entt::entity e; Body& b; Weapon& w; Renderable& r; const BehaviorSettings& s; float dt; };
    struct RuleContext { const Health& health; }; 
};

template<> struct CapabilityRoutingTraits<UnitAIContract> { 
    using SpaceType = CapabilitySpace<CombatState, PerceptionRange, GroupStrategy>; 
    using RuleContext = UnitAIContract::RuleContext;
};

// =============================================================================
// [ GAMEPLAY ] 3. LOGIC IMPLEMENTATION (GPP POV)
// =============================================================================

// --- GENERIC SCOUT LOGIC ---
template<class T, class TAt = At<>>
struct DroneLogic : Capability<UnitAIContract> {
    static void Execute(UnitAIContract::Params& p) {
        p.r.color = SKYBLUE;
        if (p.w.cooldown > 0) p.w.cooldown -= p.dt;
    }
};

// Attack Logic
template<class T, auto... R>
struct DroneLogic<T, At<CombatState::Aggressive, PerceptionRange::Ranged, R...>> : Capability<UnitAIContract> {
    static void Execute(UnitAIContract::Params& p) {
        p.r.color = WHITE;
        if (p.w.cooldown <= 0) {
            p.w.cooldown = p.w.fire_rate; 
            auto proj = p.reg.create();
            Vector2 dir = Vector2Normalize(p.b.vel); 
            p.reg.emplace<Body>(proj, p.b.pos, Vector2Scale(dir, 600.0f)); 
            p.reg.emplace<Projectile>(proj, 1.5f);
            p.reg.emplace<Renderable>(proj, YELLOW, 2.0f); 
        }
        if (p.w.cooldown > 0) p.w.cooldown -= p.dt;
    }
};

// Scatter Logic Override
template<class T, auto... R>
struct DroneLogic<T, At<GroupStrategy::Scatter, R...>> : Capability<UnitAIContract> {
    static void Execute(UnitAIContract::Params& p) {
        p.r.color = VIOLET;
        p.b.pos.x += p.b.vel.x * p.dt * 2.0f; // Rapid movement when scattering
        p.b.pos.y += p.b.vel.y * p.dt * 2.0f;
    }
};

// --- COMMANDER LOGIC ---
template<class T, class TAt = At<>>
struct CommanderLogic : Capability<UnitAIContract> {
    static void Execute(UnitAIContract::Params& p) {
        p.r.color = GOLD;
        p.r.size = 6.0f; // Bigger visual profile
    }
};

struct PanicConfig {
    int priority = 100;
    bool Condition(const UnitAIContract::RuleContext& ctx) const { return ctx.health.current < 30.0f; }
};

template<class T, class TAt = At<>>
struct DroneFlee : Capability<UnitAIContract, PanicConfig> {
    static void Execute(UnitAIContract::Params& p) {
        p.r.color = ORANGE; 
        p.b.pos.x += p.b.vel.x * p.dt * 1.5f; 
        p.b.pos.y += p.b.vel.y * p.dt * 1.5f;
    }
};

struct Drone {};
struct Commander {};
using DroneFleet = TypeList<Drone>;
static const CapabilityBinding<DroneFleet, DroneLogic, DroneFlee> g_DroneBinding{};
static const CapabilityBinding<Commander, CommanderLogic> g_CommanderBinding{};

// =============================================================================
// [ GAMEPLAY ] 4. BATTLEFIELD ENGINE & BENCHMARK
// =============================================================================

class BattlefieldEngine {
    entt::registry reg;
    float mutation_rate = 5.0f;
    size_t frame_count = 0; 
    GroupStrategy global_strategy = GroupStrategy::Formation;
    entt::entity commander_entity = entt::null;
    
public:
    void Init() { 
        AddWave(1000); 
        DesignateCommander();
    }

    void DesignateCommander() {
        auto scouts = reg.view<CRGIdentity>();
        if (!scouts.empty()) {
            commander_entity = scouts.front();
            auto& id = reg.get<CRGIdentity>(commander_entity);
            id.handle = ModelHandle::FromType<Commander>();
            reg.emplace_or_replace<CommanderTag>(commander_entity);
        }
    }

    void AddWave(size_t count) {
        for(size_t i = 0; i < count; i++) {
            auto e = reg.create();
            reg.emplace<Body>(e, Vector2{(float)GetRandomValue(0, 1280), (float)GetRandomValue(0, 720)}, Vector2{(float)GetRandomValue(-150, 150), (float)GetRandomValue(-150, 150)});
            reg.emplace<Weapon>(e, 0.0f, 0.5f);
            CRGIdentity id;
            id.handle = ModelHandle::FromType<Drone>();
            id.current_state = CombatState::Idle;
            id.current_perception = PerceptionRange::Safe;
            id.current_strategy = global_strategy;
            reg.emplace<CRGIdentity>(e, id);
            reg.emplace<ActiveCapability<UnitAIContract>>(e);
            reg.emplace<Health>(e, 100.0f);
            reg.emplace<BehaviorSettings>(e, 25.0f);
            reg.emplace<Renderable>(e, SKYBLUE, 3.0f);
        }
    }
    
    void SetMutationRate(float r) { mutation_rate = r; }

    ProfileResult Update(float dt, bool use_crg, bool immortal) {
        ProfileResult p;
        frame_count++;

        // Physics
        auto t0 = std::chrono::high_resolution_clock::now();
        reg.view<Body>().each([dt](auto& b) {
            b.pos.x += b.vel.x * dt; b.pos.y += b.vel.y * dt;
            if (b.pos.x < 0 || b.pos.x > 1280) b.vel.x *= -1;
            if (b.pos.y < 0 || b.pos.y > 720) b.vel.y *= -1;
        });
        reg.view<Projectile>().each([&](auto entity, auto& proj) {
            proj.lifespan -= dt; if (proj.lifespan <= 0) reg.destroy(entity);
        });
        p.physics_ms = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - t0).count();

        // Structural mutations
        auto t2 = std::chrono::high_resolution_clock::now();
        size_t total = reg.storage<entt::entity>().size();
        size_t to_mutate = static_cast<size_t>(total * (mutation_rate / 100.0f));
        size_t mutated = 0;
        reg.view<CRGIdentity>().each([&](auto entity, auto& id) {
            if (mutated < to_mutate) {
                id.current_state = (GetRandomValue(0, 100) > 50) ? CombatState::Aggressive : CombatState::Idle;
                mutated++;
                if (!use_crg) {
                    if (id.current_state == CombatState::Aggressive) reg.emplace_or_replace<AggressiveTag>(entity);
                    else reg.remove<AggressiveTag>(entity);
                }
            }
        });
        
        // GLOBAL STRATEGY RESOLUTION (Commander input or S key)
        if (IsKeyPressed(KEY_S)) {
            global_strategy = (global_strategy == GroupStrategy::Formation) ? GroupStrategy::Scatter : GroupStrategy::Formation;
        }

        p.struct_ms = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - t2).count();

        // AI LOGIC
        auto t4 = std::chrono::high_resolution_clock::now();
        if (use_crg) {
            const size_t TOTAL_SLICES = 12;
            const size_t current_slice = frame_count % TOTAL_SLICES;

            reg.view<CRGIdentity, Body, Health, ActiveCapability<UnitAIContract>>().each([&](auto entity, auto& id, auto& b, auto& h, auto& cap) {
                if ((static_cast<uint32_t>(entity) % TOTAL_SLICES) == current_slice) {
                    // Update Strategy from swarm global state
                    id.current_strategy = global_strategy;

                    // Update Perception
                    float distSq = Vector2LengthSqr(Vector2Subtract(b.pos, Vector2{640, 360}));
                    if (distSq < 10000.0f) id.current_perception = PerceptionRange::Melee;
                    else if (distSq < 90000.0f) id.current_perception = PerceptionRange::Ranged;
                    else id.current_perception = PerceptionRange::Safe;

                    UnitAIContract::RuleContext ctx { h };
                    cap = CapabilityRouter::Find<UnitAIContract>(id.handle, ctx, id.current_state, id.current_perception, id.current_strategy);
                }
            });

            reg.view<ActiveCapability<UnitAIContract>, Body, Weapon, Renderable, BehaviorSettings>().each([&](auto entity, auto& cap, auto& b, auto& w, auto& r, auto& s) {
                UnitAIContract::Params params { reg, entity, b, w, r, s, dt };
                cap(params); 
            });
        }
        p.ai_ms = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - t4).count();

        // HEALTH & SUCCESSION
        if (!immortal) {
            reg.view<Health>().each([dt](auto& h) { h.current -= 5.0f * dt; });
            for(auto [entity, h] : reg.view<Health>().each()) {
                if (h.current <= 0) {
                    if (entity == commander_entity) commander_entity = entt::null;
                    reg.destroy(entity);
                }
            }
            // If commander is dead, designate a new one immediately
            if (commander_entity == entt::null || !reg.valid(commander_entity)) DesignateCommander();
        } else reg.view<Health>().each([](auto& h) { h.current = 100.0f; });

        return p;
    }

    void Render(ProfileResult p, bool crg, bool immortal) {
        BeginDrawing(); ClearBackground(Color{5, 5, 10, 255});
        
        Vector2 leaderPos = {0,0};
        if (reg.valid(commander_entity)) leaderPos = reg.get<Body>(commander_entity).pos;

        reg.view<Body, Renderable, Health, CRGIdentity>().each([&](auto entity, const Body& b, const Renderable& r, const Health& h, const CRGIdentity& id) {
            DrawCircleV(b.pos, r.size, Fade(r.color, std::max(0.2f, h.current / 100.0f)));
            // Visual Swarm Links
            if (crg && reg.valid(commander_entity) && entity != commander_entity) {
                if (id.current_strategy == GroupStrategy::Formation)
                    DrawLineV(b.pos, leaderPos, Fade(GOLD, 0.05f));
            }
        });

        DrawRectangle(0, 0, 520, 310, Fade(BLACK, 0.8f));
        size_t entity_count = reg.storage<entt::entity>().size();
        DrawText(TextFormat("PHYSICS: %.2f ms", p.physics_ms), 10, 10, 20, LIGHTGRAY); 
        DrawText(TextFormat("AI LOGIC: %.2f ms", p.ai_ms), 10, 40, 20, GREEN);
        DrawText(TextFormat("STRAT: %s", global_strategy == GroupStrategy::Formation ? "FORMATION" : "SCATTER"), 10, 70, 20, VIOLET);
        DrawText(TextFormat("ROUTING: %s", crg ? "CRG (Stateless)" : "ECS (Structural)"), 10, 100, 20, RAYWHITE);
        DrawText(TextFormat("MUTATION: %.1f %%", mutation_rate), 10, 130, 20, SKYBLUE);        
        DrawText(TextFormat("ENTITIES: %zu", entity_count), 10, 160, 20, RAYWHITE); 
        DrawText(TextFormat("IMMORTALITY: %s", immortal ? "ON" : "OFF"), 10, 190, 20, immortal ? GREEN : ORANGE); 
        DrawText("CONTROLS: [SPACE] Mode | [S] Strategy | [UP/DOWN] Mut | [W] +500", 10, 230, 15, LIGHTGRAY);
        DrawCircle(15, 275, 6, GOLD); DrawText("LEADER (Commander)", 30, 265, 18, GOLD);
        EndDrawing();
    }
};

int main() {
    InitWindow(1280, 720, "CRG BATTLEFIELD - STAGE 17 SWARM");
    SetTargetFPS(60); 
    BattlefieldEngine engine; engine.Init();
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