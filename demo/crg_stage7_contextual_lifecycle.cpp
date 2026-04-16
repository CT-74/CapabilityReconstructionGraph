// ======================================================
// STAGE 7 — CONTEXTUAL LIFECYCLE (THE FALSE PEAK)
// ======================================================
//
// @intent:
// Attempt to control behavior over time using C++ lifetime semantics (RAII).
//
// @what_changed:
// Behavior nodes are bound to RAII scopes. They register and unregister dynamically.
//
// @key_insight:
// This is a trap. RAII is about execution control, not domain state. Taking the lens 
// cap off a camera doesn't age the subject in front of it.
//
// @what_is_not:
// Not thread-safe (mutates static pointers)
// Not a true temporal dimension
// Not a domain state progression
//
// @transition:
// Discard RAII mutation and introduce a true, thread-safe domain axis.
//
// @spoken_line:
// “We told ourselves we had solved time. But this is a thread-safety nightmare, 
// and worse... it’s a flat lie.”
// ======================================================


#include <iostream>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <string>

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
// TYPE ERASURE TRANSPORT
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

        TypeID GetTypeID() const override
        {
            return TypeIDOf<T>::Get();
        }
    };

    template<class T>
    void Set(T v)
    {
        ptr = std::make_unique<Model<T>>(std::move(v));
    }

    TypeID GetTypeID() const
    {
        return ptr->GetTypeID();
    }

    template<class T>
    const T& Get() const
    {
        return static_cast<Model<T>*>(ptr.get())->value;
    }

private:
    std::unique_ptr<Concept> ptr;
};

// ======================================================
// NODE LIST (runtime linked registry)
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

    ~LinkedNode()
    {
        Unregister();
    }

    Interface* GetNext() const { return m_next; }
    static Interface* GetHead() { return s_head; }

private:
    void Unregister()
    {
        Interface* self = static_cast<Interface*>(static_cast<Derived*>(this));

        // Case 1: head removal
        if (s_head == self)
        {
            s_head = m_next;
            m_next = nullptr;
            return;
        }

        // Case 2: find previous and unlink
        Interface* current = s_head;
        while (current)
        {
            auto* node = static_cast<LinkedNode*>(current);
            if (node->m_next == self)
            {
                node->m_next = m_next;
                m_next = nullptr;
                return;
            }

            current = node->m_next;
        }
    }

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
};

struct IPrintTypeID
{
    virtual ~IPrintTypeID() = default;
    virtual void Execute(const TypeErasedShell&) const = 0;
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
template<class T>
struct PrintNameDefinition
    : IPrintName
    , LinkedNode<PrintNameDefinition<T>, IPrintName>
{
    void Execute(const TypeErasedShell& t) const override
    {
        std::cout << "Generic: " << t.Get<T>().name << "\n";
    }
};

template<>
struct PrintNameDefinition<Boss>
    : IPrintName
    , LinkedNode<PrintNameDefinition<Boss>, IPrintName>
{
    void Execute(const TypeErasedShell& t) const override
    {
        std::cout << "🔥 BOSS: " << t.Get<Boss>().name << "\n";
    }
};

template<>
struct PrintNameDefinition<Player>
    : IPrintName
    , LinkedNode<PrintNameDefinition<Player>, IPrintName>
{
    void Execute(const TypeErasedShell& t) const override
    {
        std::cout << "Player: " << t.Get<Player>().name << "\n";
    }
};

template<class T>
struct PrintTypeIDDefinition
    : IPrintTypeID
    , LinkedNode<PrintTypeIDDefinition<T>, IPrintTypeID>
{
    void Execute(const TypeErasedShell& t) const override
    {
        std::cout << "TypeID=" << t.GetTypeID() << "\n";
    }
};

// ======================================================
// MODEL LIST
// ======================================================
template<class...>
struct TypeList;

using Models = TypeList<NPC, Boss, Player>;

// ======================================================
// BEHAVIOR MATRIX (routing only)
// ======================================================
template<class ModelList, template<class> class... Defs>
struct BehaviorMatrix;

template<class... Models, template<class> class... Defs>
struct BehaviorMatrix<TypeList<Models...>, Defs...>
{
    template<class Interface, class... Subset>
    const Interface* Find(const TypeErasedShell& tes)
    {
        const auto id = tes.GetTypeID();
        const Interface* result = nullptr;

        if constexpr (sizeof...(Subset) > 0)
        {
            const bool allowed = ((id == TypeIDOf<Subset>::Get()) || ...);
            if (!allowed)
                return nullptr;
        }

        ((id == CRG::TypeIDOf<Models>::Get() &&
            (result = Resolve<Interface>())), ...);

        return result;
    }

private:
    template<class Interface>
    static const Interface* Resolve()
    {
        Interface* head = Interface::GetHead();

        // runtime node selection placeholder:
        // iterate linked nodes and pick matching one if needed

        return head;
    }
};


// ======================================================
// DOMAIN BINDING
// ======================================================
using PrintTypeIDBehavior = BehaviorMatrix<
    Models,
    PrintTypeIDDefinition
>;

static const PrintTypeIDBehavior gs_PrintTypeIDBehaviors;

// ======================================================
// API
// ======================================================
template<class Interface, class... Subset>
void Call(const TypeErasedShell& tes)
{
    if (const Interface* iface = Behavior::Find<Interface, Subset...>(tes))
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

    std::cout << "OUTSIDE SCOPE:\n";
    tes.Set(Boss{"Dragon"});
    Call<IPrintName>(tes);
    Call<IPrintTypeID>(tes);

    {
       using PrintNameBehavior = BehaviorMatrix<
            Models,
            PrintNameDefinition
        >; 

        PrintNameBehavior scope;

        std::cout << "INSIDE SCOPE:\n";
        tes.Set(Player{"Alex"});
        Call<IPrintName>(tes);
        Call<IPrintTypeID>(tes);
    }
}
