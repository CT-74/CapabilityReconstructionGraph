// ======================================================
// STAGE 7 — THE NON-GLOBAL LIFECYCLE TRAP (THE FALSE PEAK)
// ======================================================
//
// @intent:
// Attempt to control behavior over time using non-global lifecycles 
// (whether stack-based RAII or heap-allocated objects).
//
// @what_changed:
// Behavior nodes are bound to object scopes. They register and unregister dynamically.
//
// @key_insight:
// We often think of RAII as just the stack, but architecturally, an object 
// allocated on the heap during runtime is just a longer, non-global scope. 
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
//
// @spoken_line:
// “Look at this... it works perfectly. We told ourselves we had solved time. 
// But what happens if a background thread is traversing this graph the exact 
// microsecond this object is destroyed? It's a thread-safety nightmare, and 
// worse... it’s an architectural lie.”
// ======================================================

#include <iostream>

template<class Derived, class Interface>
class LinkedNode
{
public:
    LinkedNode()
    {
        // [WARNING: DYNAMIC MUTATION INITIATED]
        m_next = s_head;
        s_head = static_cast<Interface*>(static_cast<Derived*>(this));
    }

    ~LinkedNode()
    {
        // [WARNING: CONCURRENCY HAZARD]
        Unregister();
    }

    Interface* GetNext() const { return m_next; }
    static Interface* GetHead() { return s_head; }

private:
    void Unregister()
    {
        Interface* self = static_cast<Interface*>(static_cast<Derived*>(this));

        if (s_head == self)
        {
            s_head = m_next;
            m_next = nullptr;
            return;
        }

        Interface* current = s_head;
        while (current)
        {
            auto* node = static_cast<LinkedNode*>(current);
            if (node->m_next == self)
            {
                node->m_next = m_next;
                m_next = nullptr;
                return;
            }
            current = node->m_next;
        }
    }

private:
    inline static Interface* s_head = nullptr;
    Interface* m_next = nullptr;
};

struct IBehavior { virtual void Exec() = 0; };

struct ScopedBehavior : IBehavior, LinkedNode<ScopedBehavior, IBehavior> {
    void Exec() override { std::cout << "   -> Behavior Executing (Graph is mutated!)\n"; }
};

// Simulated resolution: scans the graph to find available behaviors
void ResolveAndExecute() {
    IBehavior* current = LinkedNode<ScopedBehavior, IBehavior>::GetHead();
    if (!current) {
        std::cout << "   -> No behavior found.\n";
    }
    while (current) {
        current->Exec();
        // In a real system, current->GetNext() would be called here
        break; // Simplified for the demo
    }
}

int main()
{
    std::cout << "--- Stage 7: The Non-Global Lifecycle Trap ---\n\n";

    std::cout << "[Global State] Before Scope:\n";
    ResolveAndExecute();

    {
        std::cout << "\n[Local State] Entering Non-Global Scope (Stack or Heap)...\n";
        ScopedBehavior b; 
        
        std::cout << "[Local State] Inside Scope:\n";
        ResolveAndExecute(); // IT WORKS! The False Peak.
        
        std::cout << "[Local State] Leaving Scope (Destructor triggers graph mutation)...\n";
    }

    std::cout << "\n[Global State] After Scope:\n";
    ResolveAndExecute();

    std::cout << "\n[Conclusion] It works locally, but it's fundamentally broken globally.\n";
    
    return 0;
}
