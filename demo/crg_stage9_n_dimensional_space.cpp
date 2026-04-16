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
// No hardcoded dimensions.
// No centralized maps.
// No structural allocations.
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

// ======================================================
// N-DIMENSIONAL DOMAIN AXES
// ======================================================
enum class Epoch { Any, Init, Combat };
enum class Biome { Any, Desert, Tundra };
enum class Role  { Any, Client, Server };

// Semantic Wrappers for clean NTTP-to-Type declarations
template<auto V> using When = std::integral_constant<decltype(V), V>;
template<auto V> using In   = std::integral_constant<decltype(V), V>;
template<auto V> using Auth = std::integral_constant<decltype(V), V>;

// ======================================================
// TYPE SYSTEM & IDENTITY
// ======================================================
using TypeID = std::size_t;

template<class T>
struct TypeIDOf {
    static TypeID Get() { return typeid(T).hash_code(); }
};

struct NPC    { std::string name; };
struct Boss   { std::string name; };
struct Player { std::string name; };

// ======================================================
// TYPE ERASURE TRANSPORT (EPHEMERAL HANDLE)
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
// CORE ARCHITECTURE: THE INTRUSIVE DISCOVERY LAYER
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
// THE CAPABILITY INTERFACE (N-DIMENSIONAL MATCHING)
// ======================================================
struct IExecuteAction {
    virtual ~IExecuteAction() = default;
    virtual void Execute(const TypeErasedShell&) const = 0;
    
    // Variadic Match for N-Dimensional coordinates
    virtual bool Match(TypeID id, Epoch e, Biome b, Role r) const = 0;
};

// Base template for definitions
template<class T, class T_E = When<Epoch::Any>, class T_B = In<Biome::Any>, class T_R = Auth<Role::Any>>
struct ActionDefinition : IExecuteAction, LinkedNode<ActionDefinition<T, T_E, T_B, T_R>, IExecuteAction> {
    bool Match(TypeID id, Epoch e, Biome b, Role r) const override {
        return id == TypeIDOf<T>::Get() && 
               (T_E::value == Epoch::Any || T_E::value == e) &&
               (T_B::value == Biome::Any || T_B::value == b) &&
               (T_R::value == Role::Any  || T_R::value == r);
    }
    void Execute(const TypeErasedShell& t) const override {
        std::cout << "[GENERIC] " << t.Get<T>().name << " performs a standard action.\n";
    }
};

// ======================================================
// BEHAVIOR MATRIX (THE OBSERVER)
// ======================================================
template<class...> struct TypeList;
using Models = TypeList<NPC, Boss, Player>;

// Fixed: template<class...> for variadic template-template support
template<class ModelT, template<class...> class... Defs>
struct BehaviorMatrixSingle : public Defs<ModelT>... {};

template<class ModelList, template<class...> class... Defs>
struct BehaviorMatrix;

template<class... Models, template<class...> class... Defs>
struct BehaviorMatrix<TypeList<Models...>, Defs...> : public BehaviorMatrixSingle<Models, Defs...>... {
    
    template<class Interface, class... Subset, class... Args>
    static const Interface* Find(const TypeErasedShell& tes, Args... args) {
        const auto id = tes.GetTypeID();
        const Interface* result = nullptr;

        // Optional Projection Filter (Static subsetting)
        if constexpr (sizeof...(Subset) > 0) {
            if (!((id == TypeIDOf<Subset>::Get()) || ...)) return nullptr;
        }

        // Search in all registered models
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

// Aliases for matrix expansion
template<class T> using GenericDef = ActionDefinition<T>;
template<class T> using SandstormDef = ActionDefinition<T, When<Epoch::Combat>, In<Biome::Desert>, Auth<Role::Server>>;

using ActionDomain = BehaviorMatrix<Models, GenericDef, SandstormDef>;
static const ActionDomain gs_Registry; // The matrix observes the topology

// ======================================================
// API
// ======================================================
template<class Interface, class... Subset, class... Args>
void Call(const TypeErasedShell& tes, Args... args) {
    if (const Interface* iface = ActionDomain::Find<Interface, Subset...>(tes, args...)) {
        iface->Execute(tes);
    } else {
        std::cout << "No capability found for this coordinate.\n";
    }
}

// ======================================================
// DECENTRALIZED MODULES (The "Invisible" Logic)
// ======================================================

namespace { // Simulate Plugin_Core.cpp
    static GenericDef<NPC>    g_regNPC;
    static GenericDef<Boss>   g_regBoss;
    static GenericDef<Player> g_regPlayer;
}

namespace { // Simulate Plugin_DesertCombat_Server.cpp
    // This specialization is invisible to the rest of the file.
    struct DesertSandstorm : IExecuteAction, LinkedNode<DesertSandstorm, IExecuteAction> {
        bool Match(TypeID id, Epoch e, Biome b, Role r) const override {
            return id == TypeIDOf<Boss>::Get() && 
                   e == Epoch::Combat && 
                   b == Biome::Desert && 
                   r == Role::Server;
        }
        void Execute(const TypeErasedShell& t) const override {
            std::cout << "[SERVER-ONLY] 🔥 " << t.Get<Boss>().name << " unleashes a LETHAL SANDSTORM!\n";
        }
    };
    static DesertSandstorm auto_reg_sandstorm; 
}

// ======================================================
// MAIN: THE HYPER-COORDINATE TEST
// ======================================================
int main() {
    TypeErasedShell boss; boss.Set(Boss{"Scorpion King"});
    TypeErasedShell npc;  npc.Set(NPC{"Merchant"});

    std::cout << "--- STAGE 9: N-DIMENSIONAL RECONSTRUCTION ---\n\n";

    // Test 1: Boss in Combat, but on Client (Security: fallbacks to generic)
    std::cout << "Boss (Combat, Desert, Client):\n  -> ";
    Call<IExecuteAction>(boss, Epoch::Combat, Biome::Desert, Role::Client);

    // Test 2: The Exact Hyper-Coordinate (Specialization found)
    std::cout << "\nBoss (Combat, Desert, Server):\n  -> ";
    Call<IExecuteAction>(boss, Epoch::Combat, Biome::Desert, Role::Server);

    // Test 3: NPC in the same coordinate (No sandstorm, falls back to generic)
    std::cout << "\nNPC (Combat, Desert, Server):\n  -> ";
    Call<IExecuteAction>(npc, Epoch::Combat, Biome::Desert, Role::Server);

    return 0;
}
