// Copyright (c) 2026 Cyril Tissier. All rights reserved.
// Licensed under the Apache License, Version 2.0.
//
// =============================================================================
// CAPABILITY CRG - STAGE 2: OPAQUE TRANSPORT
// =============================================================================
//
// @intent:
// Introduce a runtime identity and a type-erased container independent 
// of any logic or fixed class hierarchy.
//
// @architecture:
// - ModelHandle: The "Vessel". A type-erased container for DATA models.
// - Concept/Model: The Bridge. A standard Type Erasure pattern (Sean Parent style)
//   to erase the concrete type T while preserving its Identity.
// - ModelTypeID: A unique identifier generated via typeid().hash_code().
//
// @note:
// For pedagogical clarity, ModelHandle uses std::unique_ptr.
// In a AAA production engine, this structure typically relies on 
// Small Buffer Optimization (SBO) with an internal aligned buffer and 
// placement new to guarantee zero-heap-allocation on the hot path.
// =============================================================================

#include <iostream>
#include <memory>
#include <typeinfo>
#include <string>

// Global identity type
using ModelTypeID = std::size_t;
template<class T> struct TypeIDOf { static ModelTypeID Get() { return typeid(T).hash_code(); } };

// --- 1. THE TRANSPORT (Type-Erased Model Container) ---
class ModelShell {
public:
    // The Bridge: Internal interface to recover Identity
    struct Concept { 
        virtual ~Concept() = default; 
        virtual ModelTypeID GetModelID() const = 0; 
    };

    // The Implementation: Concrete wrapper for any data type T
    template<class T> 
    struct Model : Concept {
        T value;
        Model(T v) : value(std::move(v)) {}
        
        ModelTypeID GetModelID() const override { 
            return TypeIDOf<T>::Get(); 
        }
    };

    ModelShell() = default;

    // Set: Erase the concrete type T but preserve its Identity
    template<class T> 
    void Set(T v) { 
        // Note: Production code would use an SBO buffer and placement new here
        m_ptr = std::make_unique<Model<T>>(std::move(v)); 
    }

    // GetID: Returns the unique identity of the stored model
    ModelTypeID GetID() const { 
        return m_ptr ? m_ptr->GetModelID() : 0; 
    }

    // GetAs: Safely recover the typed data (requires knowing the type)
    template<class T> 
    const T& GetAs() const { 
        // In AAA, a debug assert(GetID() == typeid(T).hash_code()) is mandatory here
        return static_cast<const Model<T>*>(m_ptr.get())->value; 
    }

private:
    std::unique_ptr<Concept> m_ptr;
};

// --- 2. DATA MODELS (Pure State, No Logic, No Base Class) ---
struct Scout     { std::string callsign; };
struct HeavyTank { int armor_plating;   };

// =============================================================================
// MAIN: OPAQUE TRANSPORT DEMO
// =============================================================================

int main() {
    std::cout << "--- CRG STAGE 2: Opaque Transport ---\n\n";

    ModelShell shell;
    
    // Identity 1: The Scout
    shell.Set(Scout{"Vanguard-01"});
    std::cout << "[ID]   Model Type ID: " << shell.GetID() << "\n";
    std::cout << "[DATA] Scout Name:    " << shell.GetAs<Scout>().callsign << "\n\n";

    // Identity 2: The HeavyTank
    shell.Set(HeavyTank{600});
    std::cout << "[ID]   Model Type ID: " << shell.GetID() << "\n";
    std::cout << "[DATA] Tank Armor:    " << shell.GetAs<HeavyTank>().armor_plating << "mm\n";

    return 0;
}