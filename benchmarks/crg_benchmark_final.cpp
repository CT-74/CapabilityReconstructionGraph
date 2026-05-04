// Copyright (c) 2026 Cyril Tissier. All rights reserved.
// Licensed under the Apache License, Version 2.0.

#include <iostream>
#include <vector>
#include <chrono>
#include <fstream>

struct EnergyContract { struct Params { float& battery; }; };

template<class TContract>
struct DODDescriptor { void (*pfnExecute)(typename TContract::Params&); };

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

struct DrainLogic { static void Execute(EnergyContract::Params& p) { p.battery -= 1.0f; } };
struct CombatLogic { static void Execute(EnergyContract::Params& p) { p.battery -= 10.0f; } };

struct IPhysics { 
    float battery = 100.0f;
    int state = 0; 
    char pad[56]; // 64-byte alignment
};

void RunTest(size_t N, float mutation_rate, std::ofstream& out) {
    std::vector<IPhysics> entities(N);
    std::vector<ActiveCapability<EnergyContract>> active_caps(N);
    
    DODDescriptor<EnergyContract> descPatrol { &DrainLogic::Execute };
    DODDescriptor<EnergyContract> descCombat { &CombatLogic::Execute };

    int mod_trigger = (mutation_rate <= 0) ? 9999999 : (int)(1.0f / mutation_rate);

    // [ TEST 1 ] CRG (Structural Immunity)
    auto s1 = std::chrono::high_resolution_clock::now();
    for(size_t i=0; i<N; ++i) {
        EnergyContract::Params p { entities[i].battery };
        active_caps[i](p); // Muscle execution

        if (i % mod_trigger == 0) { // Brain resolution
            entities[i].state = (entities[i].state == 0) ? 1 : 0;
            active_caps[i] = (entities[i].state == 0) ? descPatrol : descCombat;
        }
    }
    auto e1 = std::chrono::high_resolution_clock::now();
    double crg_ns = std::chrono::duration<double, std::nano>(e1 - s1).count() / N;

    // [ TEST 2 ] ECS (Structural Mutation: Swap & Pop)
    size_t current_N = N;
    auto s2 = std::chrono::high_resolution_clock::now();
    for(size_t i = 0; i < current_N; ) {
        EnergyContract::Params p { entities[i].battery };
        if (entities[i].state == 0) DrainLogic::Execute(p);
        else CombatLogic::Execute(p);

        if (i % mod_trigger == 0) {
            entities[i] = entities[current_N - 1]; // Swap overhead[cite: 3]
            current_N--; 
            // Do NOT increment i to process the newly swapped element
        } else {
            i++;
        }
    }
    auto e2 = std::chrono::high_resolution_clock::now();
    double ecs_ns = std::chrono::duration<double, std::nano>(e2 - s2).count() / N;

    out << N << "," << mutation_rate << "," << ecs_ns << "," << crg_ns << "\n";
}

int main() {
    std::ofstream out("data/crg_benchmark_final.csv");
    if (!out.is_open()) return 1;

    out << "N,Rate,ECS_ns,CRG_ns\n";
    float rates[] = {0.01f, 0.10f};
    for (float rate : rates) {
        for (int n : {65536, 1048576}) {
            RunTest(n, rate, out);
        }
    }
    return 0;
}