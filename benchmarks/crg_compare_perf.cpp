// Copyright (c) 2026 Cyril Tissier. All rights reserved.
// Licensed under the Apache License, Version 2.0.
//
// =============================================================================
// CRG COMPARE PERF - STAGE 12 (V1.0 FINAL)
// =============================================================================
// @intent: Raw Bandwidth Analysis (Gi/s).
// Compares Theoretical Max Speed (Hardcoded Loop) vs Stage 12 Dispatch.
// Demonstrates that Single-Jump Routing still saturates Memory Bandwidth.
// =============================================================================

#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <fstream>

// --- ENGINE CORE: DOD CONTRACTS & OPTIMIZED ACTIVE CAPABILITY ---

struct PhysicsContract {
    struct Params { 
        float* pos; 
        float* vel; 
    };
};

template<class TContract>
struct DODDescriptor {
    void (*pfnExecute)(typename TContract::Params&); 
};

/**
 * @brief Stage 12 Optimized ActiveCapability.
 * Caches the raw function address for the "Muscle" phase.
 */
template<class T>
struct ActiveCapability {
    void (*pfnResolved)(typename T::Params&) = nullptr;

    inline ActiveCapability& operator=(const DODDescriptor<T>& desc) {
        pfnResolved = desc.pfnExecute;
        return *this;
    }

    // Direct jump: The core of the O(1) performance promise.
    inline void operator()(typename T::Params& params) const {
        if (pfnResolved) pfnResolved(params); 
    }
};

// --- BEHAVIORS: STATELESS DOD LOGIC ---

struct BehaviorPatrol {
    static void Execute(PhysicsContract::Params& p) { p.pos[0] += p.vel[0]; }
};

struct BehaviorCombat {
    static void Execute(PhysicsContract::Params& p) { p.pos[0] += p.vel[0] * 2.0f; }
};

// --- INFRASTRUCTURE: DATA LAYOUT ---

// 64-byte Structure (Standard CPU Cache Line size)
struct IPhysics {
    float position[3] = {0.0f, 0.0f, 0.0f};
    float velocity[3] = {1.0f, 1.0f, 1.0f};
    int state = 0;
    char padding[48];
};

// --- BENCHMARK EXECUTION ---

int main(int argc, char* argv[]) {
    size_t entity_count = (argc > 1) ? std::stoull(argv[1]) : 1000000;
    
    std::vector<IPhysics> data(entity_count);
    std::vector<ActiveCapability<PhysicsContract>> caps(entity_count);

    // Prepare descriptors
    DODDescriptor<PhysicsContract> descPatrol { &BehaviorPatrol::Execute };
    DODDescriptor<PhysicsContract> descCombat { &BehaviorCombat::Execute };

    for(size_t i = 0; i < entity_count; ++i) {
        caps[i] = (data[i].state == 0) ? descPatrol : descCombat;
    }

    // =========================================================================
    // 1. BASELINE ECS (Theoretical max speed, hardcoded loop)
    // =========================================================================
    // Represents the absolute hardware ceiling with no indirection.
    auto start_ecs = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < entity_count; ++i) {
        PhysicsContract::Params p { &data[i].position[0], &data[i].velocity[0] };
        BehaviorPatrol::Execute(p); // Hardcoded static call (likely inlined)
    }
    auto end_ecs = std::chrono::high_resolution_clock::now();
    double ecs_time = std::chrono::duration<double, std::milli>(end_ecs - start_ecs).count();

    // =========================================================================
    // 2. STAGE 12 CRG (Optimized Single-Jump Dispatch)
    // =========================================================================
    // Represents the actual engine cost with the "Muscle" phase enabled.
    auto start_crg = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < entity_count; ++i) {
        PhysicsContract::Params p { &data[i].position[0], &data[i].velocity[0] };
        caps[i](p); // Single Jump Routing via cached function pointer
    }
    auto end_crg = std::chrono::high_resolution_clock::now();
    double crg_time = std::chrono::duration<double, std::milli>(end_crg - start_crg).count();

    // Bandwidth Calculation (Gi/s) to analyze memory saturation.
    double total_bytes = (double)entity_count * sizeof(IPhysics);
    double ecs_bw = (total_bytes / (1024.0 * 1024.0 * 1024.0)) / (ecs_time / 1000.0);
    double crg_bw = (total_bytes / (1024.0 * 1024.0 * 1024.0)) / (crg_time / 1000.0);

    // Standard Output Format: Count, ECS_Time, CRG_Time, ECS_BW, CRG_BW
    std::cout << entity_count << "," << ecs_time << "," << crg_time << "," << ecs_bw << "," << crg_bw << std::endl;
    
    return 0;
}