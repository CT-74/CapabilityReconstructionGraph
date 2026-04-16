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
#include <cstddef>

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
struct RunDiagnostic : INode
{
    void Execute(const void* obj) const override
    {
        std::cout << static_cast<const T*>(obj)->id << " passes diagnostics.\n";
    }

    TypeID TargetType() const override
    {
        return TypeIDOf<T>::Get();
    }
};

struct Drone { std::string id{"Drone"}; };
struct HeavyLifter { std::string id{"HeavyLifter"}; };

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
    {
        n->Execute(&obj);
    }
}

int main()
{
    RunDiagnostic<Drone> droneDiag;
    RunDiagnostic<HeavyLifter> lifterDiag;

    Drone d{"Scout-1"};
    HeavyLifter h{"Loader-X"};

    Call(d);
    Call(h);

    return 0;
}
