// ======================================================
// STAGE 1 — EMERGENT STRUCTURE
// ======================================================
//
// @intent:
// Introduce runtime structure without an explicit container.
//
// @what_changed:
// Intrusive linked list (self-registering nodes) introduced.
// Unified naming for next/head pointers.
//
// @key_insight:
// Structure can emerge from object lifetime side effects instead of explicit 
// storage. We have a graph without a graph object.
//
// @what_is_not:
// No identity system, no behavior system, no lookup mechanism. Just raw topology.
//
// @transition:
// Nodes exist, but we cannot distinguish them dynamically. We need Identity.
// ======================================================

#include <iostream>

struct IBehavior {
    virtual ~IBehavior() = default;
    virtual void Execute() const = 0;

    const IBehavior* GetNext() const { return m_next; }
    static const IBehavior* GetHead() { return s_head; }

protected:
    IBehavior() {
        m_next = s_head;
        s_head = this;
    }

private:
    inline static const IBehavior* s_head = nullptr;
    const IBehavior* m_next = nullptr;
};

struct DroneBehavior : IBehavior { 
    void Execute() const override { std::cout << "Drone Behavior Active\n"; } 
};

struct HeavyLifterBehavior : IBehavior { 
    void Execute() const override { std::cout << "HeavyLifter Behavior Active\n"; } 
};

int main() {
    DroneBehavior a; 
    HeavyLifterBehavior b; 

    for (const auto* n = IBehavior::GetHead(); n; n = n->GetNext()) {
        n->Execute();
    }
    return 0;
}