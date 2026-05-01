#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
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

// 64-byte structure to align with CPU cache lines[cite: 4, 5]
struct IPhysics { 
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

// --- BEHAVIOR MATRIX (Stage 6 Logic)[cite: 4] ---
template<class...> struct TypeList;
template<class ModelT, template<class> class... Defs> struct BehaviorMatrixSingle : public Defs<ModelT>... {};
template<class ModelList, template<class> class... Defs> struct BehaviorMatrix;

template<class... Models, template<class> class... Defs>
struct BehaviorMatrix<TypeList<Models...>, Defs...> : public BehaviorMatrixSingle<Models, Defs...>... {
    
    template<class Interface>
    static const Interface* Find(TypeHash id, int state) {
        const Interface* result = nullptr;
        // Step 1: Identity resolution
        ((id == TypeIDOf<Models>::Get() && (result = Resolve<Interface, Models>(state))), ...);
        return result;
    }

private:
    template<class Interface, class Model>
    static const Interface* Resolve(int state) {
        const Interface* res = nullptr;
        // Step 2: Capability resolution
        ([&]() {
            if constexpr (std::is_base_of_v<Interface, Defs<Model>>) {
                const Interface* candidate = Defs<Model>::GetHead();
                // Simple linked-list traversal for matching state[cite: 4]
                while(candidate) {
                    if(candidate->Match(state)) return (res = candidate, void());
                    break; 
                }
            }
        }(), ...);
        return res;
    }
};

using Domain = BehaviorMatrix<TypeList<Scout>, Patrol, Combat>;
inline static const Domain g_domain{};

void RunTest(size_t N, std::ofstream& out) {
    std::vector<IPhysics> phys(N);
    std::vector<IDCard> ids(N);
    TypeHash scout_id = TypeIDOf<Scout>::Get();
    
    for(size_t i=0; i<N; ++i) {
        ids[i].logic_class = scout_id;
        phys[i].state = (i % 20 == 0) ? 1 : 0; // Mixed Patrol/Combat behaviors[cite: 4]
    }

    // CRG Benchmark: O(1) Matrix resolution[cite: 4]
    auto s1 = std::chrono::high_resolution_clock::now();
    for(size_t j=0; j<N; ++j) {
        if (auto* b = Domain::Find<IMovement>(ids[j].logic_class, phys[j].state)) {
            b->Execute(phys[j]);
        }
    }
    auto e1 = std::chrono::high_resolution_clock::now();
    double crg_ns = std::chrono::duration<double, std::nano>(e1 - s1).count() / N;

    // ECS Benchmark: Idealized direct loop[cite: 4]
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
    // Generate output file in the data/ subfolder
    std::ofstream out("data/crg_benchmark_architect.csv");
    if (!out.is_open()) {
        std::cerr << "Error: Could not open data/crg_benchmark_architect.csv\n";
        return 1;
    }
    
    out << "N,ECS_ns,CRG_ns\n";
    for (int i = 10; i <= 24; ++i) { 
        RunTest(std::pow(2, i), out);
    }
    
    std::cout << "Done: Results saved in data/crg_benchmark_architect.csv" << std::endl;
    return 0;
}