// Copyright (c) 2026 Cyril Tissier. All rights reserved.
// Licensed under the Apache License, Version 2.0.

#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <fstream>

// --- ENGINE CORE: DOD CONTRACTS & OPTIMIZED ACTIVE CAPABILITY ---

struct MovementContract {
    struct Params { float* pos; }; 
};

template<class TContract>
struct DODDescriptor {
    void (*pfnExecute)(typename TContract::Params&); 
};

/**
 * @brief Stage 12 Optimized ActiveCapability.
 * Direct pointer storage for O(1) "Muscle" execution[cite: 5].
 */
template<class T>
struct ActiveCapability {
    void (*pfnResolved)(typename T::Params&) = nullptr;

    inline ActiveCapability& operator=(const DODDescriptor<T>& desc) {
        pfnResolved = desc.pfnExecute;
        return *this;
    }

    inline void operator()(typename T::Params& params) const {
        if (pfnResolved) pfnResolved(params); 
    }
};

// --- INFRASTRUCTURE: DATA LAYOUT ---

// 64-byte structure for cache line alignment[cite: 5]
struct IPhysics { 
    int entity_id;
    float pos[3]; 
    float vel[3]; 
    int state; // 0: Patrol, 1: Combat
    char pad[44]; 
};

// --- BEHAVIORS: STATELESS DOD LOGIC ---

struct PatrolLogic {
    static void Execute(MovementContract::Params& p) { p.pos[0] += 1.0f; }
};

struct CombatLogic {
    static void Execute(MovementContract::Params& p) { p.pos[0] += 2.0f; }
};

int main(int argc, char* argv[]) {
    size_t N = (argc > 1) ? std::stoull(argv[1]) : 1000000;
    
    std::vector<IPhysics> phys(N);
    std::vector<ActiveCapability<MovementContract>> caps(N);
    std::vector<int> sparse_set(N); 

    DODDescriptor<MovementContract> descPatrol { &PatrolLogic::Execute };
    DODDescriptor<MovementContract> descCombat { &CombatLogic::Execute };

    for(size_t i=0; i<N; ++i) {
        phys[i].entity_id = (int)i;
        phys[i].state = 0;
        caps[i] = descPatrol;
        sparse_set[i] = (int)i;
    }

    // --- TEST 1: CRG (Value Mutation) ---
    auto s1 = std::chrono::high_resolution_clock::now();
    for(size_t i=0; i<N; ++i) {
        MovementContract::Params p { &phys[i].pos[0] };
        caps[i](p); // Muscle: Execute Logic[cite: 5]

        if (i % 20 == 0) { 
            phys[i].state = 1;
            caps[i] = descCombat; // Brain: Value Mutation (8-byte write)[cite: 5]
        }
    }
    auto e1 = std::chrono::high_resolution_clock::now();
    double crg_time = std::chrono::duration<double, std::milli>(e1 - s1).count();

    // --- TEST 2: ECS (Structural Mutation / Swap & Pop) ---
    for(auto& p : phys) p.state = 0;
    std::vector<IPhysics> combat_archetype; 
    combat_archetype.reserve(N/20);
    size_t current_size = N;

    auto s2 = std::chrono::high_resolution_clock::now();
    for(size_t i = 0; i < current_size; ) {
        // FIXED: Create named variable to avoid temporary-to-non-const-ref error[cite: 5]
        MovementContract::Params p { &phys[i].pos[0] };

        if (phys[i].state == 0) PatrolLogic::Execute(p);
        else CombatLogic::Execute(p);

        if (i % 20 == 0) { 
            phys[i].state = 1;
            combat_archetype.push_back(phys[i]); // Heavy 64-byte move[cite: 5]
            
            phys[i] = phys[current_size - 1]; // Swap & Pop[cite: 5]
            sparse_set[phys[i].entity_id] = (int)i; // Sparse set tracking overhead[cite: 5]
            
            current_size--;
            // Do NOT increment i to process the swapped element[cite: 5]
        } else {
            i++;
        }
    }
    auto e2 = std::chrono::high_resolution_clock::now();
    double ecs_time = std::chrono::duration<double, std::milli>(e2 - s2).count();

    // Console output for Python parsing[cite: 8]
    std::cout << N << "," << ecs_time << "," << crg_time << std::endl;

    std::ofstream out("data/crg_benchmark_bar.csv");
    if (out.is_open()) {
        out << "N,ECS_ms,CRG_ms\n";
        out << N << "," << ecs_time << "," << crg_time << "\n";
    }

    return 0;
}