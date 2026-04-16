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
// - Aliases map hyper-coordinates to the single domain matrix instance.
//
// @key_insight:
// The matrix doesn't know what 'Time' is. It just evaluates an intersection of 
// orthogonal coordinates. If we can pass time, we can pass space, authority, and factions.
//
// @what_is_not:
// No hardcoded dimensions
// No centralized maps
// No structural allocations
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
// STAGE 9: N-DIMENSIONAL DOMAIN AXES
// ======================================================
enum class Epoch { Any, Init, Combat };
enum class Biome { Any, Desert, Tundra };
enum class Role  { Any, Client, Server };

// Semantic Wrappers for clean NTTP-to-Type declarations
template<auto V> using When = std::integral_constant<decltype(V), V>;
template<auto V> using In   = std::integral_constant<decltype(V), V>;
template<auto V> using Auth = std::integral_constant<decltype(V), V>;

// ======================================================
// TYPE SYSTEM
// ======================================================
using TypeID = std::size_t;

template<class T>
struct TypeIDOf
{
    static TypeID Get() { return typeid(T).hash_code(); }
};

// ======================================================
// TYPE ERASURE TRANSPORT (EPHEMERAL HANDLE)
// ======================================================
class TypeErasedShell
{
public:
    struct Concept
    {
        virtual ~Concept() = default;
        virtual TypeID GetTypeID() const = 0;
    };

