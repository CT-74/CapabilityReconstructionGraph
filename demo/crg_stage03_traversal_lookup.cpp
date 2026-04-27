// =============================================================================
// CAPABILITY ROUTING GATEWAY (CRG) - STAGE 3: IDENTITY-BASED RESOLUTION (SBO)
// =============================================================================
// @Intent: Resolve behavior by matching Model Identity during O(N) traversal.
// @Changes:
//   1. ModelHandle: Transport for DATA models, using 48-byte SBO (Zero-Alloc).
//   2. s_behavior_head: Variable template to ensure a shared head per Interface.
//   3. BehaviorNode: CRTP plumbing for auto-registration and m_next management.
//   4. IMovement: Pure domain interface, mandates GetTargetModelID() for lookup.
// =============================================================================

#include <iostream>
#include <typeinfo>
#include <string>
#include <cstddef>

using ModelTypeID = std::size_t;

// 1. THE TRANSPORT (Stateless Model Container with SBO)
class ModelHandle {
    static constexpr std::size_t SBO_SIZE = 48;
public:
    struct Concept { 
        virtual ~Concept() = default; 
        virtual ModelTypeID GetModelID() const = 0; 
    };

    template<class T> 
    struct Model : Concept {
        T data; 
        Model(T v) : data(std::move(v)) {}
        ModelTypeID GetModelID() const override { return typeid(T).hash_code(); }
    };

    ModelHandle() = default;
    ~ModelHandle() { Reset(); }

    template<class T> 
    void Set(T v) {
        Reset();
        static_assert(sizeof(Model<T>) <= SBO_SIZE, "Model too large for SBO");
        new (&m_buffer) Model<T>(std::move(v)); // Placement new in SBO buffer
        m_set = true;
    }

    ModelTypeID GetID() const { 
        return m_set ? reinterpret_cast<const Concept*>(&m_buffer)->GetModelID() : 0; 
    }

    template<class T> 
    const T& GetAs() const { 
        return reinterpret_cast<const Model<T>*>(&m_buffer)->data; 
    }

private:
    void Reset() { if(m_set) { reinterpret_cast<Concept*>(&m_buffer)->~Concept(); m_set = false; } }
    alignas(std::max_align_t) std::byte m_buffer[SBO_SIZE];
    bool m_set = false;
};

// 2. THE PLUMBING (Shared Head & CRTP Node)
// Variable template to guarantee ONE head per Interface type
template <typename Interface>
inline Interface* s_behavior_head = nullptr;

template <typename Derived, typename Interface>
struct BehaviorNode : Interface {
    Interface* m_next = nullptr;

    BehaviorNode() {
        // Intrusive wiring: O(1) side-effect of construction
        m_next = s_behavior_head<Interface>;
        s_behavior_head<Interface> = static_cast<Interface*>(this);
    }
};

// 3. THE DOMAIN (Pure Interface)
struct IMovement {
    virtual ~IMovement() = default;
    virtual ModelTypeID GetTargetModelID() const = 0;
    virtual void Execute(const ModelHandle& h) const = 0;
};

// 4. THE LOOKUP ENGINE (O(N) Traversal)
template<class Interface>
const Interface* FindBehavior(const ModelHandle& h) {
    // We walk the intrusive list for this specific interface
    for (auto* n = s_behavior_head<Interface>; n; ) {
        if (n->GetTargetModelID() == h.GetID()) return n;
        
        // Recover the 'm_next' pointer by casting back to the node layer
        // In Stage 3, we assume all implementations are BehaviorNodes.
        n = static_cast<BehaviorNode<void, Interface>*>(reinterpret_cast<void*>(n))->m_next;
    }
    return nullptr;
}

// 5. CONCRETE BEHAVIORS (Gameplay Logic)
struct Scout { std::string callsign; };
struct ScoutMove : BehaviorNode<ScoutMove, IMovement> {
    ModelTypeID GetTargetModelID() const override { return typeid(Scout).hash_code(); }
    void Execute(const ModelHandle& h) const override {
        std::cout << "Scout [" << h.GetAs<Scout>().callsign << "] engine at 100%.\n";
    }
};

struct HeavyTank { int armor; };
struct TankMove : BehaviorNode<TankMove, IMovement> {
    ModelTypeID GetTargetModelID() const override { return typeid(HeavyTank).hash_code(); }
    void Execute(const ModelHandle& h) const override {
        std::cout << "Tank (" << h.GetAs<HeavyTank>().armor << "mm) rotating treads.\n";
    }
};

// Global instances for discovery
static ScoutMove g_scoutLogic;
static TankMove  g_tankLogic;

int main() {
    std::cout << "--- CRG STAGE 3: Identity-Based Resolution ---\n";
    ModelHandle handle;
    
    // Test 1: Scout
    handle.Set(Scout{"Vanguard-01"});
    if (auto* logic = FindBehavior<IMovement>(handle)) logic->Execute(handle);

    // Test 2: HeavyTank
    handle.Set(HeavyTank{600});
    if (auto* logic = FindBehavior<IMovement>(handle)) logic->Execute(handle);

    return 0;
}