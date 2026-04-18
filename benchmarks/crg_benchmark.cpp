/*
 * Copyright 2026 Cyril Tissier
 * Licensed under the Apache License, Version 2.0
 * Benchmark: Archetype Fragmentation vs CRG Contextual Observation
 */

#include <benchmark/benchmark.h>
#include <vector>
#include <cstring>
#include <cstdint>

// -----------------------------------------------------------------------------
// 1. DATA SETUP
// -----------------------------------------------------------------------------

// A realistic entity payload (Transform, Velocity, Physics data ~ 256 bytes)
// In a real ECS, Archetype Fragmentation forces the copy of all these components.
struct alignas(32) EntityData {
    uint64_t id;
    float transform[16];
    float physics[32];
    uint64_t padding[4]; 
};

// Mock CRG Capability Interface and Matrix
struct ICapability { virtual void Execute(EntityData& data) = 0; };
struct CapA : ICapability { void Execute(EntityData& data) override { benchmark::DoNotOptimize(data.id += 1); } };
struct CapB : ICapability { void Execute(EntityData& data) override { benchmark::DoNotOptimize(data.id += 2); } };

CapA g_capA;
CapB g_capB;
ICapability* g_crgMatrix[2] = { &g_capA, &g_capB };

// -----------------------------------------------------------------------------
// 2. TRADITIONAL ECS MOCK (Archetype Fragmentation)
// -----------------------------------------------------------------------------
// Simulates changing an entity's state by moving it to a new Archetype block.
static void BM_EcsArchetypeMutation(benchmark::State& state) {
    const size_t entityCount = state.range(0);
    
    std::vector<EntityData> archetype_idle(entityCount);
    std::vector<EntityData> archetype_combat;
    archetype_combat.reserve(entityCount);

    for (auto _ : state) {
        // State Change: Move entities from Idle to Combat Archetype
        for (size_t i = 0; i < entityCount; ++i) {
            archetype_combat.push_back(archetype_idle.back());
            archetype_idle.pop_back();
        }

        // State Change: Move them back (simulate toggling state)
        for (size_t i = 0; i < entityCount; ++i) {
            archetype_idle.push_back(archetype_combat.back());
            archetype_combat.pop_back();
        }
        
        benchmark::ClobberMemory();
    }
    
    // Bytes processed = Entity size * Number of entities * 2 moves per iteration
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(entityCount) * sizeof(EntityData) * 2);
}
BENCHMARK(BM_EcsArchetypeMutation)->RangeMultiplier(4)->Range(1024, 65536);

// -----------------------------------------------------------------------------
// 3. CRG STATELESS MVC MOCK (Contextual Observation)
// -----------------------------------------------------------------------------
// Simulates changing state by shifting an axis coordinate, leaving data contiguous.
static void BM_CrgContextualObservation(benchmark::State& state) {
    const size_t entityCount = state.range(0);
    
    // Data remains perfectly contiguous and NEVER moves.
    std::vector<EntityData> contiguous_data(entityCount);
    // Context axes (e.g., Time, Environment) 
    std::vector<uint8_t> context_axes(entityCount, 0);

    for (auto _ : state) {
        // State Change: Simply observe a new context (O(1) value update)
        for (size_t i = 0; i < entityCount; ++i) {
            context_axes[i] = 1; // Shift to Combat context
            
            // Reconstruct capability and execute (O(1) resolution)
            ICapability* activeCap = g_crgMatrix[context_axes[i]];
            activeCap->Execute(contiguous_data[i]);
        }

        // State Change: Revert context
        for (size_t i = 0; i < entityCount; ++i) {
            context_axes[i] = 0; // Shift to Idle context
            
            ICapability* activeCap = g_crgMatrix[context_axes[i]];
            activeCap->Execute(contiguous_data[i]);
        }
        
        benchmark::ClobberMemory();
    }
    
    // Bytes processed is zero for memory copies, but we count the data touched.
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(entityCount) * sizeof(EntityData) * 2);
}
BENCHMARK(BM_CrgContextualObservation)->RangeMultiplier(4)->Range(1024, 65536);

BENCHMARK_MAIN();