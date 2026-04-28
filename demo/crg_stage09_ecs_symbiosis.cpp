// Copyright (c) 2024-2026 Cyril Tissier. All rights reserved.
//
// =============================================================================
// CAPABILITY ROUTING GATEWAY (CRG) - STAGE 9: THE SYMBIOSIS (PURE ECS)
// =============================================================================
//
// @intent:
// Harmonize high-density Data-Oriented Design (ECS) with the compile-time 
// N-Dimensional tensor. The CRG owns the Logic Projection, while the ECS 
// owns the Data Pipeline.
//
// @architecture:
// - Zero Coupling: Interfaces are pure and take raw Component references.
// - N-Dimensional: Logic varies across N-axes (Time, Alert, Biome) in O(1).
// - Symbiosis: The ECS System extracts coordinates from components to 
//   query the CapabilityRouter.
// =============================================================================

#include <iostream>
#include <typeinfo>
#include <vector>
#include <tuple>
#include <span>

// =============================================================================
// 1. INFRASTRUCTURE (DLL SAFE)
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

// =============================================================================
// 2. N-DIMENSIONAL SPACE MATH
// =============================================================================

template<typename T> struct EnumTraits;

struct GlobalState { constexpr operator std::size_t() const { return 0; } };
template<> struct EnumTraits<GlobalState> { static constexpr std::size_t Count = 1; };

template<class... TAxes>
struct Space {
    static constexpr std::size_t Dimensions = sizeof...(TAxes);
    static constexpr std::size_t Volume = (EnumTraits<TAxes>::Count * ... * 1);

    template<std::size_t DimIdx>
    static constexpr std::size_t GetStride() {
        std::size_t stride = 1;
        auto calc = [&](std::size_t i) {
            if (i > DimIdx) stride *= EnumTraits<std::tuple_element_t<i, std::tuple<TAxes...>>>::Count;
        };
        []<std::size_t... Is>(std::index_sequence<Is...>, auto f) { (f(Is), ...); }(std::make_index_sequence<Dimensions>{}, calc);
        return stride;
    }

    template<std::size_t DimIdx>
    static constexpr auto GetCoordAtIndex(std::size_t index) {
        using AxisT = std::tuple_element_t<DimIdx, std::tuple<TAxes...>>;
        return static_cast<AxisT>((index / GetStride<DimIdx>()) % EnumTraits<AxisT>::Count);
    }

    static constexpr std::size_t ComputeOffset(TAxes... coords) {
        std::size_t offset = 0;
        ((offset = offset * EnumTraits<TAxes>::Count + static_cast<std::size_t>(coords)), ...);
        return offset;
    }
};

template<> struct Space<> { static constexpr std::size_t Dimensions = 0; static constexpr std::size_t Volume = 1; };

// --- TOPOLOGY & COORDINATES ---
template<class TInterface> struct CapabilityTopology { using SpaceType = Space<GlobalState>; };

template<auto... Values> struct At {};

template<class TSpace, std::size_t Index, std::size_t... DimIs>
auto MakeAtFromIndex(std::index_sequence<DimIs...>) {
    return At<TSpace::template GetCoordAtIndex<DimIs>(Index)...>{};
}

// =============================================================================
// 3. CORE TYPES
// =============================================================================

using ModelTypeID = std::size_t;
using InterfaceTypeID = std::size_t; 
template<class T> struct TypeIDOf { static std::size_t Get() { return typeid(T).hash_code(); } };
template<typename... Ts> struct TypeList {};

template<class TInterface> 
struct Capability : public TInterface { using InterfaceType = TInterface; };

struct IRegistryNode {
    virtual ~IRegistryNode() = default;
    virtual ModelTypeID GetTargetModelID() const = 0;
    virtual const void* ResolveSpanRaw(InterfaceTypeID interfaceID) const = 0;
};

using RegistryVector = std::vector<const IRegistryNode*>;
using RouterSlot = RegistrySlot<RegistryVector>;
CRG_BIND_SLOT(RegistryVector)

struct IAssembler { virtual void Assemble(RegistryVector& registry) const = 0; };
struct IBakerNode : public NodeList<IBakerNode, IAssembler> {};
CRG_BIND_SLOT(const IBakerNode*)

