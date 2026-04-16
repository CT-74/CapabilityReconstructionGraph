# A Runtime Capability Graph for External Behavior Composition in C++

**Author & Architect:** Cyril Tissier

*Disclaimer: This document defines an abstract architectural pattern. It is a conceptual model for educational purposes and does not represent proprietary implementation details.*

---

## 1. Problem Statement

Modern C++ systems struggle with extensibility when behavior is tightly coupled to type hierarchies, ECS layouts, or centralized registries.

These approaches typically introduce:
- structural coupling between identity and behavior
- centralized ownership of extension points
- fragile cross-module extensibility
- refactor requirements for system evolution

---

## 2. Design Goal

Enable a system where:

- structure, identity, and behavior are decoupled
- behavior can be defined externally
- composition emerges without central orchestration
- runtime structure can be discovered, not predeclared

---

## 3. Core Insight

A capability system does not need to be explicitly stored, nor is it limited to a flat mapping of type-to-behavior.

Instead, it emerges from the intersection of an N-Dimensional space:

- an identity space (runtime type/model identity)
- a behavior space (external definitions)
- contextual axes (temporal epochs, spatial biomes, network authority, etc.)

No explicit capability graph is constructed or stored. The capability graph is the observed runtime projection of this N-Dimensional intersection.

---

## 4. The Power of Decentralized Discovery (Stage 1 logic)

At its most fundamental level (Stage 1), the CRG functions as a **decentralized discovery protocol**. 

By utilizing intrusive linked structures, the system bypasses the need for a centralized registry (`std::vector`, `std::map`). This provides a massive advantage for **modular architectures and plugin systems**:

- **Zero-Boilerplate Registration:** Extension modules (DLLs, Shared Objects) contribute to the global behavior set simply by existing. Upon being loaded, their static instances automatically inject themselves into the global intrusive topology.
- **Cross-Boundary Compatibility:** The discovery mechanism works across binary boundaries without a central coordinator. The "Core" module does not need to own or even know about the extensions; it simply traverses the available nodes.
- **Topological Discovery:** This level alone is sufficient for systems requiring simple enumeration of actions, tools, or observers, even before the full Identity/Behavior resolution of later stages is applied.

---

## 5. Progressive Construction

The system is introduced through nine stages.

---

### Stage 1 — Structural Primitive (CRG-v0)

A runtime-linked graph is formed through object lifetime side effects (intrusive linking). Structure is defined purely by traversal.

### Stage 2 — Identity Space

A stable runtime identity is introduced via a Type-Erased handle (Identity carrier). Structure and identity remain orthogonal.

### Stage 3 — Identity-Based Resolution (Routing Layer)

A resolution mechanism is introduced. It is not a map, but a runtime resolution based on scanning the discovery layer.

### Stage 4 — External Behavior Definitions

Behavior is fully decoupled from identity. Multiple definitions can exist for the same identity domain in a separate space.

### Stage 5 — Projection (Visibility)

A compile-time constraint layer acts as a visibility lens, restricting which nodes are mathematically observable.

### Stage 6 — Fusion (Deterministic Selection)

Deterministic runtime resolution binds identity to behavior under projection constraints. The system acts as a flat 2D capability map.

### Stage 7 — Contextual Lifecycle (The False Peak)

An attempt to control capability over time using RAII. This proves insufficient and thread-unsafe for true domain state progression.

### Stage 8 — The Temporal Axis (3D Space)

Introduction of **Logical Epochs**. Identity is evaluated against a temporal coordinate. The system transitions to a 3D phase space.

### Stage 9 — The N-Dimensional Context Space (Hypergraph)

The resolution matrix is abstracted to support variadic coordinate axes (Space, Authority, Factions). An infinitely complex, hyper-contextual capability hypergraph emerges at runtime.

---

## 6. System Properties

- **No Central Registry:** No `std::unordered_map` or centralized "Manager" objects.
- **Distributed Ownership:** Modules define their own behavior-identity-context triplets.
- **O(1) Structural Cost:** Topological changes are intrusive and require no heap allocations.
- **Emergent Projection:** The graph is a view, not a container.

---

## 7. Practical Implementation: AAA Ergonomics & Performance

**Ephemeral Identity Transport (SBO)**
The identity carrier (`TypeErasedShell`) exists purely on the stack as a transient handle. Using Small Buffer Optimization (SBO), it ensures allocation-free transport.

**Zero-Friction API (NTTP Deduction)**
Developers query the hypergraph through a unified `Call` interface:
```cpp
Call<IExecuteAction>(entity, Epoch::Combat, Biome::Desert, Role::Server);
```
The system automatically resolves the exact N-Dimensional coordinate and executes the behavior by extracting concrete data via `.GetAs<T>()`.

---

## 8. Application Domains

- Plugin architectures with zero-registry overhead.
- Hyper-contextual gameplay logic (Complex AI/Combat).
- Large-scale modular simulation architectures.
- Runtime instrumentation and debugging layers.

---

## END
