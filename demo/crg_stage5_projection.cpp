// ======================================================
// STAGE 5 — PROJECTION (VISIBILITY)
// ======================================================
//
// @intent:
// Introduce compile-time constrained visibility over the behavior space.
//
// @what_changed:
// Variadic template filtering (Subset...) added to the resolution call.
//
// @key_insight:
// Projection is a lens, not a structure. We don't change the system; 
// we change what we are allowed to see.
//
// @what_is_not:
// No runtime registry duplication, no new storage layer.
//
// @transition:
// Formally define the rule for selecting the final capability.
//
// @spoken_line:
// “We do not change the system — we constrain our own gaze. Only a 
// subset of reality is now visible.”
// ======================================================


#include <iostream>
#include <string>
#include <typeinfo>

using TypeID = std::size_t;

template<class T>
struct TypeIDOf
{
    static TypeID Get() { return typeid(T).hash_code(); }
};

struct INode
{
    virtual ~INode() = default;
    virtual void Execute(const void*) const = 0;
    virtual TypeID TargetType() const = 0;

    INode* next = nullptr;
    static inline INode* head = nullptr;

protected:
    INode()
    {
        next = head;
        head = this;
    }
};

template<class T>
struct PrintName : INode
{
    void Execute(const void* obj) const override
    {
        std::cout << static_cast<const T*>(obj)->name << "\n";
    }

    TypeID TargetType() const override
    {
        return TypeIDOf<T>::Get();
    }
};

struct NPC { std::string name{"NPC"}; };
struct Boss { std::string name{"Boss"}; };
struct Player { std::string name{"Player"}; };

template<class T, class... Subset>
const INode* Resolve()
{
    const TypeID id = TypeIDOf<T>::Get();

    if constexpr (sizeof...(Subset) > 0)
    {
        const bool allowed = ((id == TypeIDOf<Subset>::Get()) || ...);
        if (!allowed)
            return nullptr;
    }

    for (auto* n = INode::head; n; n = n->next)
    {
        if (n->TargetType() == id)
            return n;
    }

    return nullptr;
}

template<class T, class... Subset>
void Call(const T& obj)
{
    if (auto* n = Resolve<T, Subset...>())
        n->Execute(&obj);
}

int main()
{
    PrintName<NPC> npc;
    PrintName<Boss> boss;
    PrintName<Player> player;

    NPC n{"Grunt"};
    Boss b{"Dragon"};
    Player p{"Alex"};

    Call(n);
    Call(b);
    Call(p);

    Call<Boss, Boss>(b);
}