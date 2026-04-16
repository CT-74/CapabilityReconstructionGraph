// ======================================================
// STAGE 4 — GLOBAL BEHAVIOR SPACE
// ======================================================
//
// @intent:
// Establish a unified, global linked runtime behavior registry.
//
// @what_changed:
// Behaviors are defined as independent nodes in a global linked list.
//
// @key_insight:
// Everything exists. All potential behaviors are globally visible and 
// equally reachable by the resolution engine.
//
// @what_is_not:
// No filtering, no selection rules, no lifecycle control.
//
// @transition:
// Constrain visibility to make the system manageable.
//
// @spoken_line:
// “Everything exists — nothing is selected. We have built a universe 
// of possibilities.”
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

template<class T>
const INode* Resolve()
{
    const TypeID id = TypeIDOf<T>::Get();

    for (auto* n = INode::head; n; n = n->next)
    {
        if (n->TargetType() == id)
            return n;
    }

    return nullptr;
}

template<class T>
void Call(const T& obj)
{
    if (auto* n = Resolve<T>())
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
}