// ======================================================
// STAGE 9 — THE N-DIMENSIONAL CONTEXT SPACE (HYPERGRAPH)
// ======================================================
//
// @intent:
// Abstract the resolution matrix to support variadic, orthogonal coordinate axes 
// while preserving auto-instantiation and strict compile-time routing.
//
// @what_changed:
// - The matrix blindly forwards N-dimensional arguments (Args...) to Resolve.
// - Definitions use semantic NTTP wrappers (When<>, In<>, Auth<>).
// - Matrix support for variadic template-template parameters.
//
// @key_insight:
// The matrix doesn't know what 'Time' or 'Space' is. It just evaluates an 
// intersection of orthogonal coordinates. The "Manager" doesn't manage; it observes.
//
// @what_is_not:
// No hardcoded dimensions. No centralized maps. No structural allocations.
//
// @transition:
// [End of Talk - Conclusion]
//
// @spoken_line:
// “We hadn't just added a Z-axis. We had accidentally opened a portal to an 
// N-Dimensional universe.”
// ======================================================

#include <iostream>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <string>
#include <cstddef>

// ======================================================
// AXES
// ======================================================
enum class State    { Any, Init, Combat };
enum class Zone     { Any, Desert, Tundra };
enum class Security { Any, Client, Server };

template<auto V> using When = std::integral_constant<decltype(V), V>;
template<auto V> using In   = std::integral_constant<decltype(V), V>;
template<auto V> using Auth = std::integral_constant<decltype(V), V>;

// ======================================================
// TYPE SYSTEM
// ======================================================
using TypeID = std::size_t;

template<class T>
struct TypeIDOf { static TypeID Get() { return typeid(T).hash_code(); } };

struct Drone       { std::string name; };
struct HeavyLifter { std::string name; };

// ======================================================
// SBO TRANSPORT
// ======================================================
class HardwareHandle {
    static constexpr std::size_t SBO_SIZE = 48;
public:
    struct Concept { virtual ~Concept() = default; virtual TypeID GetTypeID() const = 0; };
    template<class T> struct Model : Concept {
        T value;
        Model(T v) : value(std::move(v)) {}
        TypeID GetTypeID() const override { return TypeIDOf<T>::Get(); }
    };

    HardwareHandle() = default;
    ~HardwareHandle() { Reset(); }

    template<class T> void Set(T v) {
        static_assert(sizeof(Model<T>) <= SBO_SIZE, "SBO Capacity Exceeded!");
        Reset();
        new (&m_buffer) Model<T>(std::move(v));
        m_set = true;
    }

    TypeID GetTypeID() const { return reinterpret_cast<const Concept*>(&m_buffer)->GetTypeID(); }
    template<class T> const T& Get() const { return static_cast<const Model<T>*>(reinterpret_cast<const Concept*>(&m_buffer))->value; }

private:
    void Reset() { if (m_set) { reinterpret_cast<Concept*>(&m_buffer)->~Concept(); m_set = false; } }
    alignas(std::max_align_t) std::byte m_buffer[SBO_SIZE];
    bool m_set = false;
};

// ======================================================
// LINKED NODE
// ======================================================
template<class Derived, class Interface>
class LinkedNode {
public:
    LinkedNode() {
        m_next = s_head;
        s_head = static_cast<Interface*>(static_cast<Derived*>(this));
    }
    Interface* GetNext() const { return m_next; }
    static Interface* GetHead() { return s_head; }
private:
    inline static Interface* s_head = nullptr;
    Interface* m_next = nullptr;
};

// ======================================================
// INTERFACE
// ======================================================
struct IExecuteAction {
    virtual ~IExecuteAction() = default;
    virtual void Execute(const HardwareHandle&) const = 0;
    
    // N-Dimensional Match
    virtual bool Match(TypeID id, State e, Zone b, Security r) const = 0;
};

