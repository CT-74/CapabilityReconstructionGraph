// Copyright (c) 2024-2026 Cyril Tissier. All rights reserved.
// Licensed under the Apache License, Version 2.0.
// See LICENSE file in the project root for full license information.
//
// ======================================================
// STAGE 08 — THE TEMPORAL AXIS (1D PROJECTION)
// ======================================================
//
// @intent:
// Introduce a generic axis projection (Temporal) while keeping the GPP API 
// absolutely pristine and stable.
//
// @what_changed:
// - The Engine hides the `span` internally. 
// - The GPP uses a unified API: GetBehavior<Interface>(modelID, coordinate).
// - Default parameters allow 0D interfaces to be queried without coordinates.
// - Strict O(1) resolution remains intact under the hood.
// ======================================================

#include <iostream>
#include <typeinfo>
#include <string>
#include <cstddef>
#include <new>
#include <tuple>
#include <span>

// ======================================================
// [ ENGINE CORE ] 1. IDENTITY & GENERICS
// ======================================================
using TypeID = std::size_t;
template<class T> struct TypeIDOf { static TypeID Get() { return typeid(T).hash_code(); } };
template<typename... Ts> struct TypeList {};

// The Code Gen trait template
template<typename T> struct EnumTraits;

// A generic axis for Interfaces that do NOT depend on Time or Biomes (0D)
struct MonoState { constexpr operator std::size_t() const { return 0; } };
template<> struct EnumTraits<MonoState> { static constexpr std::size_t Count = 1; };


// ======================================================
// [ ENGINE CORE ] 2. TYPE ERASED SHELL
// ======================================================
class HardwareHandle {
    static constexpr std::size_t SBO_SIZE = 48;
public:
    struct Concept { virtual ~Concept() = default; virtual TypeID GetTypeID() const = 0; };
    template<class T> struct Model : Concept {
        T value; Model(T v) : value(std::move(v)) {}
        TypeID GetTypeID() const override { return TypeIDOf<T>::Get(); }
    };
    HardwareHandle() = default; ~HardwareHandle() { Reset(); }
    template<class T> void Set(T v) {
        static_assert(sizeof(Model<T>) <= SBO_SIZE, "SBO Capacity Exceeded!");
        Reset(); new (&m_buffer) Model<T>(std::move(v)); m_set = true;
    }
    TypeID GetTypeID() const { return m_set ? reinterpret_cast<const Concept*>(&m_buffer)->GetTypeID() : 0; }
private:
    void Reset() { if(m_set) { reinterpret_cast<Concept*>(&m_buffer)->~Concept(); m_set = false; } }
    alignas(std::max_align_t) std::byte m_buffer[SBO_SIZE]; bool m_set = false;
};


// ======================================================
// [ ENGINE CORE ] 3. EXTERNAL POLYMORPHISM ROUTER
// ======================================================
struct IModelRegistryNode {
    IModelRegistryNode* next = nullptr;
    virtual TypeID GetModelTypeID() const = 0;
    virtual const void* ResolveSpanRaw(TypeID interfaceID) const = 0;
};

class BehaviorMatrixList {
    static inline IModelRegistryNode* s_head = nullptr;

public:
    static void Register(IModelRegistryNode* node) {
        node->next = s_head;
        s_head = node;
    }

    // THE UNIFIED ECS API: Hides the tensor, returns the exact pointer in O(1).
    // Uses '= {}' so 0D interfaces can be called without arguments.
    template<class InterfaceT>
    static const InterfaceT* GetBehavior(TypeID modelID, typename InterfaceT::AxisType coordinate = {}) {
        for (auto* curr = s_head; curr; curr = curr->next) {
            if (curr->GetModelTypeID() == modelID) {
                if (const void* ptr = curr->ResolveSpanRaw(TypeIDOf<InterfaceT>::Get())) {
                    // Reconstruct the span to ensure bounds (or just do pointer arithmetic)
                    std::span<const InterfaceT* const> span(
                        static_cast<const InterfaceT* const*>(ptr), 
                        EnumTraits<typename InterfaceT::AxisType>::Count
                    );
                    return span[static_cast<std::size_t>(coordinate)];
                }
            }
        }
        return nullptr;
    }
};


// ======================================================
// [ ENGINE CORE ] 4. THE CAPABILITY NODE & BAKER
// ======================================================
template<class ModelT, template<class> class... Behaviors>
class BakedCapabilityNode : public IModelRegistryNode, public Behaviors<ModelT>... {
    
    template<class InterfaceT>
    struct SpanBaker {
        static constexpr std::size_t Size = EnumTraits<typename InterfaceT::AxisType>::Count;
        const InterfaceT* buffer[Size] = {nullptr};
        
