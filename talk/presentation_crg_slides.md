# Présentation : Capability Reconstruction Graph (CRG)
## Auteur : Cyril Tissier

---

## Slide 1 : Titre & Hook
- **Titre :** Capability Reconstruction Graph (CRG)
- **Sous-titre :** L'émergence au service de la performance : Composer le comportement en C++ sans mutation.
- **Visuel suggéré :** Un graphe de points reliés par des rayons de lumière (évocation de projection).

---

## Slide 2 : Architectural & Legal Firewall
- **Titre :** Firewall Architectural & Légal
- **Contenu :**
    - Distinction stricte entre le **Modèle Conceptuel** (Propriété Intellectuelle de l'Auteur) et tout actif corporate.
    - Implémentation "Clean-room" créée spécifiquement pour cette présentation.
    - Code pédagogique, simplifié et indépendant de tout moteur propriétaire.

---

## Slide 3 : Le Problème - Le Statu Quo
- **Titre :** Pourquoi nos architectures souffrent ?
- **Points clés :**
    - **OOP Classique :** Hiérarchies rigides, couplage fort Identity-Behavior.
    - **ECS :** Bottlenecks centralisés, gestion complexe de l'aspect non-intrusif.
    - **Registries Globaux :** Singletons, Locks, et enfer du multithreading.
- **Hook :** "On code des objets, alors qu'on devrait coder des relations."

---

## Slide 4 : La Vision
- **Titre :** Une Nouvelle Perspective
- **Contenu :**
    - L'Identité n'est pas un conteneur de données.
    - L'Identité est une **coordonnée** dans un espace de phases.
    - Le comportement n'est pas possédé, il est **reconstruit** par observation.

---

## Slide 5 : Stages 0-3 - Les Fondations (1D)
- **Titre :** Échapper au "God Controller"
- **Contenu :**
    - **Self-registration :** Liaison intrusive pour créer la structure.
    - **SBO (Small Buffer Optimization) :** Transports à coût zéro, sans allocation tas.
    - **Axe 1 :** La Structure (Le graphe brut).

---

## Slide 6 : Stages 4-6 - La Matrice d'Émergence (2D)
- **Titre :** Découplage Identity x Behavior
- **Contenu :**
    - Séparation stricte : Qui (Identity) vs Quoi (Behavior).
    - **Auto-discovery :** Le système "trouve" le comportement par traversée.
    - **Résultat :** Une matrice de capacités fonctionnelle mais statique.

---

## Slide 7 : DÉMO LIVE 1 - L'émergence simple
- **Objectif :** Montrer un objet sans méthode `update()` ni membre `capability`, qui gagne un comportement par simple présence dans le graphe.
- **Code :** `resolver.resolve<Physics>(entity)`

---

## Slide 8 : Stage 7 - Le Mur : Contextual Lifecycle
- **Titre :** Le Piège du Cycle de Vie (The Trap)
- **Contenu :**
    - **Le Problème :** Comment gérer le changement (ex: Biome A vers Biome B) ?
    - **L'erreur classique :** Utiliser RAII ou muter le graphe localement.
    - **Conséquence :** Race conditions, instabilité des pointeurs, complexité O(N!).

---

## Slide 9 : Pourquoi RAII échoue ici ?
- **Titre :** RAII n'est pas la solution universelle
- **Points techniques :**
    - RAII lie la sémantique à la portée (scope) locale.
    - Dans un système distribué ou massivement multithreadé, la mutation "au fil de l'eau" casse la cohérence des vues.

---

## Slide 10 : LE PIVOT - Mutation vs Observation
- **Titre :** Changer de Paradigme
- **Concept :** - **Mutation (Ancien) :** Je modifie l'objet pour changer son état.
    - **Observation (Nouveau) :** Je change ma coordonnée de lecture.
- **Analogie :** On ne change pas le film sur la pellicule, on déplace le projecteur.

---

## Slide 11 : Stage 8 - La 3ème Dimension : Le Temps
- **Titre :** L'Axe Temporel (3D)
- **Contenu :**
    - Introduction d'un axe orthogonal : `Time`.
    - L'état de l'entité est une fonction de `(Structure, Behavior, Time)`.
    - **Résultat :** Plus aucune mutation mémoire pour changer d'état.

---

## Slide 12 : Stage 9 - L'Hypergraphe N-D
- **Titre :** Variadic Axes : Vers l'Infini
- **Contenu :**
    - Généralisation : Tout type C++ peut être un axe.
    - `crg::axis<Environment>`, `crg::axis<Authority>`, `crg::axis<Security>`.
    - La capacité est une projection au croisement de N axes.

---

## Slide 13 : Résolution Variadique (Code)
- **Titre :** La puissance des Templates
- **Code Snippet :**
    ```cpp
    template <typename... Axes>
    auto capability = resolver.at(identity, axes...);
    ```
- **Note :** Résolution déterministe et compilée.

---

## Slide 14 : DÉMO LIVE 2 - Projection Multidimensionnelle
- **Objectif :** Changer d'environnement (Biome) ou d'autorité sans toucher à un seul octet de l'entité.
- **Observation :** La capacité "apparaît" et "disparaît" selon les coordonnées.

---

## Slide 15 : Propriétés Système
- **Titre :** Performance & Robustesse
- **Points clés :**
    - **Zéro Allocation :** SBO partout.
    - **Thread-Safe par design :** Topologie immuable.
    - **Non-intrusif :** Utilisable sur du legacy sans refactoring.

---

## Slide 16 : Cas d'usage concrets
- **Titre :** Où utiliser le CRG ?
- **Domaines :**
    - **GameDev :** IA systémique, streaming de mondes.
    - **Finance :** Moteurs de règles à haute fréquence.
    - **Systèmes Distribués :** Cohérence contextuelle sans synchronisation lourde.

---

## Slide 17 : Conclusion
- **Message :** L'architecture est une projection. 
- **Synthèse :** Découplage total + Multidimensionnel = Flexibilité infinie.

---

## Slide 18 : Q&A
- **Titre :** Questions & Réponses
- **Contact :** Cyril Tissier
- **Lien Repo :** https://lnkd.in/eFFK2etA
