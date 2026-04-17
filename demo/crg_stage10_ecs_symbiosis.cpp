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
#include <cstddef>   // FIX: Requis pour std::size_t, std::byte, std::max_align_t
#include <utility>   // FIX: Requis pour std::move

// --- CRG CORE (SBO & TYPE ID) ---
using TypeID = std::size_t;
template<class T> struct TypeIDOf { static TypeID Get() { return typeid(T).hash_code(); } };

class HardwareHandle {
    // FIX: Passé de 48 à 64 pour absorber la taille de std::string sur tous les OS
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
// FIX: Normalisation des templates (1 seul paramètre T) pour être 100% C++17 compliant
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

// FIX: Utilisation de `template<class> class` au lieu de `template<class...> class`
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
// MOCK ECS (The "Body")
// ======================================================
struct Entity {
    TypeID type;
    std::string name;
    float special_value; // battery or cargo
};

class SimpleRegistry {
public:
    std::vector<Entity> entities;
    void CreateDrone(std::string n) { entities.push_back({TypeIDOf<Drone>::Get(), n, 100.0f}); }
    void CreateLifter(std::string n) { entities.push_back({TypeIDOf<HeavyLifter>::Get(), n, 500.0f}); }
};

// ======================================================
// THE SYSTEM (The Symbiosis)
// ======================================================
void AISystem_Update(SimpleRegistry& reg, WorldState current_time) {
    std::cout << "\n--- System Update (Context: " 
              << (current_time == WorldState::Day ? "Day" : "Night") << ") ---\n";

    for (auto& e : reg.entities) {
        // 1. On prépare le Handle sur la pile (Stack Allocation only, Zero Heap)
        HardwareHandle handle;
        
        // On reconstruit l'identité à partir de la data brute de l'ECS
        if (e.type == TypeIDOf<Drone>::Get()) 
            handle.Set(Drone{e.name, e.special_value});
        else if (e.type == TypeIDOf<HeavyLifter>::Get()) 
            handle.Set(HeavyLifter{e.name, e.special_value});

        // 2. Le CRG projette le comportement SANS muter l'ECS
        if (auto* ai = AIDomain::Resolve<IUnitAI>(e.type, current_time)) {
            ai->Update(handle);
        }
    }
}

int main() {
    SimpleRegistry world;
    world.CreateDrone("Scout-Alpha");
    world.CreateLifter("Atlas-Omega");

    // Simulation du cycle jour/nuit
    AISystem_Update(world, WorldState::Day);
    AISystem_Update(world, WorldState::Night);

    return 0;
}
