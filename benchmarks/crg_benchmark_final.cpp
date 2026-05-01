#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <fstream>

// --- INFRASTRUCTURE: CRG STAGE 12 (ACTIVE CAPABILITY) ---
struct EnergyContract { 
    struct Params { float& battery; }; 
};

struct DODDescriptor { 
    void (*pfnExecute)(EnergyContract::Params); // Passed by value to accept temporaries
};

// Logic implementations
inline void DrainLogic(EnergyContract::Params p) { p.battery -= 1.0f; }
inline void CombatLogic(EnergyContract::Params p) { p.battery -= 10.0f; }

struct IPhysics { 
    float battery = 100.0f;
    int state = 0; // 0: Patrol, 1: Combat
    char pad[56];  // 64-byte alignment
};

void RunTest(size_t N, float mutation_rate, std::ofstream& out) {
    std::vector<IPhysics> entities(N);
    std::vector<DODDescriptor> active_caps(N);
    DODDescriptor desc_patrol { &DrainLogic };
    DODDescriptor desc_combat { &CombatLogic };

    int mod_trigger = (mutation_rate <= 0) ? 9999999 : (int)(1.0f / mutation_rate);

    // --- BENCHMARK CRG (Stage 12 Symbiosis) ---
    auto s1 = std::chrono::high_resolution_clock::now();
    for(size_t i=0; i<N; ++i) {
        active_caps[i] = (entities[i].state == 0) ? desc_patrol : desc_combat;
        active_caps[i].pfnExecute({ entities[i].battery }); // Works with temporary

        if (i % mod_trigger == 0) entities[i].state = (entities[i].state == 0) ? 1 : 0;
    }
    auto e1 = std::chrono::high_resolution_clock::now();
    double crg_ns = std::chrono::duration<double, std::nano>(e1 - s1).count() / N;

    // --- BENCHMARK ECS (Structural Mutation: Swap & Pop) ---
    size_t current_N = N;
    auto s2 = std::chrono::high_resolution_clock::now();
    for(size_t i=0; i<current_N; ++i) {
        if (entities[i].state == 0) DrainLogic({entities[i].battery});
        else CombatLogic({entities[i].battery});

        if (i % mod_trigger == 0) {
            entities[i] = entities[current_N - 1]; 
            current_N--; 
            i--;
        }
    }
    auto e2 = std::chrono::high_resolution_clock::now();
    double ecs_ns = std::chrono::duration<double, std::nano>(e2 - s2).count() / N;

    out << N << "," << mutation_rate << "," << ecs_ns << "," << crg_ns << "\n";
}

int main() {
    std::ofstream out("data/crg_benchmark_final.csv");
    if (!out.is_open()) {
        std::cerr << "Error: Could not open data/crg_benchmark_final.csv\n";
        return 1;
    }

    out << "N,Rate,ECS_ns,CRG_ns\n";
    float rates[] = {0.01f, 0.10f};
    for (float rate : rates) {
        for (int n : {65536, 1048576}) RunTest(n, rate, out);
    }

    std::cout << "Success: Results saved in data/crg_benchmark_final.csv" << std::endl;
    return 0;
}