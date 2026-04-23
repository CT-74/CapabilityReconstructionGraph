// ======================================================
// STAGE 3 — IDENTITY-BASED RESOLUTION (SBO)
// ======================================================
//
// @intent:
// Resolve behavior by walking the graph, and optimize memory.
//
// @what_changed:
// Traversal-based resolution over linked nodes using TypeID matching.
// HardwareHandle evolves to Small Buffer Optimization (SBO) for zero-allocation.
// IBehavior now mandates GetModelID().
//
// @key_insight:
// Lookup is an emergent property of walking the structure.
//
// @transition:
// Writing `m_next = s_head` in every interface is tedious. 
// We need to abstract the auto-registration.
// ======================================================

#include <iostream>
#include <typeinfo>
#include <string>
#include <cstddef>

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

struct IBehavior {
    virtual ~IBehavior() = default;
    virtual void Execute(const HardwareHandle& h) const = 0;
    virtual TypeID GetModelID() const = 0;

    const IBehavior* GetNext() const { return m_next; }
    static const IBehavior* GetHead() { return s_head; }
protected:
    IBehavior() { m_next = s_head; s_head = this; }
private:
    inline static const IBehavior* s_head = nullptr;
    const IBehavior* m_next = nullptr;
};

template<class T>
struct DiagBehavior : IBehavior {
    void Execute(const HardwareHandle& h) const override { std::cout << h.Get<T>().name << " diag complete.\n"; }
    TypeID GetModelID() const override { return TypeIDOf<T>::Get(); }
};

struct HeavyLifter { std::string name; };

template<class Interface>
const Interface* Find(const HardwareHandle& h) {
    for (const auto* n = Interface::GetHead(); n; n = n->GetNext()) {
        if (n->GetModelID() == h.GetTypeID()) return static_cast<const Interface*>(n);
    }
    return nullptr;
}

int main() {
    DiagBehavior<HeavyLifter> b;
    HardwareHandle h; h.Set(HeavyLifter{"Loader-99"});

    if (auto* n = Find<IBehavior>(h)) n->Execute(h);
    return 0;
}