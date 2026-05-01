#include <iostream>
#include <vector>
#include <chrono>

// 64-byte Structure (Standard CPU Cache Line size)[cite: 10]
struct IPhysics {
    float position[3] = {0.0f, 0.0f, 0.0f};
    float velocity[3] = {1.0f, 1.0f, 1.0f};
    int state = 0;
    char padding[48];
};

void BehaviorPatrol(IPhysics* p) { p->position[0] += p->velocity[0]; }
void BehaviorCombat(IPhysics* p) { p->position[0] += p->velocity[0] * 2.0f; }

int main(int argc, char* argv[]) {
    size_t entity_count = (argc > 1) ? std::stoull(argv[1]) : 100000;
    
    std::vector<IPhysics> data(entity_count);
    std::vector<IPhysics*> hot_path(entity_count);
    for(size_t i=0; i<entity_count; ++i) hot_path[i] = &data[i];

    // ==========================================
    // 1. BASELINE ECS (Theoretical max speed, hardcoded loop)
    // ==========================================
    auto start_ecs = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < entity_count; ++i) {
        // Compiler will likely inline this (maximum throughput)[cite: 10]
        BehaviorPatrol(&data[i]); 
    }
    auto end_ecs = std::chrono::high_resolution_clock::now();
    double ecs_time = std::chrono::duration<double, std::milli>(end_ecs - start_ecs).count();

    // ==========================================
    // 2. BASELINE CRG (O(1) Logic Injection)
    // ==========================================
    auto start_crg = std::chrono::high_resolution_clock::now();
    auto logic_a = BehaviorPatrol;
    auto logic_b = BehaviorCombat;
    for (size_t i = 0; i < entity_count; ++i) {
        // On-the-fly function pointer resolution[cite: 10]
        auto* logic = (data[i].state == 0) ? logic_a : logic_b;
        logic(hot_path[i]);
    }
    auto end_crg = std::chrono::high_resolution_clock::now();
    double crg_time = std::chrono::duration<double, std::milli>(end_crg - start_crg).count();

    // Bandwidth Calculation (Gi/s)[cite: 10]
    double bytes = (double)entity_count * sizeof(IPhysics);
    double ecs_bw = (bytes / (1024.0 * 1024.0 * 1024.0)) / (ecs_time / 1000.0);
    double crg_bw = (bytes / (1024.0 * 1024.0 * 1024.0)) / (crg_time / 1000.0);

    // CSV Output: Count, ECS_Time, CRG_Time, ECS_BW, CRG_BW[cite: 10]
    std::cout << entity_count << "," << ecs_time << "," << crg_time << "," << ecs_bw << "," << crg_bw << std::endl;
    return 0;
}