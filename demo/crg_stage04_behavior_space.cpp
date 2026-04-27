// =============================================================================
// CAPABILITY ROUTING GATEWAY (CRG) - STAGE 4: GLOBAL BEHAVIOR SPACE
// =============================================================================
// @Intent: Establish a generic, interface-agnostic auto-registration mechanism.
// @Nomenclature: 
//   - ModelHandle: Data container.
//   - BehaviorNode<D, I>: CRTP plumbing for any interface.
// @Key_Insight: 
//   Infrastructure and Domain are separated. Any interface can now form its 
//   own global capability space automatically via CRTP.
// @Note: 
//   This version uses std::unique_ptr for clarity (pedagogical version).
//   Production/Ubi version uses SBO (Small Buffer Optimization) for 0-alloc.
// =============================================================================

#include <iostream>
#include <typeinfo>
#include <string>
#include <memory>

using ModelTypeID = std::size_t;

// --- 1. MODEL TRANSPORT (Type Erasure) ---
class ModelHandle {
public:
    struct Concept { 
        virtual ~Concept() = default; 
        virtual ModelTypeID GetModelID() const = 0; 
    };

    template<class T> 
    struct Model : Concept {
        T value; 
        Model(T v) : value(std::move(v)) {}
        ModelTypeID GetModelID() const override { return typeid(T).hash_code(); }
    };

    ModelHandle() = default;

    template<class T> 
    void Set(T v) { 
        m_ptr = std::make_unique<Model<T>>(std::move(v)); 
    }

    ModelTypeID GetID() const { 
        return m_ptr ? m_ptr->GetModelID() : 0; 
    }

    template<class T> 
    const T& GetAs() const { 
        return static_cast<const Model<T>*>(m_ptr.get())->value; 
    }

private:
    std::unique_ptr<Concept> m_ptr;
};

// --- 2. THE INFRASTRUCTURE (CRTP Node) ---

// Variable template (C++17) to store the head of the list per Interface
template <typename Interface>
inline Interface* s_behavior_head = nullptr;

template <typename Derived, typename Interface>
struct BehaviorNode : Interface {
    Interface* m_next = nullptr;

    BehaviorNode() {
        // Automatic O(1) registration upon construction
        m_next = s_behavior_head<Interface>;
        s_behavior_head<Interface> = static_cast<Interface*>(this);
    }
    
    // Technical helper to walk the list since the Interface doesn't know about 'm_next'
    static Interface* GetNext(Interface* current) {
        return static_cast<BehaviorNode*>(current)->m_next;
    }
};

// --- 3. THE DOMAIN (Pure Interface) ---
struct IDiagnostic {
    virtual ~IDiagnostic() = default;
    virtual void Execute(const ModelHandle& h) const = 0;
    virtual ModelTypeID GetTargetModelID() const = 0;
};

// --- 4. GAMEPLAY LOGIC (Behaviors) ---
struct Drone { std::string name; };
struct Tank  { int armor; };

template<class T>
struct DiagBehavior : BehaviorNode<DiagBehavior<T>, IDiagnostic> {
    ModelTypeID GetTargetModelID() const override { return typeid(T).hash_code(); }
    
    void Execute(const ModelHandle& h) const override { 
        std::cout << "[DIAGNOSTIC] Checking Model ID: " << h.GetID() << " | Data hash match verified.\n"; 
    }
};

// Global discovery (Implicit registration via static instantiation)
static DiagBehavior<Drone> g_drone_diag;
static DiagBehavior<Tank>  g_tank_diag;

// --- 5. RESOLUTION (The Routing Gateway) ---
template<class Interface>
const Interface* FindBehavior(const ModelHandle& h) {
    const ModelTypeID id = h.GetID();
    
    // O(N) Traversal of the emergent space
    for (auto* n = s_behavior_head<Interface>; n; ) {
        if (n->GetTargetModelID() == id) return n;
        
        // Use the Node's knowledge to find the next sibling
        n = BehaviorNode<DiagBehavior<Drone>, Interface>::GetNext(n); 
        // Note: For the demo, we cast to a known node type, 
        // in Stage 5, the Baker will flatten this into a table.
    }
    return nullptr;
}

int main() {
    std::cout << "--- CRG STAGE 4: Global Behavior Space (CRTP) ---\n";

    ModelHandle handle;
    
    // Transport Drone Data
    handle.Set(Drone{"Scout-01"});
    if (const auto* diag = FindBehavior<IDiagnostic>(handle)) {
        diag->Execute(handle);
    }

    // Transport Tank Data
    handle.Set(Tank{500});
    if (const auto* diag = FindBehavior<IDiagnostic>(handle)) {
        diag->Execute(handle);
    }

    return 0;
}