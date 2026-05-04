// Copyright (c) 2026 Cyril Tissier. All rights reserved.
// Licensed under the Apache License, Version 2.0.
//
// =============================================================================
// CRG ARCHITECT BENCHMARK - STAGE 12 (V1.0 FINAL)
// =============================================================================
// @intent: Measure the "Dispatch Tax" of Direct Function Pointer Routing
// compared to idealized hardcoded ECS loops.
// Optimized for Single-Jump Execution (O(1) Muscle Phase).
// =============================================================================

#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <type_traits>
#include <fstream>
#include <unordered_map>

// --- ENGINE CORE: ANCHORING & IDENTITY ---

template<class T> struct UniversalAnchor {
    static inline T s_Value{}; 
};

using ModelTypeID = std::size_t; 
template<class T> struct TypeIDOf { static ModelTypeID Get() { return typeid(T).hash_code(); } };

// --- ENGINE CORE: TENSOR ROUTING (HORNER'S METHOD) ---

template<typename T> struct EnumTraits;
template<> struct EnumTraits<int> { static constexpr std::size_t Count = 2; }; // State: 0 or 1

template<class... TAxes>
struct CapabilitySpace {
    static constexpr std::size_t Dimensions = sizeof...(TAxes);
    static constexpr std::size_t Volume = (Dimensions == 0) ? 1 : (EnumTraits<TAxes>::Count * ... * 1);

    template<typename... TArgs>
    static constexpr std::size_t ComputeOffset(const TArgs&... args) {
        if constexpr (Dimensions == 0) return 0;
        else return ComputeInternal(std::tie(args...), std::make_index_sequence<Dimensions>{});
    }
private:
    template<typename TupleT, std::size_t... Is>
    static constexpr std::size_t ComputeInternal(const TupleT& t, std::index_sequence<Is...>) {
        const std::size_t coords[] = { static_cast<std::size_t>(std::get<Is>(t))... };
        constexpr std::size_t dims[] = { EnumTraits<TAxes>::Count... };
        std::size_t offset = 0;
        for (std::size_t i = 0; i < Dimensions; ++i) offset = offset * dims[i] + coords[i];
        return offset;
    }
};

// --- ENGINE CORE: DOD CONTRACTS & OPTIMIZED ACTIVE CAPABILITY ---

struct MovementContract {
    struct Params { float* pos; }; // DOD Parameter block
};

template<class TContract>
struct DODDescriptor {
    void (*pfnExecute)(typename TContract::Params&); 
};

/**
 * @brief Stage 12 Optimized ActiveCapability.
 * Caches the raw function address to achieve single-jump execution.
 */
template<class T>
struct ActiveCapability {
    void (*pfnResolved)(typename T::Params&) = nullptr;

    inline ActiveCapability& operator=(const DODDescriptor<T>& desc) {
        pfnResolved = desc.pfnExecute;
        return *this;
    }

    inline void operator()(typename T::Params& params) const {
        if (pfnResolved) pfnResolved(params); // Single Indirection Call
    }
};

// --- INFRASTRUCTURE: DATA LAYOUT ---

// 64-byte structure to align with CPU cache lines
struct IPhysics { 
    float pos[3]; 
    float vel[3]; 
    int state; 
    char pad[44]; 
};

struct IDCard { ModelTypeID logic_class; };

// --- BEHAVIORS: STATELESS DOD LOGIC ---

struct Scout {};

struct PatrolLogic {
    static void Execute(MovementContract::Params& p) { p.pos[0] += 1.0f; }
};

struct CombatLogic {
    static void Execute(MovementContract::Params& p) { p.pos[0] += 2.0f; }
};

// --- BENCHMARK SUITE ---

void RunTest(size_t N, std::ofstream& out) {
    std::vector<IPhysics> phys(N);
    std::vector<ActiveCapability<MovementContract>> caps(N);
    
    // Preparation: Create descriptors
    DODDescriptor<MovementContract> descPatrol { &PatrolLogic::Execute };
    DODDescriptor<MovementContract> descCombat { &CombatLogic::Execute };

    for(size_t i=0; i<N; ++i) {
        phys[i].state = (i % 20 == 0) ? 1 : 0;
        // The "Brain" has already resolved the routing
        caps[i] = (phys[i].state == 0) ? descPatrol : descCombat;
    }

    // [ TEST 1 ] CRG Benchmark: Direct Function Pointer Execution (The Muscle)
    auto s1 = std::chrono::high_resolution_clock::now();
    for(size_t j=0; j<N; ++j) {
        MovementContract::Params p { &phys[j].pos[0] };
        caps[j](p); // Single Jump Call
    }
    auto e1 = std::chrono::high_resolution_clock::now();
    double crg_ns = std::chrono::duration<double, std::nano>(e1 - s1).count() / N;

    // [ TEST 2 ] ECS Benchmark: Idealized hardcoded direct loop baseline
    auto s2 = std::chrono::high_resolution_clock::now();
    for(size_t j=0; j<N; ++j) {
        if (phys[j].state == 0) phys[j].pos[0] += 1.0f;
        else phys[j].pos[0] += 2.0f;
    }
    auto e2 = std::chrono::high_resolution_clock::now();
    double ecs_ns = std::chrono::duration<double, std::nano>(e2 - s2).count() / N;

    out << N << "," << ecs_ns << "," << crg_ns << "\n";
}

int main() {
    // Ensure data directory exists
    std::ofstream out("data/crg_benchmark_architect.csv");
    if (!out.is_open()) {
        std::cerr << "Error: Could not open data/crg_benchmark_architect.csv\n";
        return 1;
    }
    
    out << "N,ECS_ns,CRG_ns\n";
    std::cout << "Starting Architect Benchmark (Stage 12 Direct-Ptr)..." << std::endl;

    // Power-of-two entity scaling
    for (int i = 10; i <= 24; ++i) { 
        size_t N = std::pow(2, i);
        std::cout << "Testing N = " << N << "..." << std::endl;
        RunTest(N, out);
    }
    
    std::cout << "Done: Results saved in data/crg_benchmark_architect.csv" << std::endl;
    return 0;
}