// =============================================================================
// 4. THE BAKER (TENSOR COMPILER)
// =============================================================================

template<class TModel, template<class, class> class Cap, class IdxSeq>
struct CapabilityNode;

template<class TModel, template<class, class> class Cap, std::size_t... Is>
struct CapabilityNode<TModel, Cap, std::index_sequence<Is...>> 
    : public Cap<TModel, decltype(MakeAtFromIndex<typename CapabilityTopology<typename Cap<TModel, At<>>::InterfaceType>::SpaceType, Is>(std::make_index_sequence<CapabilityTopology<typename Cap<TModel, At<>>::InterfaceType>::SpaceType::Dimensions>{}))>... 
{
    using Intf = typename Cap<TModel, At<>>::InterfaceType;
    using TSpace = typename CapabilityTopology<Intf>::SpaceType;
    static constexpr std::size_t Size = TSpace::Volume;
    const Intf* m_buffer[Size];

    CapabilityNode() {
        const Intf* ptrs[] = { static_cast<const Intf*>(static_cast<const Cap<TModel, decltype(MakeAtFromIndex<TSpace, Is>(std::make_index_sequence<TSpace::Dimensions>{}))>*>(this))... };
        for(std::size_t i=0; i<Size; ++i) m_buffer[i] = ptrs[i];
    }
    const void* GetSpanRaw() const { return m_buffer; }
};

template<class TModel, template<class, class> class... TCapabilities>
class CapabilitySpace : public IRegistryNode, 
    public CapabilityNode<TModel, TCapabilities, std::make_index_sequence<CapabilityTopology<typename TCapabilities<TModel, At<>>::InterfaceType>::SpaceType::Volume>>... 
{
public:
    ModelTypeID GetTargetModelID() const override { return TypeIDOf<TModel>::Get(); }
    const void* ResolveSpanRaw(InterfaceTypeID interfaceID) const override {
        const void* result = nullptr;
        (void)((interfaceID == TypeIDOf<typename TCapabilities<TModel, At<>>::InterfaceType>::Get() && 
         (result = static_cast<const CapabilityNode<TModel, TCapabilities, std::make_index_sequence<CapabilityTopology<typename TCapabilities<TModel, At<>>::InterfaceType>::SpaceType::Volume>>*>(this)->GetSpanRaw())), ...);
        return result;
    }
};

template<class TModel, template<class, class> class... TCapabilities>
struct CapabilityBaker : public IBakerNode {
    CapabilitySpace<TModel, TCapabilities...> m_unit;
    void Assemble(RegistryVector& registry) const override { registry.push_back(&m_unit); }
};

template<class... Models, template<class, class> class... TCapabilities>
struct CapabilityBaker<TypeList<Models...>, TCapabilities...> : public CapabilityBaker<Models, TCapabilities...>... {
    void Assemble(RegistryVector& registry) const override { (CapabilityBaker<Models, TCapabilities...>::Assemble(registry), ...); }
};

// =============================================================================
// 5. THE ROUTER
// =============================================================================

class CapabilityRouter {
private:
    static void EnsureBaked() { struct StaticGuard { StaticGuard() { CapabilityRouter::Bake(); } }; static StaticGuard s_Guard; }
public:
    static void Bake() {
        RouterSlot::s_Value.clear();
        for (auto* b = NodeListAnchor<IBakerNode>::s_Value; b; b = b->m_Next) b->Assemble(RouterSlot::s_Value);
    }

    template<class InterfaceT, class... Coords>
    static const InterfaceT* Find(ModelTypeID modelID, Coords... coords) {
        EnsureBaked();
        using TSpace = typename CapabilityTopology<InterfaceT>::SpaceType;
        std::size_t offset = 0;
        if constexpr (sizeof...(Coords) > 0) offset = TSpace::ComputeOffset(coords...);

        for (const auto* node : RouterSlot::s_Value) {
            if (node->GetTargetModelID() == modelID) {
                if (const void* ptr = node->ResolveSpanRaw(TypeIDOf<InterfaceT>::Get())) 
                    return static_cast<const InterfaceT* const*>(ptr)[offset];
            }
        }
        return nullptr;
    }
};

