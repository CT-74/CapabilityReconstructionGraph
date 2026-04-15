#include <iostream>
#include <cstdint>

// =====================================================
// (0) SHARED ID SYSTEM
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
// (1) EMERGENT STRUCTURE — LIST ONLY
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
// (2) IDENTITY LAYER (UNLOCKED STEP)
// =====================================================

struct NodeWithID : Node
{
    virtual TypeID GetID() const = 0;
};

// =====================================================
// (3) CONCRETE TYPES
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
// (4) BEHAVIOR SPACE (EXTERNAL POLYMORPHISM)
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

    virtual void Execute(const NodeWithID&) const = 0;
};

Behavior* Behavior::head = nullptr;

// =====================================================
// (5) BEHAVIOR DEFINITIONS
// =====================================================

template<class T>
struct PrintBehavior : Behavior
{
    PrintBehavior()
        : Behavior(TypeInfo<T>::Get())
    {}

    void Execute(const NodeWithID&) const override
    {
        std::cout << "[behavior] executed\n";
    }
};

// =====================================================
// (6) RESOLUTION (NO MAP EXISTS)
// =====================================================

const Behavior* FindBehavior(TypeID id)
{
    for (Behavior* b = Behavior::head; b; b = b->next)
        if (b->targetID == id)
            return b;

    return nullptr;
}

// =====================================================
// (7) LIVE DEMO SETUP (IMPORTANT FOR TALK FLOW)
// =====================================================

A a;
B b;
C c;

PrintBehavior<A> ba;
PrintBehavior<B> bb;
PrintBehavior<C> bc;

// =====================================================
// (8) MAIN — YOU CONTROL THE STORY HERE
// =====================================================

int main()
{
    std::cout << "=== EMERGENT SYSTEM DEMO ===\n\n";

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