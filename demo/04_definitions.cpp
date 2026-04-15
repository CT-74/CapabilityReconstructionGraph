#include <iostream>
#include <cstdint>

// =========================================
// STEP 4 — EXTERNAL BEHAVIOR DEFINITIONS
// =========================================
//
// Key idea:
// Behavior is no longer inside the node.
// It is defined externally and discovered via identity.
//

using TypeID = std::size_t;

// -------------------------------------------------
// Identity system
// -------------------------------------------------

template<class T>
struct TypeInfo
{
    static TypeID Get()
    {
        static TypeID id = reinterpret_cast<TypeID>(&id);
        return id;
    }
};

// -------------------------------------------------
// Forward declaration
// -------------------------------------------------

struct Node;

// -------------------------------------------------
// BEHAVIOR INTERFACE (external contract)
// -------------------------------------------------

struct IPrint
{
    virtual void Execute(const Node& n) const = 0;
};

// -------------------------------------------------
// EMERGENT LIST (IDENTITY SPACE)
// -------------------------------------------------

struct Node
{
    static Node* head;
    Node* next = nullptr;

    Node()
    {
        next = head;
        head = this;
    }

    virtual ~Node() = default;

    virtual TypeID GetID() const = 0;
};

Node* Node::head = nullptr;

// -------------------------------------------------
// Concrete identity nodes (NO BEHAVIOR HERE)
// -------------------------------------------------

struct A : Node { TypeID GetID() const override { return TypeInfo<A>::Get(); } };
struct B : Node { TypeID GetID() const override { return TypeInfo<B>::Get(); } };
struct C : Node { TypeID GetID() const override { return TypeInfo<C>::Get(); } };

// -------------------------------------------------
// SECOND LIST — BEHAVIOR DEFINITIONS
// -------------------------------------------------

struct Definition
{
    static Definition* head;
    Definition* next = nullptr;

    TypeID targetID;

    Definition(TypeID id)
        : targetID(id)
    {
        next = head;
        head = this;
    }

    virtual void Execute(const Node& n) const = 0;
};

Definition* Definition::head = nullptr;

// -------------------------------------------------
// Concrete behavior definitions
// -------------------------------------------------

template<class T>
struct PrintDef : Definition
{
    PrintDef()
        : Definition(TypeInfo<T>::Get())
    {}

    void Execute(const Node&) const override
    {
        std::cout << "Print behavior for type\n";
    }
};

// instantiate definitions (SECOND SPACE)
PrintDef<A> defA;
PrintDef<B> defB;
PrintDef<C> defC;

// -------------------------------------------------
// RESOLUTION (identity → behavior scan)
// -------------------------------------------------

const Definition* FindDefinition(TypeID id)
{
    for (auto* d = Definition::head; d; d = d->next)
        if (d->targetID == id)
            return d;

    return nullptr;
}

// -------------------------------------------------
// DEMO
// -------------------------------------------------

int main()
{
    A a;
    B b;
    C c;

    std::cout << "Resolving behaviors externally:\n";

    for (Node* n = Node::head; n; n = n->next)
    {
        const Definition* def = FindDefinition(n->GetID());

        if (def)
            def->Execute(*n);
    }
}