#include <iostream>

// ======================================================
// LINKED NODE (CRG V0 PRIMITIVE)
// ======================================================

template<class Derived, class Interface>
class LinkedNode : public Interface
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
// INTERFACE (NO "EXECUTE" ASSUMPTION)
// ======================================================

struct INode
{
    virtual ~INode() = default;

    virtual void PrintName() const = 0;
    virtual void PrintDebug() const = 0;
};

// ======================================================
// NODES (NO DOMAIN, NO SINGLE ACTION MODEL)
// ======================================================

struct NodeA : public LinkedNode<NodeA, INode>
{
    void PrintName() const override { std::cout << "NodeA::Name\n"; }
    void PrintDebug() const override { std::cout << "NodeA::Debug\n"; }
};

struct NodeB : public LinkedNode<NodeB, INode>
{
    void PrintName() const override { std::cout << "NodeB::Name\n"; }
    void PrintDebug() const override { std::cout << "NodeB::Debug\n"; }
};

struct NodeC : public LinkedNode<NodeC, INode>
{
    void PrintName() const override { std::cout << "NodeC::Name\n"; }
    void PrintDebug() const override { std::cout << "NodeC::Debug\n"; }
};

// ======================================================
// TRAVERSAL (IMPORTANT: NO EXECUTE CONCEPT)
// ======================================================

template<class Interface, class Func>
void ForEach(Func&& f)
{
    Interface* node = Interface::GetHead();

    while (node)
    {
        f(*node);
        node = node->GetNext();
    }
}

// ======================================================
// MAIN
// ======================================================

int main()
{
    NodeA a;
    NodeB b;
    NodeC c;

    ForEach<INode>([](INode& node)
    {
        node.PrintName();
        node.PrintDebug();
    });

    return 0;
}