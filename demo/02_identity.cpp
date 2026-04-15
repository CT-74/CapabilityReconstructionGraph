#include <iostream>
#include <cstdint>

// =========================================
// STEP 2 — IDENTITY LAYER
// =========================================
//
// Key idea:
// We attach a stable runtime identity to each node,
// but still keep the emergent list model.
//

using TypeID = std::size_t;

// -------------------------------------------------
// Minimal identity generator (demo-only)
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
// Base emergent list node
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

    virtual void Run() const = 0;
    virtual TypeID GetID() const = 0;
};

Node* Node::head = nullptr;

// -------------------------------------------------
// Concrete nodes with identity
// -------------------------------------------------

struct A : Node
{
    void Run() const override { std::cout << "A\n"; }
    TypeID GetID() const override { return TypeInfo<A>::Get(); }
};

struct B : Node
{
    void Run() const override { std::cout << "B\n"; }
    TypeID GetID() const override { return TypeInfo<B>::Get(); }
};

struct C : Node
{
    void Run() const override { std::cout << "C\n"; }
    TypeID GetID() const override { return TypeInfo<C>::Get(); }
};

// -------------------------------------------------
// Demo
// -------------------------------------------------

int main()
{
    A a;
    B b;
    C c;

    std::cout << "Traversing nodes WITH identity:\n";

    for (Node* n = Node::head; n; n = n->next)
    {
        std::cout << "ID=" << n->GetID() << " -> ";
        n->Run();
    }
}