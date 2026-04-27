// =============================================================================
// CAPABILITY ROUTING GATEWAY (CRG) - STAGE 2: IDENTITY SPACE
// =============================================================================
// @Intent: Introduce runtime identity independent of logic or structure.
// @Nomenclature:
//   1. ModelHandle: The container for DATA models.
//   2. Concept / Model: Standard Type Erasure bridge (Sean Parent style).
//   3. ModelTypeID: Unique ID generated via hash_code().
// @Key_Insight: 
//   We can now transport "Anything" and know "What" it is at runtime 
//   without a dynamic_cast ladder or a fixed base class.
// =============================================================================

#include <iostream>
#include <memory>
#include <typeinfo>
#include <string>

// Global identity type
using ModelTypeID = std::size_t;

// 1. THE TRANSPORT (Type-Erased Model Container)
class ModelHandle {
public:
    // The Bridge: Internal interface to recover TypeID
    struct Concept { 
        virtual ~Concept() = default; 
        virtual ModelTypeID GetModelID() const = 0; 
    };

    // The Implementation: Concrete wrapper for type T
    template<class T> 
    struct Model : Concept {
        T value;
        Model(T v) : value(std::move(v)) {}
        ModelTypeID GetModelID() const override { return typeid(T).hash_code(); }
    };

    ModelHandle() = default;

    // Set: Erase the concrete type T but keep its Identity
    template<class T> 
    void Set(T v) { 
        m_ptr = std::make_unique<Model<T>>(std::move(v)); 
    }

    // GetID: Returns the identity of the stored model
    ModelTypeID GetID() const { 
        return m_ptr ? m_ptr->GetModelID() : 0; 
    }

    // GetAs: Recover the data (requires knowing the type)
    template<class T> 
    const T& GetAs() const { 
        return static_cast<const Model<T>*>(m_ptr.get())->value; 
    }

private:
    std::unique_ptr<Concept> m_ptr;
};

// 2. DATA MODELS (Pure State, No Logic, No Base Class)
struct Scout { std::string callsign; };
struct HeavyTank { int armor_plating; };

// =============================================================================
// MAIN: IDENTITY DEMO
// =============================================================================

int main() {
    std::cout << "--- CRG STAGE 2: Identity Space ---\n";

    ModelHandle handle;
    
    // Identity 1: The Scout
    handle.Set(Scout{"Vanguard-01"});
    std::cout << "[ID] Model Type ID: " << handle.GetID() << "\n";
    std::cout << "[DATA] Scout Name: " << handle.GetAs<Scout>().callsign << "\n\n";

    // Identity 2: The HeavyTank
    handle.Set(HeavyTank{600});
    std::cout << "[ID] Model Type ID: " << handle.GetID() << "\n";
    std::cout << "[DATA] Tank Armor: " << handle.GetAs<HeavyTank>().armor_plating << "mm\n";

    return 0;
}