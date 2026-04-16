// ======================================================
// STAGE 7 — CONTEXTUAL LIFECYCLE (THE FALSE PEAK)
// ======================================================
//
// @intent:
// Attempt to control behavior over time using C++ lifetime semantics (RAII).
//
// @what_changed:
// Behavior nodes are bound to RAII scopes. They register and unregister dynamically.
//
// @key_insight:
// This is a trap. RAII is about execution control, not domain state. Taking the lens 
// cap off a camera doesn't age the subject in front of it.
//
// @what_is_not:
// Not thread-safe (mutates static pointers)
// Not a true temporal dimension
//
// @transition:
// Discard RAII mutation and introduce a true, thread-safe domain axis.
//
// @spoken_line:
// “We told ourselves we had solved time. But this is a thread-safety nightmare, 
// and worse... it’s a flat lie.”
// ======================================================

#include <iostream>

template<class Derived, class Interface>
class LinkedNode
{
public:
    LinkedNode()
    {
        m_next = s_head;
        s_head = static_cast<Interface*>(static_cast<Derived*>(this));
    }

    ~LinkedNode()
    {
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

struct RAIIBehavior : IBehavior, LinkedNode<RAIIBehavior, IBehavior> {
    void Exec() override { std::cout << "Running...\n"; }
};

int main()
{
    {
        std::cout << "Entering scope...\n";
        RAIIBehavior b;
    }
    std::cout << "Left scope. Unlinking completed (but unsafely in a multi-threaded context).\n";
    return 0;
}
