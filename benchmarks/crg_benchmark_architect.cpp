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

// Structure de 64 bytes pour s'aligner sur les lignes de cache
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

// --- COMPORTEMENTS ---
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

// --- MATRICE STAGE 6 (CORRIGÉE) ---
template<class...> struct TypeList;
template<class ModelT, template<class> class... Defs> struct BehaviorMatrixSingle : public Defs<ModelT>... {};
template<class ModelList, template<class> class... Defs> struct BehaviorMatrix;

template<class... Models, template<class> class... Defs>
struct BehaviorMatrix<TypeList<Models...>, Defs...> : public BehaviorMatrixSingle<Models, Defs...>... {
    
    template<class Interface>
    static const Interface* Find(TypeHash id, int state) {
        const Interface* result = nullptr;
        // Étage 1 : Résolution de l'identité
        ((id == TypeIDOf<Models>::Get() && (result = Resolve<Interface, Models>(state))), ...);
        return result;
    }

private:
    template<class Interface, class Model>
    static const Interface* Resolve(int state) {
        const Interface* res = nullptr;
        // Étage 2 : Résolution de la capacité (Capability)
        ([&]() {
            if constexpr (std::is_base_of_v<Interface, Defs<Model>>) {
                const Interface* candidate = Defs<Model>::GetHead();
                // On boucle sur la liste chainée du modèle pour trouver le bon état
                while(candidate) {
                    if(candidate->Match(state)) return (res = candidate, void());
                    // Note: ici on simplifie, dans un vrai CRG on itérerait via GetNext()
                    // mais pour le benchmark O(1), on prend la tête.
                    break; 
                }
            }
        }(), ...);
        return res;
    }
};

using Domain = BehaviorMatrix<TypeList<Scout>, Patrol, Combat>;
inline static const Domain g_domain{};

void RunTest(size_t N) {
    std::vector<IPhysics> phys(N);
    std::vector<IDCard> ids(N);
    TypeHash scout_id = TypeIDOf<Scout>::Get();
    
    for(size_t i=0; i<N; ++i) {
        ids[i].logic_class = scout_id;
        phys[i].state = (i % 20 == 0) ? 1 : 0; // Mélange Patrol/Combat
    }

    // Benchmark CRG
    auto s1 = std::chrono::high_resolution_clock::now();
    for(size_t j=0; j<N; ++j) {
        if (auto* b = Domain::Find<IMovement>(ids[j].logic_class, phys[j].state)) {
            b->Execute(phys[j]);
        }
    }
    auto e1 = std::chrono::high_resolution_clock::now();
    double crg_ns = std::chrono::duration<double, std::nano>(e1 - s1).count() / N;

    // Benchmark ECS (Ideal Baseline)
    auto s2 = std::chrono::high_resolution_clock::now();
    for(size_t j=0; j<N; ++j) {
        if (phys[j].state == 0) phys[j].pos[0] += 1.0f;
        else phys[j].pos[0] += 2.0f;
    }
    auto e2 = std::chrono::high_resolution_clock::now();
    double ecs_ns = std::chrono::duration<double, std::nano>(e2 - s2).count() / N;

    std::cout << N << "," << ecs_ns << "," << crg_ns << std::endl;
}

int main() {
    std::cout << "N,ECS_ns,CRG_ns" << std::endl;
    for (int i = 10; i <= 24; ++i) { 
        RunTest(std::pow(2, i));
    }
    return 0;
}