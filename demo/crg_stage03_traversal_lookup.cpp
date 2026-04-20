// ======================================================
// STAGE 3 — IDENTITY-BASED RESOLUTION (SBO)
// ======================================================
//
// @intent:
// Introduce lookup via traversal instead of mapping, and move to zero-allocation.
//
// @what_changed:
// Traversal-based resolution over linked nodes using TypeID matching.
// HardwareShell evolves into HardwareHandle using Small Buffer Optimization (SBO).
//
// @key_insight:
// We don't need a hash map. Lookup is an emergent property of walking 
// the structure we already have.
//
// @what_is_not:
// No centralized dispatch, no global behavior space yet.
//
// @transition:
// Decouple behaviors from structural nodes into their own unified space.
//
// @spoken_line:
// “This already behaves like a lookup — without being one. We are 
// starting to reconstruct information.”
// ======================================================

#include <iostream>
#include <cstddef>
#include <typeinfo>
#include <string>

using TypeID = std::size_t;

template<class T>
struct TypeIDOf { static TypeID Get() { return typeid(T).hash_code(); } };

// --- Identity (SBO Transport) ---
class HardwareHandle
{
    static constexpr std::size_t SBO_SIZE = 48;

public:
    struct Concept { 
        virtual ~Concept() = default; 
        virtual TypeID GetTypeID() const = 0; 
    };

    template<class T>
    struct Model : Concept
    {
        T value;
        Model(T v) : value(std::move(v)) {}
        TypeID GetTypeID() const override { return TypeIDOf<T>::Get(); }
    };

    HardwareHandle() = default;
    
    ~HardwareHandle() { Reset(); }

    template<class T>
    void Set(T v) 
    {
        static_assert(sizeof(Model<T>) <= SBO_SIZE, "SBO Capacity Exceeded!");
        Reset();
        new (&m_buffer) Model<T>(std::move(v));
        m_set = true;
    }

    TypeID GetTypeID() const { 
        return reinterpret_cast<const Concept*>(&m_buffer)->GetTypeID(); 
    }

    template<class T>
    const T& Get() const { 
        return static_cast<const Model<T>*>(reinterpret_cast<const Concept*>(&m_buffer))->value; 
    }

private:
    void Reset() 
    {
        if (m_set) 
        { 
            reinterpret_cast<Concept*>(&m_buffer)->~Concept(); 
            m_set = false; 
        }
    }

    alignas(std::max_align_t) std::byte m_buffer[SBO_SIZE];
    bool m_set = false;
};

// --- Behavior Space ---
struct INode
{
    virtual ~INode() = default;
    virtual void Execute(const HardwareHandle&) const = 0;
    virtual TypeID TargetType() const = 0;

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

template<class T>
struct DiagNode : INode
{
    void Execute(const HardwareHandle& h) const override
    {
        std::cout << h.Get<T>().id << " diagnostic complete.\n";
    }

    TypeID TargetType() const override
    {
        return TypeIDOf<T>::Get();
    }
};

struct Drone { std::string id{"Drone"}; };
struct HeavyLifter { std::string id{"HeavyLifter"}; };

template<class Interface>
const Interface* Find(const HardwareHandle& h)
{
    for (auto* n = INode::GetHead(); n; n = n->GetNext())
    {
        if (n->TargetType() == h.GetTypeID())
        {
            return static_cast<const Interface*>(n);
        }
    }
    return nullptr;
}

int main()
{
    DiagNode<Drone> a; 
    DiagNode<HeavyLifter> b;

    HardwareHandle h;
    h.Set(HeavyLifter{"Loader-99"});

    if (auto* n = Find<INode>(h))
    {
        n->Execute(h);
    }

    return 0;
}