        SpanBaker(const BakedCapabilityNode* node) {
            ((std::is_same_v<InterfaceT, typename Behaviors<ModelT>::InterfaceType> ? 
                (void)(buffer[static_cast<std::size_t>(Behaviors<ModelT>::State)] = static_cast<const InterfaceT*>(static_cast<const Behaviors<ModelT>*>(node))) 
                : (void)0), ...);
        }
    };

    template<class InterfaceT>
    const void* GetSpanBuffer() const {
        static SpanBaker<InterfaceT> baker(this);
        return baker.buffer;
    }

public:
    BakedCapabilityNode() { BehaviorMatrixList::Register(this); }
    TypeID GetModelTypeID() const override { return TypeIDOf<ModelT>::Get(); }

    const void* ResolveSpanRaw(TypeID interfaceID) const override {
        const void* result = nullptr;
        ((interfaceID == TypeIDOf<typename Behaviors<ModelT>::InterfaceType>::Get() && 
         (result = GetSpanBuffer<typename Behaviors<ModelT>::InterfaceType>())), ...);
        return result;
    }
};

template<class ModelList, template<class> class... Behaviors>
struct DomainBaker;

template<class... Models, template<class> class... Behaviors>
struct DomainBaker<TypeList<Models...>, Behaviors...> {
    std::tuple<BakedCapabilityNode<Models, Behaviors...>...> m_nodes;
};


// ======================================================
// [ GAMEPLAY SPACE ] 5. GPP DEFINITIONS
// ======================================================

// The Axis
enum class WorldState : std::size_t { Day, Night };
template<> struct EnumTraits<WorldState> { static constexpr std::size_t Count = 2; };

// The Models
struct Drone { std::string name; };
struct HeavyLifter { std::string name; };

// The Interfaces
struct IDiagnostic {
    using InterfaceType = IDiagnostic;
    using AxisType = MonoState; // 0D
    virtual void Execute(const HardwareHandle& h) const = 0;
};

struct IMovement {
    using InterfaceType = IMovement;
    using AxisType = WorldState; // 1D
    virtual void Move(const HardwareHandle& h) const = 0;
};

// The Behaviors
template<class T>
struct DiagnosticBehavior : IDiagnostic {
    static constexpr MonoState State = {};
    void Execute(const HardwareHandle& h) const override { std::cout << "[0D] Running Diagnostics.\n"; }
};

template<class T>
struct DayMovementBehavior : IMovement {
    static constexpr WorldState State = WorldState::Day;
    void Move(const HardwareHandle& h) const override { std::cout << "[1D - DAY] Visual Patrol.\n"; }
};

template<class T>
struct NightMovementBehavior : IMovement {
    static constexpr WorldState State = WorldState::Night;
    void Move(const HardwareHandle& h) const override { std::cout << "[1D - NIGHT] Infrared Stealth.\n"; }
};


// ======================================================
// [ GAMEPLAY SPACE ] 6. MODULE INSTANTIATIONS
// ======================================================
using SimulationModels = TypeList<Drone, HeavyLifter>;

static const DomainBaker<SimulationModels, DiagnosticBehavior> g_DiagBaker{};
static const DomainBaker<SimulationModels, DayMovementBehavior, NightMovementBehavior> g_MovementBaker{};


// ======================================================
// USER CODE (ECS Hot Path Simulation)
// ======================================================
int main() {
    HardwareHandle h; 
    h.Set(Drone{"Scout-1"});

    std::cout << "--- STAGE 08: THE TEMPORAL AXIS (UNIFIED API) ---\n\n";
    
    TypeID currentModelID = h.GetTypeID();
    WorldState currentTime = WorldState::Day;

    std::cout << "--- TIME: DAY ---\n";
    
    // API is completely agnostic of the underlying span/buffer
    if (auto* move = BehaviorMatrixList::GetBehavior<IMovement>(currentModelID, currentTime)) {
        move->Move(h);
    }
    
    // 0D Interfaces don't even need the coordinate argument thanks to the default parameter
    if (auto* diag = BehaviorMatrixList::GetBehavior<IDiagnostic>(currentModelID)) {
        diag->Execute(h);
    }

    currentTime = WorldState::Night;
    std::cout << "\n--- TIME: NIGHT ---\n";
    
    if (auto* move = BehaviorMatrixList::GetBehavior<IMovement>(currentModelID, currentTime)) {
        move->Move(h);
    }
    if (auto* diag = BehaviorMatrixList::GetBehavior<IDiagnostic>(currentModelID)) {
        diag->Execute(h);
    }

    return 0;
}