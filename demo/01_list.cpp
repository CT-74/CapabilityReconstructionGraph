#include <iostream>

// =========================================
// STEP 1 — EMERGENT LIST (SELF-REGISTERING)
// =========================================
//
// Key idea:
// Objects register themselves into a global chain
// without any external container or registry.
//

struct Node
{
    static Node* head;

    Node* next = nullptr;

    Node()
    {
        // self-insert at head
        next = head;
        head = this;
    }

    virtual ~Node() = default;

    virtual void Run() const = 0;
};

// static global head pointer
Node* Node::head = nullptr;

// -----------------------------------------
// Example concrete nodes
// -----------------------------------------

struct A : Node
{
    void Run() const override { std::cout << "A\n"; }
};

struct B : Node
{
    void Run() const override { std::cout << "B\n"; }
};

struct C : Node
{
    void Run() const override { std::cout << "C\n"; }
};

// -----------------------------------------
// Demo
// -----------------------------------------

int main()
{
    A a;
    B b;
    C c;

    std::cout << "Traversing emergent list:\n";

    for (Node* n = Node::head; n; n = n->next)
    {
        n->Run();
    }
}