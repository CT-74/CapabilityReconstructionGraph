#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <type_traits>

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

struct IPhysics { 
    int entity_id;
    float pos[3]; 
    float vel[3]; 
    int state; 
    char pad[44]; 
};

// FONCTION DE CALCUL LOURD (Simule une vraie logique de jeu)
// On utilise 'volatile' ou on retourne une valeur pour éviter que le compilo n'efface tout
inline void HeavyLogic(IPhysics& p, float factor) {
    for(int i=0; i<10; ++i) { // 10 itérations de calcul mathématique
        p.pos[0] += std::sin(p.pos[1] * factor) * p.vel[0];
        p.pos[1] += std::cos(p.pos[0] * factor) * p.vel[1];
        p.pos[2] += std::sqrt(std::abs(p.pos[0])) * factor;
    }
}

struct IDCard { TypeHash logic_class; };
struct IMovement { 
    virtual bool Match(int s) const = 0;
    virtual void Execute(IPhysics& p) const = 0; 
};

struct Scout {};
template<class T> struct Patrol : IMovement, LinkedNode<Patrol<T>, IMovement> {
    bool Match(int s) const override { return s == 0; }
    void Execute(IPhysics& p) const override { HeavyLogic(p, 1.0f); }
};
template<class T> struct Combat : IMovement, LinkedNode<Combat<T>, IMovement> {
    bool Match(int s) const override { return s == 1; }
    void Execute(IPhysics& p) const override { HeavyLogic(p, 2.0f); }
};

// --- MATRICE STAGE 6 ---
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

void RunTest(size_t N, float mutation_rate) {
    std::vector<IPhysics> phys(N);
    std::vector<IDCard> ids(N);
    std::vector<int> sparse_set(N);
    TypeHash scout_id = TypeIDOf<Scout>::Get();
    int mod_trigger = (mutation_rate <= 0) ? 99999999 : (int)(1.0f / mutation_rate);

    for(size_t i=0; i<N; ++i) {
        phys[i].entity_id = i;
        phys[i].pos[0] = (float)i;
        phys[i].state = 0;
        ids[i].logic_class = scout_id;
        sparse_set[i] = i;
    }

    // --- Benchmark CRG ---
    auto s1 = std::chrono::high_resolution_clock::now();
    for(size_t j=0; j<N; ++j) {
        if (auto* b = Domain::Find<IMovement>(ids[j].logic_class, phys[j].state)) {
            b->Execute(phys[j]);
        }
        if (j % mod_trigger == 0) phys[j].state = (phys[j].state == 0) ? 1 : 0; 
    }
    auto e1 = std::chrono::high_resolution_clock::now();
    double crg_ns = std::chrono::duration<double, std::nano>(e1 - s1).count() / N;

    // --- Benchmark ECS ---
    std::vector<IPhysics> other_archetype; other_archetype.reserve(N);
    auto s2 = std::chrono::high_resolution_clock::now();
    for(size_t j=0; j<N; ++j) {
        // Logique métier
        if (phys[j].state == 0) HeavyLogic(phys[j], 1.0f);
        else HeavyLogic(phys[j], 2.0f);

        // Mutation structurelle (Le coût du mouvement)
        if (j % mod_trigger == 0) {
            phys[j].state = (phys[j].state == 0) ? 1 : 0;
            other_archetype.push_back(phys[j]);
            phys[j] = phys[N-1]; // Simulation Swap
            sparse_set[phys[j].entity_id] = j; 
        }
    }
    auto e2 = std::chrono::high_resolution_clock::now();
    double ecs_ns = std::chrono::duration<double, std::nano>(e2 - s2).count() / N;

    std::cout << N << "," << mutation_rate << "," << ecs_ns << "," << crg_ns << std::endl;
}

int main() {
    std::cout << "N,Rate,ECS_ns,CRG_ns" << std::endl;
    float rates[] = {0.01f, 0.05f, 0.10f};
    for (float rate : rates) {
        for (int i = 10; i <= 22; ++i) {
            RunTest(std::pow(2, i), rate);
        }
    }
    return 0;
}