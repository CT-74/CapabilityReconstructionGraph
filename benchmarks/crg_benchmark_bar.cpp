#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <type_traits>
#include <fstream>

// --- INFRASTRUCTURE ---
using TypeHash = std::size_t;
template<class T> struct TypeIDOf { static TypeHash Get() { return typeid(T).hash_code(); } };

template<class Derived, class Interface>
class LinkedNode {
public:
    LinkedNode() { s_head = static_cast<const Interface*>(static_cast<const Derived*>(this)); }
    static const Interface* GetHead() { return s_head; }
private:
    inline static const Interface* s_head = nullptr;
};

// 64-byte structure for cache line alignment[cite: 6]
struct IPhysics { 
    int entity_id;
    float pos[3]; 
    float vel[3]; 
    int state; 
    char pad[44]; 
};

struct IDCard { TypeHash logic_class; };

struct IMovement { 
    virtual bool Match(int s) const = 0;
    virtual void Execute(IPhysics& p) const = 0; 
};

// --- BEHAVIORS ---
struct Scout {};

template<class T> 
struct Patrol : IMovement, LinkedNode<Patrol<T>, IMovement> {
    bool Match(int s) const override { return s == 0; }
    void Execute(IPhysics& p) const override { p.pos[0] += 1.0f; }
};

template<class T> 
struct Combat : IMovement, LinkedNode<Combat<T>, IMovement> {
    bool Match(int s) const override { return s == 1; }
    void Execute(IPhysics& p) const override { p.pos[0] += 2.0f; }
};

// --- BEHAVIOR MATRIX (Stage 6)[cite: 6] ---
template<class...> struct TypeList;
template<class ModelT, template<class> class... Defs> struct BehaviorMatrixSingle : public Defs<ModelT>... {};
template<class ModelList, template<class> class... Defs> struct BehaviorMatrix;

template<class... Models, template<class> class... Defs>
struct BehaviorMatrix<TypeList<Models...>, Defs...> : public BehaviorMatrixSingle<Models, Defs...>... {
    
    template<class Interface>
    static const Interface* Find(TypeHash id, int state) {
        const Interface* result = nullptr;
        ((id == TypeIDOf<Models>::Get() && (result = Resolve<Interface, Models>(state))), ...);
        return result;
    }

private:
    template<class Interface, class Model>
    static const Interface* Resolve(int state) {
        const Interface* res = nullptr;
        ([&]() {
            if constexpr (std::is_base_of_v<Interface, Defs<Model>>) {
                const Interface* candidate = Defs<Model>::GetHead();
                if(candidate && candidate->Match(state)) res = candidate;
            }
        }(), ...);
        return res;
    }
};

using Domain = BehaviorMatrix<TypeList<Scout>, Patrol, Combat>;
inline static const Domain g_domain{};

int main(int argc, char* argv[]) {
    size_t N = (argc > 1) ? std::stoull(argv[1]) : 1000000;
    
    std::vector<IPhysics> phys(N);
    std::vector<IDCard> ids(N);
    std::vector<int> sparse_set(N);
    TypeHash scout_id = TypeIDOf<Scout>::Get();

    for(size_t i=0; i<N; ++i) {
        phys[i].entity_id = i;
        phys[i].state = 0;
        ids[i].logic_class = scout_id;
        sparse_set[i] = i;
    }

    // --- TEST 1: CRG (O(1) Value Mutation)[cite: 6] ---
    auto s1 = std::chrono::high_resolution_clock::now();
    for(size_t i=0; i<N; ++i) {
        if (auto* b = Domain::Find<IMovement>(ids[i].logic_class, phys[i].state)) {
            b->Execute(phys[i]);
        }
        if (i % 20 == 0) phys[i].state = 1; // 5% mutation rate
    }
    auto e1 = std::chrono::high_resolution_clock::now();
    double crg_time = std::chrono::duration<double, std::milli>(e1 - s1).count();

    // --- TEST 2: ECS (Structural Mutation / Swap & Pop)[cite: 6] ---
    for(auto& p : phys) p.state = 0; // Reset
    std::vector<IPhysics> combat_archetype;
    combat_archetype.reserve(N/20);
    size_t current_size = N;

    auto s2 = std::chrono::high_resolution_clock::now();
    for(size_t i=0; i<current_size; ++i) {
        phys[i].pos[0] += (phys[i].state == 0) ? 1.0f : 2.0f;

        if (i % 20 == 0) { // Structural Mutation (Cache Poisoning)[cite: 6, 7]
            phys[i].state = 1;
            combat_archetype.push_back(phys[i]);
            phys[i] = phys[current_size - 1]; // Swap
            sparse_set[phys[i].entity_id] = (int)i; // Update Sparse Set (Random Access)
            current_size--;
            i--;
        }
    }
    auto e2 = std::chrono::high_resolution_clock::now();
    double ecs_time = std::chrono::duration<double, std::milli>(e2 - s2).count();

    // Export results to data/ subfolder
    std::ofstream out("data/crg_benchmark_bar.csv");
    if (out.is_open()) {
        out << "N,ECS_ms,CRG_ms\n";
        out << N << "," << ecs_time << "," << crg_time << "\n";
        std::cout << "Success: Data saved in data/crg_benchmark_bar.csv" << std::endl;
    }

    return 0;
}