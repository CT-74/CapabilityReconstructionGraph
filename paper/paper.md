# A Runtime Capability Graph for External Behavior Composition in C++

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

while avoiding commitment to any specific implementation mechanism.

---

## 3. Core Insight

A capability system does not need to be explicitly stored, nor is it limited to a flat mapping of type-to-behavior.

Instead, it emerges from the intersection of an N-Dimensional space:

- an identity space (runtime type/model identity)
- a behavior space (external definitions)
- contextual axes (temporal epochs, spatial biomes, network authority, etc.)

No explicit capability graph is constructed or stored. The capability graph is the observed runtime projection of this N-Dimensional intersection.

---

## 4. Progressive Construction

The system is introduced through nine stages.

---

### Stage 1 — Structural Primitive (CRG-v0)

A runtime-linked graph is formed through object lifetime side effects.

This corresponds to the LinkedNode primitive:

- intrusive linking at construction time
- no identity meaning
- no behavior semantics
- purely structural membership

The graph exists only through traversal.

---

### Stage 2 — Identity Space

A stable runtime identity is introduced.

This corresponds to TypeErasedShell (or equivalent model identity carrier).

At this stage:
- identity exists independently of structure
- identity is not yet mapped to behavior
- no routing or lookup system is introduced yet

Structure and identity remain orthogonal.

---

### Stage 3 — Identity-Based Resolution (Routing Layer)

A resolution mechanism is introduced over the identity space.

This corresponds to the BehaviorMatrix layer:

- identity is used as a selection key
- resolution determines applicable interface
- traversal-based or lightweight scanning is used
- no explicit mapping structure is stored

This layer connects identity to behavior interfaces.

---

### Stage 4 — External Behavior Definitions

Behavior is fully decoupled from identity nodes.

A separate definition space is introduced:

- multiple behavior definitions may exist per identity space
- behavior is defined externally from identity carriers
- definitions are independently discoverable via traversal

This creates a behavior space independent from identity space.

---

### Stage 5 — Projection (Visibility)

A compile-time constraint layer is introduced.

This layer acts as a visibility lens:

- restricts which nodes are mathematically observable
- does not alter the underlying graph structure
- uses compile-time subset filtering over the behavior space

Only a subset of the global behavior space becomes observable.

---

### Stage 6 — Fusion (Deterministic Selection)

Deterministic runtime resolution binds identity to behavior.

This step finalizes the flat capability model:

- traversal is combined with projection constraints
- a concrete capability is deterministically reconstructed
- the system acts as a 2D capability map (Identity × Behavior)

One capability is selected from the constrained system.

---

### Stage 7 — Contextual Lifecycle (The False Peak)

An attempt to control capability over time using C++ lifetime semantics (RAII).

This introduces dynamic behavior scoping:

- behavior existence is bound to the call stack
- execution control is conflated with domain state
- structural mutation of global pointers occurs (thread-unsafe)

This approach proves insufficient to model a true domain state progression.

---

### Stage 8 — The Temporal Axis (3D Space)

A true domain axis is introduced to model time.

Identity is evaluated against a temporal coordinate (Logical Epochs):

- identities carry a temporal state independent of the call stack
- behaviors target specific coordinates in time
- the system transitions to a 3D phase space (Identity × Behavior × Epoch)

The reconstructed capabilities shift dynamically as the temporal coordinate evolves, without structural mutation.

---

### Stage 9 — The N-Dimensional Context Space (Hypergraph)

The resolution matrix is abstracted to support variadic, orthogonal coordinate axes.

The system scales infinitely via Non-Type Template Parameters (NTTPs):

- dimensions like Space, Authority, and Factions are introduced
- behavior definitions target exact contextual intersections
- the matrix acts as a generic N-dimensional solver

An infinitely complex, hyper-contextual capability hypergraph emerges at runtime.

---

## 5. System Properties

The resulting model exhibits:

- no centralized registry (`std::unordered_map` or similar)
- no explicit capability graph structure
- strict separation of structure, identity, and behavior
- N-Dimensional determinism
- O(1) structural cost (topology changes require no allocations)
- emergent many-to-many binding across independent spaces

---

## 6. Interpretation

What appears as a "capability graph" is not stored.

It emerges from the interaction of:

- structural traversal (LinkedNode graph)
- identity evaluation (TypeErasedShell / Model space)
- N-Dimensional contextual constraints (Epochs, Biomes, Roles)

The graph is an observed runtime projection, not a data structure.

---

## 7. Practical Implementation: AAA Ergonomics & Performance

A production-ready implementation requires strict performance and usability guarantees.

**Ephemeral Identity Transport (SBO)**
The identity carrier (`TypeErasedShell`) is never stored. It exists purely on the stack as an ephemeral handle for the duration of a capability query. Utilizing Small Buffer Optimization (SBO), the type-erased model is constructed directly within a local stack buffer, ensuring the transport layer remains entirely allocation-free and cache-friendly.

**Zero-Friction API (NTTP Deduction)**
To make the hypergraph usable without boilerplate, the API provides a pure convenience layer leveraging C++17 Non-Type Template Parameters. Instead of querying matrices by hand, developers pass a member function pointer directly to the ephemeral entity handle:

```cpp
entity.TryCall<&IAttack::Execute>(Epoch::Combat, Biome::Desert);
```

The compiler automatically deduces the required interface. The behavior definition then simply extracts the underlying concrete data using `.GetAs<T>()`. If the behavior exists at this exact N-Dimensional intersection, it executes. If not, it safely returns an empty optional.

---

## 8. Application Domains

- plugin architectures without registries
- hyper-contextual gameplay logic (state, biomes, authority)
- runtime extensibility systems
- debugging and instrumentation layers
- compositional engine architectures

---

## END