    template<class T>
    struct Model : Concept
    {
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
// NODE LIST
// ======================================================
template<class Derived, class Interface>
class LinkedNode
{
public:
    LinkedNode()
    {
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
// INTERFACES (N-DIMENSIONAL MATCHING)
// ======================================================
struct IExecuteAction
{
    virtual ~IExecuteAction() = default;
    virtual void Execute(const TypeErasedShell&) const = 0;
    
    // The Interface explicitly dictates the dimensions it evaluates
    virtual bool Match(TypeID id, Epoch e, Biome b, Role r) const = 0;
};

// ======================================================
// DOMAIN TYPES
// ======================================================
struct NPC    { std::string name{"NPC"}; };
struct Boss   { std::string name{"Boss"}; };
struct Player { std::string name{"Player"}; };

// ======================================================
// DEFINITIONS = NODE INSTANCES
// ======================================================
template<
    class T, 
    class T_Epoch = When<Epoch::Any>, 
    class T_Biome = In<Biome::Any>, 
    class T_Role  = Auth<Role::Any>
>
struct ActionDefinition
    : IExecuteAction
    , LinkedNode<ActionDefinition<T, T_Epoch, T_Biome, T_Role>, IExecuteAction>
{
    bool Match(TypeID id, Epoch e, Biome b, Role r) const override 
    {
        return id == TypeIDOf<T>::Get() && 
               (T_Epoch::value == Epoch::Any || T_Epoch::value == e) &&
               (T_Biome::value == Biome::Any || T_Biome::value == b) &&
               (T_Role::value  == Role::Any  || T_Role::value  == r);
    }

    void Execute(const TypeErasedShell& t) const override
    {
        std::cout << "[GENERIC ACTION] " << t.Get<T>().name << " does something generic.\n";
    }
};

// --- SPECIALIZATIONS ---

// The Hyper-Contextual Definition: Boss, in Combat, in the Desert, on the Server!
template<>
struct ActionDefinition<Boss, When<Epoch::Combat>, In<Biome::Desert>, Auth<Role::Server>>
    : IExecuteAction
    , LinkedNode<ActionDefinition<Boss, When<Epoch::Combat>, In<Biome::Desert>, Auth<Role::Server>>, IExecuteAction>
{
    bool Match(TypeID id, Epoch e, Biome b, Role r) const override 
    {
        return id == TypeIDOf<Boss>::Get() && 
               e == Epoch::Combat && 
               b == Biome::Desert && 
               r == Role::Server;
    }

    void Execute(const TypeErasedShell& t) const override
    {
        std::cout << "[SERVER-AUTHORITATIVE] 🔥 " << t.Get<Boss>().name << " casts DESERT SANDSTORM!\n";
    }
};

// ======================================================
// ALIASES FOR BEHAVIOR MATRIX PACK EXPANSION
// ======================================================
template<class T> 
using GenericActionDef = ActionDefinition<T>;

template<class T> 
using SandstormActionDef = ActionDefinition<T, When<Epoch::Combat>, In<Biome::Desert>, Auth<Role::Server>>;

// ======================================================
// MODEL LIST
// ======================================================
template<class...> struct TypeList;
using Models = TypeList<NPC, Boss, Player>;

// ======================================================
// BEHAVIOR MATRIX (AUTO-REGISTRATION + N-DIMENSIONAL ROUTING)
// ======================================================
template<class ModelT, template<class> class... Defs>
struct BehaviorMatrixSingle : public Defs<ModelT>... 
{
};

template<class ModelList, template<class> class... Defs>
struct BehaviorMatrix;

template<class... Models, template<class> class... Defs>
struct BehaviorMatrix<TypeList<Models...>, Defs...> 
    : public BehaviorMatrixSingle<Models, Defs...>...
{
    // The Matrix blindly forwards variadic Args... to the Interface
    template<class Interface, class... Subset, class... Args>
    static const Interface* Find(const TypeErasedShell& tes, Args... args)
    {
        const auto id = tes.GetTypeID();
        const Interface* result = nullptr;

        if constexpr (sizeof...(Subset) > 0)
        {
            const bool allowed = ((id == TypeIDOf<Subset>::Get()) || ...);
            if (!allowed)
                return nullptr;
        }

        // Strict compile-time identity gate routing
        ((id == TypeIDOf<Models>::Get() &&
            (result = Resolve<Interface>(id, args...))), ...);

        return result;
    }

private:
    template<class Interface, class... Args>
    static const Interface* Resolve(TypeID id, Args... args)
    {
        for (Interface* n = Interface::GetHead(); n != nullptr; n = n->GetNext()) 
        {
            // The N-Dimensional intersection occurs here
            if (n->Match(id, args...)) 
            {
                return n;
            }
        }
        return nullptr;
    }
};

// ======================================================
// DOMAIN BINDING
// ======================================================
using ActionDomainBehavior = BehaviorMatrix<
    Models,
    GenericActionDef,
    SandstormActionDef
>;

// A single static instantiation builds the entire N-Dimensional Hypergraph!
static const ActionDomainBehavior gs_Behaviors;

// ======================================================
// API
// ======================================================
template<class Interface, class... Subset, class... Args>
void Call(const TypeErasedShell& tes, Args... args)
{
    if (const Interface* iface = ActionDomainBehavior::Find<Interface, Subset...>(tes, args...))
    {
        iface->Execute(tes);
    }
}

// ======================================================
// MAIN
// ======================================================
int main()
{
    TypeErasedShell boss; boss.Set(Boss{"Scorpion King"});
    TypeErasedShell npc;  npc.Set(NPC{"Merchant"});

    std::cout << "--- STAGE 9: N-DIMENSIONAL RESOLUTION TESTS ---\n\n";

    // Test 1: NPC in standard context
    std::cout << "NPC (Combat, Desert, Client):\n  -> ";
    Call<IExecuteAction>(npc, Epoch::Combat, Biome::Desert, Role::Client);

    // Test 2: Boss spawning (Generic fallback)
    std::cout << "\nBoss (Init, Tundra, Server):\n  -> ";
    Call<IExecuteAction>(boss, Epoch::Init, Biome::Tundra, Role::Server);

    // Test 3: Boss in combat, in Desert, but on Client!
    std::cout << "\nBoss (Combat, Desert, Client):\n  -> ";
    Call<IExecuteAction>(boss, Epoch::Combat, Biome::Desert, Role::Client);
    // (Uses the generic action, protecting game state on the client)

    // Test 4: The Exact Hyper-Coordinate!
    std::cout << "\nBoss (Combat, Desert, Server):\n  -> ";
    Call<IExecuteAction>(boss, Epoch::Combat, Biome::Desert, Role::Server);

    return 0;
}
