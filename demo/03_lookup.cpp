#include <iostream>
#include <cstdint>

// =========================================
// STEP 3 — EMERGENT LOOKUP (NO MAP EXISTS)
// =========================================
//
// Key idea:
// We simulate "lookup" by scanning the emergent list.
// There is still no container or hash map.
//

using TypeID = std::size_t;

// -------------------------------------------------
// Type identity helper
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
// Emergent list
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
    virtual void Run() const = 0;
};

Node* Node::head = nullptr;

// -------------------------------------------------
// Concrete nodes
// -------------------------------------------------

struct A : Node
{
    TypeID GetID() const override { return TypeInfo<A>::Get(); }
    void Run() const override { std::cout << "A\n"; }
};

struct B : Node
{
    TypeID GetID() const override { return TypeInfo<B>::Get(); }
    void Run() const override { std::cout << "B\n"; }
};

struct C : Node
{
    TypeID GetID() const override { return TypeInfo<C>::Get(); }
    void Run() const override { std::cout << "C\n"; }
};

// -------------------------------------------------
// EMERGENT LOOKUP FUNCTION
// -------------------------------------------------

Node* Find(TypeID id)
{
    for (Node* n = Node::head; n; n = n->next)
    {
        if (n->GetID() == id)
            return n;
    }
    return nullptr;
}

// -------------------------------------------------
// Demo
// -------------------------------------------------

int main()
{
    A a;
    B b;
    C c;

    std::cout << "List traversal:\n";
    for (Node* n = Node::head; n; n = n->next)
        n->Run();

    std::cout << "\nEmergent lookup:\n";

    Node* found = Find(TypeInfo<B>::Get());
    if (found)
        found->Run();
}