// ======================================================
// STAGE 4 — GLOBAL BEHAVIOR SPACE & CRTP
// ======================================================
//
// @intent:
// Establish a generic, interface-agnostic auto-registration mechanism.
//
// @what_changed:
// Introduction of `LinkedNode<Derived, Interface>`.
// IBehavior no longer implements the linked list itself. 
//
// @key_insight:
// Infrastructure and Domain are separated. Any interface can now form its 
// own global capability space automatically.
//
// @transition:
// We have a global space, but we have no way to restrict execution.
// ======================================================

#include <iostream>
#include <typeinfo>
#include <string>
#include <cstddef>
#include <new>

using TypeID = std::size_t;

template<class T> 
struct TypeIDOf { static TypeID Get() { return typeid(T).hash_code(); } };

// --- 1. SBO Hardware Handle ---
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

// --- 2. The CRTP Auto-Registration Node ---
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

// --- 3. Domain Interface ---
struct IDiagnostic {
    virtual ~IDiagnostic() = default;
    virtual void Execute(const HardwareHandle& h) const = 0;
    virtual TypeID GetModelID() const = 0;
};

// --- 4. Models & Definitions ---
struct Drone { std::string name; };
struct HeavyLifter { std::string name; };

template<class T>
struct DiagDefinition : IDiagnostic, LinkedNode<DiagDefinition<T>, IDiagnostic> {
    void Execute(const HardwareHandle& h) const override { 
        std::cout << "[DIAGNOSTIC] Validating model hash: " << h.GetTypeID() << "\n"; 
    }
    TypeID GetModelID() const override { return TypeIDOf<T>::Get(); }
};

// Global immutable instantiation
static DiagDefinition<Drone> g_drone_diag;
static DiagDefinition<HeavyLifter> g_lifter_diag;

// --- 5. Resolution ---
template<class Interface>
const Interface* Find(const HardwareHandle& h) {
    const TypeID id = h.GetTypeID();
    // Linear Traversal (O(N))
    for (const auto* n = LinkedNode<DiagDefinition<Drone>, Interface>::GetHead(); n; n = n->GetNext()) {
        if (n->GetModelID() == id) return static_cast<const Interface*>(n);
    }
    return nullptr;
}

int main() {
    HardwareHandle h; 
    h.Set(Drone{"Scout-1"});

    std::cout << "--- STAGE 04: GLOBAL BEHAVIOR SPACE ---\n";
    if (auto* action = Find<IDiagnostic>(h)) {
        action->Execute(h);
    }
    return 0;
}