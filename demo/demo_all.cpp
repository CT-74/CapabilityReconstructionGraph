#include <iostream>
#include <cstdint>

// =====================================================
// SHARED IDENTITY SYSTEM (used across all stages)
// =====================================================

using TypeID = std::size_t;

template<class T>
struct TypeInfo
{
    static TypeID Get()
    {
        static TypeID id = reinterpret_cast<TypeID>(&id);
        return id;
    }
};

// =====================================================
// STAGE 1 — EMERGENT LIST
// =====================================================

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
};

Node* Node::head = nullptr;

// =====================================================
// STAGE 2 — IDENTITY EXTENSION
// =====================================================

struct NodeWithID : Node
{
    virtual TypeID GetID() const = 0;
};

// =====================================================
// STAGE 3 — BASE CONCRETE TYPES
// =====================================================

struct A : NodeWithID
{
    void Run() const override { std::cout << "A\n"; }
    TypeID GetID() const override { return TypeInfo<A>::Get(); }
};

struct B : NodeWithID
{
    void Run() const override { std::cout << "B\n"; }
    TypeID GetID() const override { return TypeInfo<B>::Get(); }
};

struct C : NodeWithID
{
    void Run() const override { std::cout << "C\n"; }
    TypeID GetID() const override { return TypeInfo<C>::Get(); }
};

// =====================================================
// STAGE 4 — EXTERNAL BEHAVIOR SPACE
// =====================================================

struct Behavior
{
    static Behavior* head;
    Behavior* next = nullptr;

    TypeID targetID;

    Behavior(TypeID id)
        : targetID(id)
    {
        next = head;
        head = this;
    }

    virtual void Execute(const NodeWithID& n) const = 0;
};

Behavior* Behavior::head = nullptr;

// -----------------------------------------------------

template<class T>
struct PrintBehavior : Behavior
{
    PrintBehavior()
        : Behavior(TypeInfo<T>::Get())
    {}

    void Execute(const NodeWithID&) const override
    {
        std::cout << "[Behavior] bound execution\n";
    }
};

// =====================================================
// STAGE 5 — FUSION RESOLUTION
// =====================================================

const Behavior* FindBehavior(TypeID id)
{
    for (Behavior* b = Behavior::head; b; b = b->next)
        if (b->targetID == id)
            return b;
    return nullptr;
}

// =====================================================
// INSTANTIATION (THE TWO SPACES)
// =====================================================

// Identity space instances
A a;
B b;
C c;

// Behavior space instances (external definitions)
PrintBehavior<A> ba;
PrintBehavior<B> bb;
PrintBehavior<C> bc;

// =====================================================
// MAIN — PROGRESSION DEMO
// =====================================================

int main()
{
    std::cout << "=============================\n";
    std::cout << " STAGE 1–5 EMERGENT SYSTEM\n";
    std::cout << "=============================\n\n";

    std::cout << "Traversing identity space + resolving behavior:\n\n";

    for (Node* n = Node::head; n; n = n->next)
    {
        auto* typed = dynamic_cast<NodeWithID*>(n);
        if (!typed) continue;

        std::cout << "Node ID = " << typed->GetID() << " -> ";

        if (auto* behavior = FindBehavior(typed->GetID()))
        {
            behavior->Execute(*typed);
        }
        else
        {
            std::cout << "no behavior\n";
        }
    }
}