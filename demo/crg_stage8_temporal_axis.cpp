// ======================================================
// STAGE 8 — THE TEMPORAL AXIS (AUTO-REGISTERED 3D SPACE)
// ======================================================
//
// @intent:
// Introduce a true domain axis for time (Epoch) while auto-instantiating 
// the graph via variadic template pack expansion and inheritance.
//
// @what_changed:
// BehaviorMatrix now inherits from Defs<Models>... to auto-register nodes.
// A single static const instance triggers the intrusive LinkedNode constructors.
// Epoch is bound to template aliases to fit the Matrix's Defs signature.
//
// @key_insight:
// The matrix isn't just a router; it's a domain boundary. Instantiating 
// the matrix automatically builds the exact 3D capability space for that domain.
// Time becomes just another coordinate passed to the resolution engine.
//
// @what_is_not:
// No stack lifecycle tricks (RAII)
// No structural mutation at runtime
// No thread-safety issues
//
// @transition:
// Parameterize the matrix to accept any kind of coordinate, not just time.
//
// @spoken_line:
// “We decoupled time from the call stack. The graph doesn't mutate, thread-safety 
// is restored, and the capabilities just shift.”
// ======================================================

#include <iostream>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <string>

// ======================================================
// STAGE 8: TEMPORAL DOMAIN
// ======================================================
enum class Epoch { Any, Init, Combat };

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
// INTERFACES
// ======================================================
struct IPrintName
{
    virtual ~IPrintName() = default;
    virtual void Execute(const TypeErasedShell&) const = 0;
    
    // Coordinates required for Resolve()
    virtual TypeID TargetType() const = 0;
    virtual Epoch TargetEpoch() const = 0;
};

// ======================================================
// DOMAIN TYPES
// ======================================================
struct NPC    { std::string name{"NPC"}; };
struct Boss   { std::string name{"Boss"}; };
struct Player { std::string name{"Player"}; };

// ======================================================
// TEMPORAL DEFINITIONS = NODE INSTANCES
// ======================================================
template<class T, Epoch E>
struct TemporalPrintDefinition
    : IPrintName
    , LinkedNode<TemporalPrintDefinition<T, E>, IPrintName>
{
    TypeID TargetType() const override { return TypeIDOf<T>::Get(); }
    Epoch TargetEpoch() const override { return E; }

    void Execute(const TypeErasedShell& t) const override
    {
        std::cout << "[GENERIC] " << t.Get<T>().name << " does something generic.\n";
    }
};

// --- SPECIALIZATIONS ---

template<>
struct TemporalPrintDefinition<Boss, Epoch::Init>
    : IPrintName
    , LinkedNode<TemporalPrintDefinition<Boss, Epoch::Init>, IPrintName>
{
    TypeID TargetType() const override { return TypeIDOf<Boss>::Get(); }
    Epoch TargetEpoch() const override { return Epoch::Init; }
    void Execute(const TypeErasedShell& t) const override {
        std::cout << "[INIT] " << t.Get<Boss>().name << " awakens...\n";
    }
};

template<>
struct TemporalPrintDefinition<Boss, Epoch::Combat>
    : IPrintName
    , LinkedNode<TemporalPrintDefinition<Boss, Epoch::Combat>, IPrintName>
{
    TypeID TargetType() const override { return TypeIDOf<Boss>::Get(); }
    Epoch TargetEpoch() const override { return Epoch::Combat; }
    void Execute(const TypeErasedShell& t) const override {
        std::cout << "[COMBAT] 🔥 " << t.Get<Boss>().name << " unleashes fury!\n";
    }
};

template<>
struct TemporalPrintDefinition<Player, Epoch::Any>
    : IPrintName
    , LinkedNode<TemporalPrintDefinition<Player, Epoch::Any>, IPrintName>
{
    TypeID TargetType() const override { return TypeIDOf<Player>::Get(); }
    Epoch TargetEpoch() const override { return Epoch::Any; }
    void Execute(const TypeErasedShell& t) const override {
        std::cout << "Player: " << t.Get<Player>().name << " is ready.\n";
    }
};

