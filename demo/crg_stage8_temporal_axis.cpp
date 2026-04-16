// ======================================================
// STAGE 8 — THE TEMPORAL AXIS (3D SPACE)
// ======================================================
//
// @intent:
// Transition from a flat 2D mapping to a 3D phase space by introducing 
// a dedicated Temporal Axis (Logical Epochs).
//
// @what_changed:
// - Introduction of the 'Epoch' enum to represent time/state.
// - Matrix updated to support variadic template-template parameters.
// - The 'Match' function signature expands to include (TypeID, Epoch).
//
// @key_insight:
// We don't mutate the graph structure when the game state changes. 
// We simply change the temporal coordinate used for resolution.
//
// @what_is_not:
// Not a state machine.
// Not a structural mutation.
// Not a runtime map update.
//
// @transition:
// We have conquered Time. But what if we need to model Space, 
// Authority, and Factions simultaneously?
//
// @spoken_line:
// “Identity is a point; Time is a line. But our systems live in a volume.”
// ======================================================

#include <iostream>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <string>

// ======================================================
// TEMPORAL AXIS
// ======================================================
enum class Epoch { Any, Init, Combat, Exploration };

// Semantic Wrappers for clean NTTP-to-Type declarations
template<auto V> using When = std::integral_constant<decltype(V), V>;

// ======================================================
// TYPE SYSTEM & IDENTITY
// ======================================================
using TypeID = std::size_t;

template<class T>
struct TypeIDOf {
    static TypeID Get() { return typeid(T).hash_code(); }
};

struct Boss { std::string name; };
struct NPC  { std::string name; };

// ======================================================
// TYPE ERASURE TRANSPORT
// ======================================================
class TypeErasedShell {
public:
    struct Concept {
        virtual ~Concept() = default;
        virtual TypeID GetTypeID() const = 0;
    };

    template<class T>
    struct Model : Concept {
        T value;
        Model(T v) : value(std::move(v)) {}
        TypeID GetTypeID() const override { return TypeIDOf<T>::Get(); }
    };

    template<class T>
    void Set(T v) { ptr = std::make_unique<Model<T>>(std::move(v)); }

    TypeID GetTypeID() const { return ptr->GetTypeID(); }

    template<class T>
    const T& Get() const { return static_cast<Model<T>*>(ptr.get())->value; }

private:
    std::unique_ptr<Concept> ptr;
};

// ======================================================
// INTRUSIVE DISCOVERY LAYER
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
    virtual void Execute(const TypeErasedShell&) const = 0;
    // Match evolved to 3D: (Identity, Epoch)
    virtual bool Match(TypeID id, Epoch e) const = 0;
};

// ======================================================
// DEFINITIONS
// ======================================================
template<class T, class T_Epoch = When<Epoch::Any>>
struct ActionDefinition : IExecuteAction, LinkedNode<ActionDefinition<T, T_Epoch>, IExecuteAction> {
    bool Match(TypeID id, Epoch e) const override {
        return id == TypeIDOf<T>::Get() && 
               (T_Epoch::value == Epoch::Any || T_Epoch::value == e);
    }

    void Execute(const TypeErasedShell& t) const override {
        std::cout << "[GENERIC] " << t.Get<T>().name << " acts.\n";
    }
};

// --- Specialization for Combat ---
template<>
struct ActionDefinition<Boss, When<Epoch::Combat>> 
    : IExecuteAction, LinkedNode<ActionDefinition<Boss, When<Epoch::Combat>>, IExecuteAction> {
    
    bool Match(TypeID id, Epoch e) const override {
        return id == TypeIDOf<Boss>::Get() && e == Epoch::Combat;
    }

    void Execute(const TypeErasedShell& t) const override {
        std::cout << "[TEMPORAL SPECIALIZATION] 🔥 " << t.Get<Boss>().name << " enters RAGE MODE!\n";
    }
};

// ======================================================
// BEHAVIOR MATRIX (3D Ready)
// ======================================================
template<class...> struct TypeList;
using Models = TypeList<NPC, Boss>;

template<class ModelT, template<class...> class... Defs>
struct BehaviorMatrixSingle : public Defs<ModelT>... {};

template<class ModelList, template<class...> class... Defs>
struct BehaviorMatrix;

template<class... Models, template<class...> class... Defs>
struct BehaviorMatrix<TypeList<Models...>, Defs...> 
    : public BehaviorMatrixSingle<Models, Defs...>... 
{
    template<class Interface, class... Subset>
    static const Interface* Find(const TypeErasedShell& tes, Epoch e) {
        const auto id = tes.GetTypeID();
        const Interface* result = nullptr;

        if constexpr (sizeof...(Subset) > 0) {
            if (!((id == TypeIDOf<Subset>::Get()) || ...)) return nullptr;
        }

        ((id == TypeIDOf<Models>::Get() && (result = Resolve<Interface>(id, e))), ...);
        return result;
    }

private:
    template<class Interface>
    static const Interface* Resolve(TypeID id, Epoch e) {
        for (Interface* n = Interface::GetHead(); n != nullptr; n = n->GetNext()) {
            if (n->Match(id, e)) return n;
        }
        return nullptr;
    }
};

// Aliases
template<class T> using BaseDef = ActionDefinition<T>;
template<class T> using CombatDef = ActionDefinition<T, When<Epoch::Combat>>;

using ActionDomain = BehaviorMatrix<Models, BaseDef, CombatDef>;
static const ActionDomain gs_Registry;

// ======================================================
// MAIN
// ======================================================
int main() {
    TypeErasedShell boss; boss.Set(Boss{"Dragon"});
    
    std::cout << "--- STAGE 8: TEMPORAL AXIS (3D) ---\n\n";

    // Test 1: Exploration (Fallback to generic)
    std::cout << "Boss (Epoch: Exploration):\n  -> ";
    if (auto* iface = ActionDomain::Find<IExecuteAction>(boss, Epoch::Exploration)) 
        iface->Execute(boss);

    // Test 2: Combat (Specialization found)
    std::cout << "\nBoss (Epoch: Combat):\n  -> ";
    if (auto* iface = ActionDomain::Find<IExecuteAction>(boss, Epoch::Combat)) 
        iface->Execute(boss);

    return 0;
}