// ======================================================
// DEFINITIONS
// ======================================================
template<class T, class T_E = When<State::Any>, class T_B = In<Zone::Any>, class T_R = Auth<Security::Any>>
struct ActionDefinition : IExecuteAction, LinkedNode<ActionDefinition<T, T_E, T_B, T_R>, IExecuteAction> {
    bool Match(TypeID id, State e, Zone b, Security r) const override {
        return id == TypeIDOf<T>::Get() && 
               (T_E::value == State::Any || T_E::value == e) &&
               (T_B::value == Zone::Any  || T_B::value == b) &&
               (T_R::value == Security::Any || T_R::value == r);
    }
    void Execute(const HardwareHandle& t) const override {
        std::cout << "[GENERIC] " << t.Get<T>().name << " performs standard action.\n";
    }
};

// ======================================================
// MATRIX
// ======================================================
template<class...> struct TypeList;
using Models = TypeList<Drone, HeavyLifter>;

template<class ModelT, template<class...> class... Defs>
struct BehaviorMatrixSingle : public Defs<ModelT>... {};

template<class ModelList, template<class...> class... Defs>
struct BehaviorMatrix;

template<class... Models, template<class...> class... Defs>
struct BehaviorMatrix<TypeList<Models...>, Defs...> : public BehaviorMatrixSingle<Models, Defs...>... {
    
    template<class Interface, class... Subset, class... Args>
    static const Interface* Find(const HardwareHandle& h, Args... args) {
        const auto id = h.GetTypeID();
        const Interface* result = nullptr;

        if constexpr (sizeof...(Subset) > 0) {
            if (!((id == TypeIDOf<Subset>::Get()) || ...)) return nullptr;
        }

        ((id == TypeIDOf<Models>::Get() && (result = Resolve<Interface>(id, args...))), ...);
        return result;
    }

private:
    template<class Interface, class... Args>
    static const Interface* Resolve(TypeID id, Args... args) {
        for (Interface* n = Interface::GetHead(); n != nullptr; n = n->GetNext()) {
            if (n->Match(id, args...)) return n;
        }
        return nullptr;
    }
};

// Definitions Registration
template<class T> using GenericDef = ActionDefinition<T>;
template<class T> using SandstormDef = ActionDefinition<T, When<State::Combat>, In<Zone::Desert>, Auth<Security::Server>>;

using ActionDomain = BehaviorMatrix<Models, GenericDef, SandstormDef>;

// ======================================================
// API
// ======================================================
template<class Interface, class... Subset, class... Args>
void Call(const HardwareHandle& h, Args... args) {
    if (const Interface* iface = ActionDomain::Find<Interface, Subset...>(h, args...)) {
        iface->Execute(h);
    } else {
        std::cout << "No capability found for this coordinate.\n";
    }
}

// ======================================================
// MODULES
// ======================================================
namespace { 
    struct DesertSandstorm : IExecuteAction, LinkedNode<DesertSandstorm, IExecuteAction> {
        bool Match(TypeID id, State e, Zone b, Security r) const override {
            return id == TypeIDOf<HeavyLifter>::Get() && 
                   e == State::Combat && 
                   b == Zone::Desert && 
                   r == Security::Server;
        }
        void Execute(const HardwareHandle& t) const override {
            std::cout << "[SERVER-ONLY] 🔥 " << t.Get<HeavyLifter>().name << " engages DUST SHIELD!\n";
        }
    };
    static DesertSandstorm auto_reg_sandstorm; 
}

// ======================================================
// MAIN
// ======================================================
int main() {
    HardwareHandle lifter; lifter.Set(HeavyLifter{"Loader-X"});
    HardwareHandle drone;  drone.Set(Drone{"Scout-1"});

    std::cout << "--- STAGE 9: N-DIMENSIONAL RECONSTRUCTION ---\n\n";

    std::cout << "HeavyLifter (Combat, Desert, Client):\n  -> ";
    Call<IExecuteAction>(lifter, State::Combat, Zone::Desert, Security::Client);

    std::cout << "\nHeavyLifter (Combat, Desert, Server):\n  -> ";
    Call<IExecuteAction>(lifter, State::Combat, Zone::Desert, Security::Server);

    std::cout << "\nDrone (Combat, Desert, Server):\n  -> ";
    Call<IExecuteAction>(drone, State::Combat, Zone::Desert, Security::Server);

    return 0;
}