// ======================================================
// ALIASES FOR BEHAVIOR MATRIX PACK EXPANSION
// ======================================================
template<class T> using PrintInitDef   = TemporalPrintDefinition<T, Epoch::Init>;
template<class T> using PrintCombatDef = TemporalPrintDefinition<T, Epoch::Combat>;
template<class T> using PrintAnyDef    = TemporalPrintDefinition<T, Epoch::Any>;

// ======================================================
// MODEL LIST
// ======================================================
template<class...> struct TypeList;
using Models = TypeList<NPC, Boss, Player>;

// ======================================================
// BEHAVIOR MATRIX (AUTO-REGISTRATION + 3D RESOLUTION)
// ======================================================

// Base Expansion: Inherits from all Definitions for a single Model
template<class ModelT, template<class> class... Defs>
struct BehaviorMatrixSingle : public Defs<ModelT>... 
{
};

// Main Matrix: Expands the TypeList and inherits from BehaviorMatrixSingle
template<class ModelList, template<class> class... Defs>
struct BehaviorMatrix;

template<class... Models, template<class> class... Defs>
struct BehaviorMatrix<TypeList<Models...>, Defs...> 
    : public BehaviorMatrixSingle<Models, Defs...>...
{
    template<class Interface, class... Subset>
    static const Interface* Find(const TypeErasedShell& tes, Epoch current_epoch)
    {
        const auto id = tes.GetTypeID();
        const Interface* result = nullptr;

        if constexpr (sizeof...(Subset) > 0)
        {
            const bool allowed = ((id == TypeIDOf<Subset>::Get()) || ...);
            if (!allowed)
                return nullptr;
        }

        // Strict compile-time constrained identity gate routing
        ((id == TypeIDOf<Models>::Get() &&
            (result = Resolve<Interface>(id, current_epoch))), ...);

        return result;
    }

private:
    template<class Interface>
    static const Interface* Resolve(TypeID id, Epoch current_epoch)
    {
        for (Interface* n = Interface::GetHead(); n != nullptr; n = n->GetNext()) 
        {
            if (n->TargetType() == id && (n->TargetEpoch() == current_epoch || n->TargetEpoch() == Epoch::Any)) 
            {
                return n;
            }
        }
        return nullptr;
    }
};

// ======================================================
// DOMAIN BINDING (THE MAGIC HAPPENS HERE)
// ======================================================
// By passing the alias templates, the Matrix will auto-instantiate the 
// exact 3D nodes we need for all Models in the TypeList.
using DomainBehavior = BehaviorMatrix<
    Models,
    PrintInitDef,
    PrintCombatDef,
    PrintAnyDef
>;

// This single line triggers the intrusive linking of the ENTIRE domain!
static const DomainBehavior gs_Behaviors;

// ======================================================
// API
// ======================================================
template<class Interface, class... Subset>
void Call(const TypeErasedShell& tes, Epoch current_epoch = Epoch::Any)
{
    if (const Interface* iface = DomainBehavior::Find<Interface, Subset...>(tes, current_epoch))
    {
        iface->Execute(tes);
    }
}

// ======================================================
// MAIN
// ======================================================
int main()
{
    TypeErasedShell tes;

    std::cout << "--- STAGE 8: AUTO-REGISTERED TEMPORAL RESOLUTION ---\n\n";

    // BOSS
    tes.Set(Boss{"Dragon"});
    std::cout << "Boss at Epoch::Init:\n  -> ";
    Call<IPrintName>(tes, Epoch::Init);

    std::cout << "Boss at Epoch::Combat:\n  -> ";
    Call<IPrintName>(tes, Epoch::Combat);

    // PLAYER (Falls back to Any)
    std::cout << "\nPlayer at Epoch::Combat (Fallback to Any):\n  -> ";
    tes.Set(Player{"Alex"});
    Call<IPrintName>(tes, Epoch::Combat);

    // NPC (Unspecialized, uses generic template)
    std::cout << "\nNPC at Epoch::Init (Generic template):\n  -> ";
    tes.Set(NPC{"Grunt"});
    Call<IPrintName>(tes, Epoch::Init);

    return 0;
}
