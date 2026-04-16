// ======================================================
// STAGE 8 — THE TEMPORAL AXIS (3D SPACE)
// ======================================================
//
// @intent:
// Transition from a flat 2D mapping to a 3D phase space by introducing 
// a dedicated Temporal Axis (Logical SystemStates).
//
// @what_changed:
// - Introduction of the 'SystemState' enum to represent time/state.
// - Matrix updated to support variadic template-template parameters.
// - The 'Match' function signature expands to include (TypeID, SystemState).
//
// @key_insight:
// We don't mutate the graph structure when the state changes. 
// We simply change the temporal coordinate used for resolution.
//
// @what_is_not:
// Not a state machine. Not a structural mutation. Not a runtime map update.
//
// @transition:
// We have conquered Time. But what if we need to model Space, 
// Authority, and Environment simultaneously?
//
// @spoken_line:
// “Identity is a point; Time is a line. But our systems live in a volume.”
// ======================================================

#include <iostream>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <string>
#include <cstddef>

// ======================================================
// TEMPORAL AXIS
// ======================================================
enum class SystemState { Any, Nominal, Emergency };

// Semantic Wrapper
template<auto V> using When = std::integral_constant<decltype(V), V>;

// ======================================================
// TYPE SYSTEM
// ======================================================
using TypeID = std::size_t;
template<class T> struct TypeIDOf { static TypeID Get() { return typeid(T).hash_code(); } };

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
        T value; Model(T v) : value(std::move(v)) {}
        TypeID GetTypeID() const override { return TypeIDOf<T>::Get(); }
    };
    HardwareHandle() = default; ~HardwareHandle() { Reset(); }
    template<class T> void Set(T v) {
        static_assert(sizeof(Model<T>) <= SBO_SIZE, "SBO Overload!");
        Reset(); new (&m_buffer) Model<T>(std::move(v)); m_set = true;
    }
    TypeID GetTypeID() const { return reinterpret_cast<const Concept*>(&m_buffer)->GetTypeID(); }
    template<class T> const T& Get() const { return static_cast<const Model<T>*>(reinterpret_cast<const Concept*>(&m_buffer))->value; }
private:
    void Reset() { if (m_set) { reinterpret_cast<Concept*>(&m_buffer)->~Concept(); m_set = false; } }
    alignas(std::max_align_t) std::byte m_buffer[SBO_SIZE]; bool m_set = false;
};

// ======================================================
// NODE & INTERFACE
// ======================================================
template<class Derived, class Interface>
class LinkedNode {
public:
    LinkedNode() { m_next = s_head; s_head = static_cast<Interface*>(static_cast<Derived*>(this)); }
    Interface* GetNext() const { return m_next; }
    static Interface* GetHead() { return s_head; }
private:
    inline static Interface* s_head = nullptr;
    Interface* m_next = nullptr;
};

struct IExecuteAction {
    virtual ~IExecuteAction() = default;
    virtual void Execute(const HardwareHandle&) const = 0;
    // 3D Match: Identity + State
    virtual bool Match(TypeID id, SystemState s) const = 0;
};

// ======================================================
// DEFINITIONS
// ======================================================
template<class T, class T_State = When<SystemState::Any>>
struct ActionDefinition : IExecuteAction, LinkedNode<ActionDefinition<T, T_State>, IExecuteAction> {
    bool Match(TypeID id, SystemState s) const override {
        return id == TypeIDOf<T>::Get() && 
               (T_State::value == SystemState::Any || T_State::value == s);
    }
    void Execute(const HardwareHandle& t) const override {
        std::cout << "[GENERIC] " << t.Get<T>().name << " acts.\n";
    }
};

template<>
struct ActionDefinition<Drone, When<SystemState::Emergency>> 
    : IExecuteAction, LinkedNode<ActionDefinition<Drone, When<SystemState::Emergency>>, IExecuteAction> {
    bool Match(TypeID id, SystemState s) const override {
        return id == TypeIDOf<Drone>::Get() && s == SystemState::Emergency;
    }
    void Execute(const HardwareHandle& t) const override {
        std::cout << "[TEMPORAL SPECIALIZATION] 🚨 " << t.Get<Drone>().name << " ENTERS EMERGENCY OVERRIDE!\n";
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
    template<class Interface, class... Subset>
    static const Interface* Find(const HardwareHandle& h, SystemState s) {
        const auto id = h.GetTypeID();
        const Interface* result = nullptr;
        if constexpr (sizeof...(Subset) > 0) {
            if (!((id == TypeIDOf<Subset>::Get()) || ...)) return nullptr;
        }
        ((id == TypeIDOf<Models>::Get() && (result = Resolve<Interface>(id, s))), ...);
        return result;
    }
private:
    template<class Interface>
    static const Interface* Resolve(TypeID id, SystemState s) {
        for (Interface* n = Interface::GetHead(); n != nullptr; n = n->GetNext()) {
            if (n->Match(id, s)) return n;
        }
        return nullptr;
    }
};

template<class T> using BaseDef = ActionDefinition<T>;
template<class T> using EmergDef = ActionDefinition<T, When<SystemState::Emergency>>;

using ActionDomain = BehaviorMatrix<Models, BaseDef, EmergDef>;

int main() {
    HardwareHandle drone; drone.Set(Drone{"Scout-1"});
    
    std::cout << "--- STAGE 8: TEMPORAL AXIS (3D) ---\n\n";

    std::cout << "Drone (State: Nominal):\n  -> ";
    if (auto* iface = ActionDomain::Find<IExecuteAction>(drone, SystemState::Nominal)) 
        iface->Execute(drone);

    std::cout << "\nDrone (State: Emergency):\n  -> ";
    if (auto* iface = ActionDomain::Find<IExecuteAction>(drone, SystemState::Emergency)) 
        iface->Execute(drone);

    return 0;
}
