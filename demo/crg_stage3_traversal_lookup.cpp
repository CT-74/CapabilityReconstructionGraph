// ======================================================
// STAGE 3 — IDENTITY-BASED RESOLUTION
// ======================================================
//
// @intent:
// Introduce lookup via traversal instead of mapping.
//
// @what_changed:
// Traversal-based resolution over linked nodes using TypeID matching.
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
#include <memory>
#include <typeinfo>

using TypeID = std::size_t;

template<class T>
struct TypeIDOf { static TypeID Get() { return typeid(T).hash_code(); } };

// --- identity ---
class ErasedTransport
{
public:
    struct Concept { virtual ~Concept() = default; virtual TypeID GetTypeID() const = 0; };

    template<class T>
    struct Model : Concept
    {
        T value;
        Model(T v) : value(std::move(v)) {}
        TypeID GetTypeID() const override { return TypeIDOf<T>::Get(); }
    };

    template<class T>
    void Set(T v) { ptr = std::make_unique<Model<T>>(std::move(v)); }

    TypeID GetTypeID() const { return ptr->GetTypeID(); }

    template<class T>
    const T& Get() const { return static_cast<Model<T>*>(ptr.get())->value; }

private:
    std::unique_ptr<Concept> ptr;
};

// --- behavior space ---
struct INode
{
    virtual ~INode() = default;
    virtual void Execute(const ErasedTransport&) const = 0;
    virtual TypeID TargetType() const = 0;

    INode* GetNext() const { return m_next; }
    static INode* GetHead() { return s_head; }

protected:
    INode() { m_next = s_head; s_head = this; }

private:
    inline static INode* s_head = nullptr;
    INode* m_next = nullptr;
};

template<class T>
struct PrintNode : INode
{
    void Execute(const ErasedTransport& t) const override
    {
        std::cout << t.Get<T>().name << "\n";
    }

    TypeID TargetType() const override
    {
        return TypeIDOf<T>::Get();
    }
};

struct NPC { std::string name{"NPC"}; };
struct Boss { std::string name{"Boss"}; };
struct Player { std::string name{"Player"}; };

template<class Interface>
const Interface* Find(const ErasedTransport& t)
{
    for (auto* n = INode::GetHead(); n; n = n->GetNext())
        if (n->TargetType() == t.GetTypeID())
            return static_cast<const Interface*>(n);

    return nullptr;
}

int main()
{
    PrintNode<NPC> a; PrintNode<Boss> b; PrintNode<Player> c;

    ErasedTransport t;
    t.Set(Boss{"Dragon"});

    if (auto* n = Find<INode>(t))
        n->Execute(t);
}