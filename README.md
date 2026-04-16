# CapabilityReconstructionGraph (CRG)

**Author & Architect:** Cyril Tissier

*Disclaimer: This repository is a purely educational exploration of a capability-based design pattern. It contains abstracted, simplified examples and does not contain or reflect any proprietary production code.*

A modern C++ architectural pattern for composing behavior at runtime through emergent N-Dimensional capability graphs. 

CRG eliminates the need for rigid inheritance hierarchies, ECS layouts, or centralized registries (`std::unordered_map`). Instead, capabilities emerge deterministically from the intersection of an N-Dimensional space: Identity, Behavior, and Contextual Axes (Time, Space, Authority, etc.).

## 🚀 Core Insight

A capability system does not need to be explicitly stored. It can be dynamically reconstructed.

In most architectures, adding a feature requires touching a central manager. In CRG, **adding a feature is a purely additive operation**: you add a file, and the topology of the system expands automatically. The "Manager" doesn't manage; it observes a projection of independently evolving runtime spaces.

## 📖 The 9-Stage Progression

This repository is structured as a progressive learning journey, taking the architecture from a naive baseline to a fully N-Dimensional Hypergraph.

* **Stage 0 — Baseline:** The rigid, tightly-coupled starting point.
* **Stage 1 — Decentralized Discovery:** Intrusive linked lists form a runtime graph purely through object lifecycles. Zero-boilerplate plugin system.
* **Stage 2 — Identity Space:** Stable runtime identities (Type-Erased) are introduced, orthogonal to structure.
* **Stage 3 — Identity-Based Resolution:** Traversal-based lookup without hash maps.
* **Stage 4 — External Behavior Definitions:** Behaviors are decoupled and defined in a separate global space.
* **Stage 5 — Projection (Visibility):** Compile-time subset filtering acts as a visibility lens over the behavior space.
* **Stage 6 — Fusion (Deterministic Selection):** Identity, behavior, and projection fuse to reconstruct a flat 2D capability.
* **Stage 7 — Contextual Lifecycle (The False Peak):** Attempting to bind behavior to RAII scopes (A lesson in why execution control is not domain state).
* **Stage 8 — The Temporal Axis (3D Space):** Introducing Logical Epochs. The matrix evaluates a 3D coordinate without structural mutation.
* **Stage 9 — The N-Dimensional Context Space:** The engine scales infinitely. Variadic template matrices evaluate exact contextual intersections (Time, Biomes, Authority) to project a hyper-contextual capability hypergraph.

## ⚡ Architecture Ergonomics

CRG is designed to be highly scalable while remaining ergonomic for the developer:

* **Decentralized Topology:** Using intrusive linking, nodes auto-register across binary boundaries (DLLs/Plugins). No central `std::vector<Plugin*>` or `Register()` calls are required.
* **Ephemeral Transport:** The Identity Space (`TypeErasedShell`) acts purely as a transient stack handle, ensuring allocation-free transport of concrete data to behavior definitions.
* **Auto-Registration:** The graph builds itself statically. Variadic inheritance allows an entire domain of N-Dimensional behavior nodes to auto-instantiate with a single static declaration.

### Unified N-Dimensional API

One elegant line queries the exact hyper-coordinate (Time, Space, Role) and resolves the behavior:

Call<IExecuteAction>(entity, Epoch::Combat, Biome::Desert, Role::Server);

## 🎤 Accompanying Material

* See `paper.md` for the full architectural white paper.
* See `canon.md` for the core structural model and principles.
* See `cppcon_pitch.txt` for the underlying presentation structure and philosophy.
