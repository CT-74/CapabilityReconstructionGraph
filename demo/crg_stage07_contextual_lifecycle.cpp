// ======================================================
// STAGE 7 — THE NON-GLOBAL LIFECYCLE TRAP (THE FALSE PEAK)
// ======================================================
//
// @intent:
// Attempt to control behavior over time using non-global lifecycles 
// (whether stack-based RAII or heap-allocated objects).
//
// @what_changed:
// LinkedNode destructor attempts to 'Unregister' itself from the global chain.
//
// @key_insight:
// THE TRAP: Tying behavior registration to an individual object's life and death 
// forces you to mutate the shared structural graph at runtime. 
// Taking the lens cap off a camera doesn't age the subject in front of it.
//
// @what_is_not:
// Not thread-safe (mutates static pointers during traversal)
// Not a true temporal dimension
//
// @transition:
// Discard dynamic mutation entirely. Introduce a true, thread-safe domain axis.
// ======================================================

#include <iostream>

template<class Derived, class Interface>
class LinkedNode {
public:
    LinkedNode() {
        m_next = s_head;
        s_head = static_cast<Interface*>(static_cast<Derived*>(this));
    }
    
    // [WARNING: CONCURRENCY HAZARD & MUTATION]
    ~LinkedNode() {
        Unregister();
    }

    Interface* GetNext() const { return m_next; }
    static Interface* GetHead() { return s_head; }

private:
    void Unregister() {
        Interface* self = static_cast<Interface*>(static_cast<Derived*>(this));
        
        if (s_head == self) {
            s_head = m_next;
            m_next = nullptr;
            return;
        }

        Interface* current = s_head;
        while (current) {
            // Assumes Interface has GetNext() for this demo setup
            auto* node = static_cast<LinkedNode<Derived, Interface>*>(current);
            if (node->m_next == self) {
                node->m_next = m_next;
                m_next = nullptr;
                return;
            }
            current = node->m_next;
        }
    }

protected:
    inline static Interface* s_head = nullptr;
    Interface* m_next = nullptr;
};

struct IBehavior { 
    virtual void Execute() = 0; 
    virtual IBehavior* GetNext() const = 0; // Required for unregister loop here
};

struct ScopedBehavior : IBehavior, LinkedNode<ScopedBehavior, IBehavior> {
    void Execute() override { std::cout << "   -> Behavior Executing (Graph is mutated!)\n"; }
    IBehavior* GetNext() const override { return m_next; }
};

void ResolveAndExecute() {
    IBehavior* current = LinkedNode<ScopedBehavior, IBehavior>::GetHead();
    if (!current) {
        std::cout << "   -> No behavior found in global graph.\n";
    }
    while (current) {
        current->Execute();
        break; // Simplified
    }
}

int main() {
    std::cout << "--- STAGE 07: THE NON-GLOBAL LIFECYCLE TRAP ---\n\n";

    std::cout << "[Global State] Before Local Scope:\n";
    ResolveAndExecute();

    {
        std::cout << "\n[Local State] Entering Scope (Allocating behavior)...\n";
        ScopedBehavior b; 
        
        std::cout << "[Local State] Inside Scope:\n";
        ResolveAndExecute(); // It "works", which is why it's a trap.
        
        std::cout << "[Local State] Leaving Scope (Destructor triggers shared graph mutation)...\n";
    }

    std::cout << "\n[Global State] After Scope:\n";
    ResolveAndExecute(); // Behavior is gone. Unsafe in multithreading.

    return 0;
}