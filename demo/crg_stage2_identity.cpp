// ======================================================
// STAGE 2 — IDENTITY SPACE
// ======================================================
//
// @intent:
// Introduce runtime identity independent of behavior and structure.
//
// @what_changed:
// Type-erased transport added carrying runtime TypeID + value.
//
// @key_insight:
// Identity becomes a first-class citizen. (Note: Demo uses unique_ptr for simplicity, 
// but the engine will evolve to SBO for a zero-allocation transport).
//
// @what_is_not:
// No traversal system yet, no behavior system, no registry.
//
// @transition:
// Introduce traversal-based association between identity and structure.
//
// @spoken_line:
// “Now the objects carry a name and a type — but they still don't know 
// what they can do.”
// ======================================================

#include <iostream>
#include <memory>
#include <typeinfo>
#include <string>

using TypeID = std::size_t;

template<class T>
struct TypeIDOf { 
    static TypeID Get() { return typeid(T).hash_code(); } 
};

class HardwareShell
{
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

    template<class T>
    void Set(T v) { 
        ptr = std::make_unique<Model<T>>(std::move(v)); 
    }

    TypeID GetTypeID() const { return ptr->GetTypeID(); }

    template<class T>
    const T& Get() const { 
        return static_cast<Model<T>*>(ptr.get())->value; 
    }

private:
    std::unique_ptr<Concept> ptr;
};

struct HeavyLifter { std::string id{"HeavyLifter"}; };

int main()
{
    HardwareShell shell;
    shell.Set(HeavyLifter{"Loader-99"});

    std::cout << "[SHELL] Stored Identity Hash: " << shell.GetTypeID() << "\n";
    std::cout << "[SHELL] Stored Payload ID: " << shell.Get<HeavyLifter>().id << "\n";

    return 0;
}
