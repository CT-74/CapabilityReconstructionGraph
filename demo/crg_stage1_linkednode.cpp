// ======================================================
// STAGE 1 — EMERGENT STRUCTURE
// ======================================================
//
// @intent:
// Introduce runtime structure without an explicit container.
//
// @what_changed:
// Intrusive linked list (self-registering nodes) introduced.
//
// @key_insight:
// Structure can emerge from object lifetime side effects instead of explicit 
// storage. We have a graph without a graph object.
//
// @what_is_not:
// No identity system, no behavior system, no lookup mechanism. Just raw topology.
//
// @transition:
// Add an identity layer to the structural nodes.
//
// @spoken_line:
// “We already have a structure — but we never built one. It’s just there, 
// in the wake of our objects.”
// ======================================================

#include <iostream>

struct INode
{
    virtual ~INode() = default;
    virtual void Diag() const = 0;

    INode* GetNext() const { return m_next; }
    static INode* GetHead() { return s_head; }

protected:
    INode()
    {
        m_next = s_head;
        s_head = this;
    }

private:
    inline static INode* s_head = nullptr;
    INode* m_next = nullptr;
};

struct DroneNode : INode { 
    void Diag() const override { std::cout << "Drone Node Active\n"; } 
};

struct HeavyLifterNode : INode { 
    void Diag() const override { std::cout << "HeavyLifter Node Active\n"; } 
};

int main()
{
    DroneNode a; 
    HeavyLifterNode b; 

    for (auto* n = INode::GetHead(); n; n = n->GetNext())
    {
        n->Diag();
    }

    return 0;
}
