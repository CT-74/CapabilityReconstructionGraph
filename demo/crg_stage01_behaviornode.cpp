// =============================================================================
// CAPABILITY ROUTING GATEWAY (CRG) - STAGE 1: EMERGENT STRUCTURE
// =============================================================================
// @Intent: Introduce runtime structure without an explicit container.
// @Changes:
//   1. Intrusive Linked List: The base class handles its own registration.
//   2. static inline: No external registry, the head is in the base.
//   3. No CRTP: Pure inheritance. Simple and direct.
// @Key_Insight: 
//   Structure emerges from object lifetime. We have a graph without 
//   a "Manager" object.
// =============================================================================

#include <iostream>

// 1. THE BASE BEHAVIOR (The Self-Registering Interface)
struct IBehavior {
    virtual ~IBehavior() = default;
    virtual void Execute() const = 0;

    // The Plumbing (Manual version, no CRTP yet)
    static inline IBehavior* s_head = nullptr;
    IBehavior* m_next = nullptr;

protected:
    IBehavior() {
        // Intrusive wiring: O(1)
        m_next = s_head;
        s_head = this;
    }
};

// 2. CONCRETE BEHAVIORS (Gameplay)
struct DroneBehavior : IBehavior { 
    void Execute() const override { std::cout << "Drone: Scanning area.\n"; } 
};

struct TankBehavior : IBehavior { 
    void Execute() const override { std::cout << "Tank: Target locked.\n"; } 
};

// =============================================================================
// MAIN: THE DISCOVERY
// =============================================================================

int main() {
    std::cout << "--- CRG STAGE 1: Emergent Structure (Raw) ---\n";

    // Objects register themselves upon construction
    DroneBehavior a; 
    TankBehavior b; 
    DroneBehavior c; 

    // Traversal: We walk the wire via the static head
    for (const IBehavior* n = IBehavior::s_head; n; n = n->m_next) {
        n->Execute();
    }

    return 0;
}