// =============================================================================
// 6. GAMEPLAY SYMBIOSIS (ECS + CRG)
// =============================================================================

// --- ECS COMPONENTS ---
struct LogicID { ModelTypeID hash; };
struct Battery { float charge = 100.0f; };
struct Biome   { enum Type { Desert, Tundra } zone; };

// --- CRG CONTEXT ---
enum class WorldState { Day, Night };
template<> struct EnumTraits<WorldState> { static constexpr std::size_t Count = 2; };
template<> struct EnumTraits<Biome::Type> { static constexpr std::size_t Count = 2; };

// --- PURE CONTRACT (DOD) ---
struct IEnergyDrain {
    virtual void Execute(Battery& bat) const = 0;
};

// --- TOPOLOGY (2D Axis: WorldState + Biome) ---
template<> struct CapabilityTopology<IEnergyDrain> { 
    using SpaceType = Space<WorldState, Biome::Type>; 
};

// --- BEHAVIORS ---
template<class T, class TAt = At<>> struct DrainLogic : Capability<IEnergyDrain> {};

// Specialized: Night + Tundra (Freezing penalty)
template<class T, auto... R>
struct DrainLogic<T, At<WorldState::Night, Biome::Tundra, R...>> : Capability<IEnergyDrain> {
    void Execute(Battery& bat) const override { bat.charge -= 10.0f; }
};

// Fallback / Standard
template<class T, class TAt>
struct DrainLogic : Capability<IEnergyDrain> {
    void Execute(Battery& bat) const override { bat.charge -= 1.0f; }
};

// --- REGISTRATION ---
struct Scout {};
struct HeavyLifter {};
using AirModels = TypeList<Scout, HeavyLifter>;
static const CapabilityBaker<AirModels, DrainLogic> g_DrainBaker{};

// =============================================================================
// 7. ECS SYSTEM (The Hot Path)
// =============================================================================

struct EnergySystem {
    // Mimicking ECS SoA Storage
    std::vector<LogicID> logic_ids;
    std::vector<Battery> batteries;
    std::vector<Biome>   biomes;

    void Update(WorldState currentTime) {
        for (std::size_t i = 0; i < logic_ids.size(); ++i) {
            // 1. Extract context from components and global state
            ModelTypeID id = logic_ids[i].hash;
            Biome::Type zone = biomes[i].zone;

            // 2. O(1) Routing through the Hypergraph
            auto* behavior = CapabilityRouter::Find<IEnergyDrain>(id, currentTime, zone);

            // 3. Execute directly on component data (No object, pure DOD)
            if (behavior) behavior->Execute(batteries[i]);
        }
    }
};

// =============================================================================
// MAIN
// =============================================================================
int main() {
    EnergySystem ecs;
    
    // Add a Scout in the Desert
    ecs.logic_ids.push_back({ TypeIDOf<Scout>::Get() });
    ecs.batteries.push_back({ 100.0f });
    ecs.biomes.push_back({ Biome::Desert });

    // Add a HeavyLifter in the Tundra
    ecs.logic_ids.push_back({ TypeIDOf<HeavyLifter>::Get() });
    ecs.batteries.push_back({ 100.0f });
    ecs.biomes.push_back({ Biome::Tundra });

    std::cout << "--- CRG STAGE 9: ECS SYMBIOSIS ---\n\n";

    std::cout << "Frame 1: Day Time\n";
    ecs.Update(WorldState::Day);
    std::cout << "  Scout Battery: " << ecs.batteries[0].charge << "%\n";
    std::cout << "  Heavy Battery: " << ecs.batteries[1].charge << "%\n";

    std::cout << "\nFrame 2: Night Time (Tundra penalty triggers for HeavyLifter)\n";
    ecs.Update(WorldState::Night);
    std::cout << "  Scout Battery: " << ecs.batteries[0].charge << "%\n";
    std::cout << "  Heavy Battery: " << ecs.batteries[1].charge << "% (FROZEN)\n";

    return 0;
}