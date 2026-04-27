// ======================================================
// STAGE 0 — THE GOD REGISTRY (The Bottleneck)
// ======================================================
//
// @Intent: 
// Show a system where discovery is centralized and synchronized.
//
// @Pain_Points:
// 1. Thread Contention: The global mutex serializes every single creation.
// 2. Rigid Manager: The "God Manager" uses a dynamic_cast ladder (O(N) types).
// 3. Binary Coupling: The manager must include every concrete header.
// ======================================================

#include <iostream>
#include <vector>
#include <mutex>
#include <algorithm>

struct IBehavior { virtual ~IBehavior() = default; };

// 1. THE GOD REGISTRY (The Performance Killer)
struct GodRegistry {
    static inline std::mutex s_mtx;
    static inline std::vector<IBehavior*> s_entities;

    static void Register(IBehavior* b) {
        // High contention point: every thread blocks here during Spawn
        std::lock_guard<std::mutex> lock(s_mtx); 
        s_entities.push_back(b);
    }

    static void Unregister(IBehavior* b) {
        std::lock_guard<std::mutex> lock(s_mtx);
        s_entities.erase(std::remove(s_entities.begin(), s_entities.end(), b), s_entities.end());
    }
};

// 2. THE BASE (Implicit registration)
struct BehaviorBase : IBehavior {
    BehaviorBase() { GodRegistry::Register(this); }
    virtual ~BehaviorBase() { GodRegistry::Unregister(this); }
};

// 3. CONCRETE GAMEPLAY (The Types)
struct Drone : BehaviorBase { void Scan() { std::cout << "Drone Scanning...\n"; } };
struct Tank  : BehaviorBase { void Fire() { std::cout << "Tank Firing!\n"; } };

// 4. THE GOD CONTROLLER (The Maintenance Nightmare)
class BehaviorManager {
public:
    void UpdateAll() {
        // The dynamic_cast ladder: Slow and rigid.
        for (auto* b : GodRegistry::s_entities) {
            if (auto* d = dynamic_cast<Drone*>(b)) d->Scan();
            else if (auto* t = dynamic_cast<Tank*>(b)) t->Fire();
            // Adding a new type here requires recompiling this manager!
        }
    }
};

int main() {
    Drone d1, d2;
    Tank t1;

    BehaviorManager manager;
    manager.UpdateAll();

    return 0;
}