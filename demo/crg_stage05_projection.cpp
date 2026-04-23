// ======================================================
// STAGE 5 — PROJECTION (VISIBILITY FILTER)
// ======================================================
//
// @intent:
// Introduce compile-time constrained visibility over the behavior space.
//
// @what_changed:
// Variadic template filtering (Subset...) added to the Find resolution.
//
// @key_insight:
// Projection is a lens. We don't mutate the graph; we change what we are 
// mathematically allowed to see. Early-out before memory traversal.
//
// @transition:
// Linear traversal is O(N). We need to transform this linked list into an O(1) map.
// ======================================================

#include <iostream>
#include <typeinfo>
#include <string>
#include <cstddef>
#include <new>

using TypeID = std::size_t;
template<class T> struct TypeIDOf { static TypeID Get() { return typeid(T).hash_code(); } };

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
        Reset(); new (&m_buffer) Model<T>(std::move(v)); m_set = true;
    }
    TypeID GetTypeID() const { return reinterpret_cast<const Concept*>(&m_buffer)->GetTypeID(); }
    template<class T> const T& Get() const { return static_cast<const Model<T>*>(reinterpret_cast<const Concept*>(&m_buffer))->value; }
private:
    void Reset() { if(m_set) { reinterpret_cast<Concept*>(&m_buffer)->~Concept(); m_set = false; } }
    alignas(std::max_align_t) std::byte m_buffer[SBO_SIZE]; bool m_set = false;
};

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

struct IDiagnostic {
    virtual ~IDiagnostic() = default;
    virtual void Execute(const HardwareHandle& h) const = 0;
    virtual TypeID GetModelID() const = 0;
};

struct Drone { std::string name; };
struct HeavyLifter { std::string name; };
struct Turret { std::string name; };

template<class T>
struct DiagDefinition : IDiagnostic, LinkedNode<DiagDefinition<T>, IDiagnostic> {
    void Execute(const HardwareHandle& h) const override { std::cout << "[DIAGNOSTIC] Pass for hash: " << h.GetTypeID() << "\n"; }
    TypeID GetModelID() const override { return TypeIDOf<T>::Get(); }
};

static DiagDefinition<Drone> g_drone_diag;
static DiagDefinition<HeavyLifter> g_lifter_diag;
static DiagDefinition<Turret> g_turret_diag;

// --- The Projection Filter ---
template<class Interface, class... Subset>
const Interface* Find(const HardwareHandle& h) {
    const TypeID id = h.GetTypeID();

    // 1. Compile-Time Lens: Early out if not in subset
    if constexpr (sizeof...(Subset) > 0) {
        if (!((id == TypeIDOf<Subset>::Get()) || ...)) {
            return nullptr;
        }
    }

    // 2. Linear Traversal
    for (const auto* n = LinkedNode<DiagDefinition<Drone>, Interface>::GetHead(); n; n = n->GetNext()) {
        if (n->GetModelID() == id) return static_cast<const Interface*>(n);
    }
    return nullptr;
}

int main() {
    HardwareHandle h; 
    h.Set(Drone{"Scout-1"});

    std::cout << "--- STAGE 05: PROJECTION (VISIBILITY) ---\n";
    
    std::cout << "Finding Drone with generic search:\n  -> ";
    if (auto* action = Find<IDiagnostic>(h)) action->Execute(h);

    std::cout << "Finding Drone filtered for HeavyLifter ONLY:\n  -> ";
    if (auto* action = Find<IDiagnostic, HeavyLifter>(h)) action->Execute(h);
    else std::cout << "Execution BLOCKED by Subset Filter.\n";

    return 0;
}