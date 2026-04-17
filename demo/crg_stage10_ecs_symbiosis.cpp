// ======================================================
// STAGE 10 — THE SYMBIOSIS (ECS + CRG)
// ======================================================
//
// @intent:
// Demonstrate the integration of the CRG model within a Data-Oriented 
// architecture (ECS).
//
// @key_insight:
// The ECS manages data locality and fast iteration. The CRG manages 
// contextual behavior projection. They are perfectly symbiotic: 
// The CRG allows changing behavior without mutating ECS archetypes.
//
// @spoken_line:
// “The ECS is the body; it runs through memory. The CRG is the mind; 
// it projects purpose onto that data without ever touching its structure.”
// ======================================================

#include <iostream>
#include <vector>
#include <string>
#include <typeinfo>
#include <type_traits>
#include <cstddef>   // Requis pour std::size_t, std::byte, std::max_align_t
#include <utility>   // Requis pour std::move

// --- CRG CORE (SBO & TYPE ID) ---
using TypeID = std::size_t;
template<class T> struct TypeIDOf { static TypeID Get() { return typeid(T).hash_code(); } };

class HardwareHandle {
    // SBO étendu à 64 pour absorber la taille de std::string sur tous les OS
    static constexpr std::size_t SBO_SIZE = 64; 
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
    TypeID GetTypeID() const { return reinterpret_cast<const Concept*>(&m_buffer)->GetTypeID(); }
    template<class T> const T& Get() const { return static_cast<const Model<T>*>(reinterpret_cast<const Concept*>(&m_buffer))->value; }
private:
    void Reset() { if (m_set) { reinterpret_cast<Concept*>(&m_buffer)->~Concept(); m_set = false; } }
    alignas(std::max_align_t) std::byte m_buffer[SBO_SIZE]; bool m_set = false;
};

// --- CRG STRUCTURE (INTRUSIVE NODES) ---
template<class Interface>
class LinkedNode {
public:
    LinkedNode() { m_next = s_head; s_head = static_cast<const Interface*>(this); }
    const Interface* GetNext() const { return m_next; }
    static const Interface* GetHead() { return s_head; }
private:
    inline static const Interface* s_head = nullptr;
    const Interface* m_next = nullptr;
};

// --- AXES & INTERFACE ---
enum class WorldState { Day, Night };

struct IUnitAI : LinkedNode<IUnitAI> {
    virtual ~IUnitAI() = default;
    virtual void Update(const HardwareHandle&) const = 0;
    virtual bool Match(TypeID id, WorldState ws) const = 0;
};

// --- DOMAIN DATA ---
struct Drone       { std::string name; float battery; };
struct HeavyLifter { std::string name; float cargo_load; };

// --- BEHAVIOR DEFINITIONS ---
template<class T>
struct BaseAI : IUnitAI {
    bool Match(TypeID id, WorldState ws) const override { 
        return id == TypeIDOf<T>::Get() && ws == WorldState::Day; 
    }
    void Update(const HardwareHandle& h) const override {
        std::cout << "[Day Mode] " << h.Get<T>().name << " is patrolling.\n";
    }
};

template<class T>
struct StealthAI : IUnitAI {
    bool Match(TypeID id, WorldState ws) const override { 
        return id == TypeIDOf<T>::Get() && ws == WorldState::Night; 
    }
    void Update(const HardwareHandle& h) const override {
        std::cout << "[Night Stealth] 🌑 " << h.Get<T>().name << " dims lights and enables silent rotors.\n";
    }
};

// --- THE MATRIX ---
template<class...> struct TypeList;

template<class ModelT, template<class> class... Defs> 
struct BehaviorMatrixSingle : public Defs<ModelT>... {};

template<class ModelList, template<class> class... Defs> 
struct BehaviorMatrix;

template<class... Models, template<class> class... Defs>
struct BehaviorMatrix<TypeList<Models...>, Defs...> : public BehaviorMatrixSingle<Models, Defs...>... {
    template<class Interface, class... Args>
    static const Interface* Resolve(TypeID id, Args... args) {
        for (const Interface* n = Interface::GetHead(); n != nullptr; n = n->GetNext()) {
            if (n->Match(id, args...)) return n;
        }
        return nullptr;
    }
};

// Instanciation Globale Immuable
using AIDomain = BehaviorMatrix<TypeList<Drone, HeavyLifter>, BaseAI, StealthAI>;
inline static const AIDomain g_behavior_matrix{};

// ======================================================
// MOCK ECS (The "Body" - Structure of Arrays approach)
// ======================================================
// In a real ECS (like EnTT), storage is handled dynamically via sparse sets.
// Here we mock the DoD approach with direct typed vectors (SoA).
class SimpleRegistry {
public:
    std::vector<Drone> drones;
    std::vector<HeavyLifter> lifters;

    void CreateDrone(std::string n) { drones.push_back(Drone{n, 100.0f}); }
    void CreateLifter(std::string n) { lifters.push_back(HeavyLifter{n, 500.0f}); }
};

// ======================================================
// THE SYSTEM (The Symbiosis)
// ======================================================

// The System iterates over a specific typed View. 
// ZERO if/else chains. ZERO type casting.
template<class T>
void ProcessAIView(const std::vector<T>& view, WorldState current_time) {
    for (const auto& entity_data : view) {
        
        // 1. We prepare the Handle on the stack (Zero Heap Allocation)
        HardwareHandle handle;
        
        // Perfect forwarding of the exact type. No runtime type checking needed!
        handle.Set(entity_data); 

        // 2. The CRG projects the behavior WITHOUT mutating the ECS archetype
        if (auto* ai = AIDomain::Resolve<IUnitAI>(TypeIDOf<T>::Get(), current_time)) {
            ai->Update(handle);
        }
    }
}

void AISystem_UpdateAll(const SimpleRegistry& reg, WorldState current_time) {
    std::cout << "\n--- System Update (Context: " 
              << (current_time == WorldState::Day ? "Day" : "Night") << ") ---\n";
    
    // In a real ECS: auto view = registry.view<Drone>();
    ProcessAIView(reg.drones, current_time);
    ProcessAIView(reg.lifters, current_time);
}

int main() {
    SimpleRegistry world;
    world.CreateDrone("Scout-Alpha");
    world.CreateLifter("Atlas-Omega");

    // Simulation of the Day/Night cycle
    AISystem_UpdateAll(world, WorldState::Day);
    AISystem_UpdateAll(world, WorldState::Night);

    return 0;
}
