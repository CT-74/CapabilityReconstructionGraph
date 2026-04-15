#pragma once

#include <iostream>
#include <memory>
#include <type_traits>

// ======================================================
// TYPE SYSTEM (DEMO ONLY)
// ======================================================
namespace CRG
{
    using TypeID = std::size_t;

    inline TypeID& GlobalCounter()
    {
        static TypeID c = 0;
        return c;
    }

    template<class T>
    struct TypeIDOf
    {
        static TypeID Get()
        {
            static const TypeID id = ++GlobalCounter();
            return id;
        }
    };
}

// ======================================================
// TYPELIST
// ======================================================
template<class...>
struct TypeList;

// ======================================================
// TYPE ERASURE TRANSPORT (TES)
// ======================================================
class TypeErasedShell
{
public:
    using TypeID = CRG::TypeID;

    struct Concept
    {
        virtual ~Concept() = default;
        virtual TypeID GetTypeID() const = 0;
    };

    template<class T>
    struct Model final : Concept
    {
        T value;

        Model(T v) : value(std::move(v)) {}

        TypeID GetTypeID() const override
        {
            return CRG::TypeIDOf<T>::Get();
        }
    };

    template<class T>
    void Set(T v)
    {
        ptr = std::make_unique<Model<T>>(std::move(v));
    }

    TypeID GetTypeID() const
    {
        return ptr->GetTypeID();
    }

    template<class T>
    const T& Get() const
    {
        return static_cast<Model<T>*>(ptr.get())->value;
    }

private:
    std::unique_ptr<Concept> ptr;
};

// ======================================================
// INTERFACES
// ======================================================
struct IPrintName
{
    virtual ~IPrintName() = default;
    virtual void Execute(const TypeErasedShell&) const = 0;
};

struct IPrintTypeID
{
    virtual ~IPrintTypeID() = default;
    virtual void Execute(const TypeErasedShell&) const = 0;
};

// ======================================================
// DEFINITIONS
// ======================================================
template<class T>
struct PrintNameDefinition : IPrintName
{
    void Execute(const TypeErasedShell& t) const override
    {
        std::cout << "Generic: " << t.Get<T>().name << "\n";
    }
};

template<>
struct PrintNameDefinition<int> : IPrintName
{
    void Execute(const TypeErasedShell&) const override
    {
        std::cout << "Special int case\n";
    }
};

template<class T>
struct PrintTypeIDDefinition : IPrintTypeID
{
    void Execute(const TypeErasedShell& t) const override
    {
        std::cout << "TypeID=" << t.GetTypeID() << "\n";
    }
};

// ======================================================
// CORE MATRIX (ENGINE PRIMITIVE)
// ======================================================
template<class ModelList, template<class> class... Defs>
struct BehaviorMatrix;

// ------------------------------------------------------
// INTERNAL DISPATCH (hidden implementation detail)
// ------------------------------------------------------
template<class... Models, template<class> class... Defs>
struct BehaviorMatrix<TypeList<Models...>, Defs...>
{
    template<class Interface>
    static const Interface* Find(const TypeErasedShell& tes)
    {
        const auto id = tes.GetTypeID();
        const Interface* result = nullptr;

        ((id == CRG::TypeIDOf<Models>::Get() &&
          (result = Get<Interface, Models>())), ...);

        return result;
    }

private:
    template<class Interface, class Model>
    static const Interface* Get()
    {
        return GetImpl<Interface, Model, Defs...>();
    }

    template<class Interface, class Model,
             template<class> class Def,
             template<class> class... Rest>
    static const Interface* GetImpl()
    {
        if constexpr (std::is_base_of_v<Interface, Def<Model>>)
        {
            static Def<Model> instance;
            return &instance;
        }
        else if constexpr (sizeof...(Rest) > 0)
        {
            return GetImpl<Interface, Model, Rest...>();
        }
        else
        {
            return nullptr;
        }
    }
};

// ======================================================
// DOMAIN TYPES
// ======================================================
struct NPC    { std::string name{"NPC"}; };
struct Boss   { std::string name{"Boss"}; };
struct Player { std::string name{"Player"}; };

// ======================================================
// MODEL LIST
// ======================================================
using Models = TypeList<NPC, Boss, Player>;

// ======================================================
// DOMAIN BEHAVIOR (IMPORTANT TEMPLATE ALIAS)
// ======================================================
template<class M>
using Behavior = BehaviorMatrix<
    M,
    PrintNameDefinition,
    PrintTypeIDDefinition
>;

// ======================================================
// STATIC DOMAIN INSTANCE
// ======================================================
static const Behavior<Models> gs_Behavior;

// ======================================================
// API
// ======================================================
template<class Interface>
void Call(const TypeErasedShell& tes)
{
    const Interface* iface = gs_Behavior.template Find<Interface>(tes);
    iface->Execute(tes);
}

// ======================================================
// MAIN (WAOUH DEMO)
// ======================================================
int main()
{
    TypeErasedShell tes;

    tes.Set(NPC{"Grunt"});
    Call<IPrintName>(tes);
    Call<IPrintTypeID>(tes);

    tes.Set(Boss{"Dragon"});
    Call<IPrintName>(tes);
    Call<IPrintTypeID>(tes);

    tes.Set(Player{"Alex"});
    Call<IPrintName>(tes);
    Call<IPrintTypeID>(tes);
}