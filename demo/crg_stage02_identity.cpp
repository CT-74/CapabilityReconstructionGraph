// ======================================================
// STAGE 2 — IDENTITY SPACE
// ======================================================
//
// @intent:
// Introduce runtime identity independent of behavior and structure.
//
// @what_changed:
// Type-erased transport (HardwareHandle) added, carrying runtime TypeID + value.
// Uses heap allocation (unique_ptr) for initial simplicity.
//
// @key_insight:
// Identity becomes a first-class citizen. 
//
// @what_is_not:
// No traversal system yet, no behavior system, no registry.
//
// @transition:
// We have an Identity, and we have a Structure (Stage 1). Let's connect them.
// ======================================================

#include <iostream>
#include <memory>
#include <typeinfo>
#include <string>

using TypeID = std::size_t;
template<class T> struct TypeIDOf { static TypeID Get() { return typeid(T).hash_code(); } };

class HardwareHandle {
public:
    struct Concept { 
        virtual ~Concept() = default; 
        virtual TypeID GetTypeID() const = 0; 
    };

    template<class T> struct Model : Concept {
        T value;
        Model(T v) : value(std::move(v)) {}
        TypeID GetTypeID() const override { return TypeIDOf<T>::Get(); }
    };

    template<class T> void Set(T v) { ptr = std::make_unique<Model<T>>(std::move(v)); }
    TypeID GetTypeID() const { return ptr->GetTypeID(); }
    template<class T> const T& Get() const { return static_cast<Model<T>*>(ptr.get())->value; }

private:
    std::unique_ptr<Concept> ptr;
};

struct HeavyLifter { std::string name{"Loader-99"}; };

int main() {
    HardwareHandle handle;
    handle.Set(HeavyLifter{"Loader-99"});

    std::cout << "[IDENTITY] Hash: " << handle.GetTypeID() << "\n";
    std::cout << "[IDENTITY] Name: " << handle.Get<HeavyLifter>().name << "\n";
    return 0;
}