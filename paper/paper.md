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

A capability system does not need to be explicitly stored.

Instead, it emerges from the interaction of three independent spaces:

- a structural space (runtime-linked graph)
- an identity space (runtime type/model identity)
- a behavior space (external definitions)

No explicit capability graph is constructed or stored.

---

## 4. Progressive Construction

The system is introduced through five stages.

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

### Stage 5 — Composition (CRG-v1 Conceptual Extension)

The system now consists of three independent spaces:

- structural graph (LinkedNode)
- identity space (TypeErasedShell / ModelList)
- behavior space (DefinitionList)

From the interaction of these spaces, a runtime capability matrix emerges.

This matrix is not stored.

It is observed at runtime through traversal-based resolution.

---

## 5. System Properties

The resulting model exhibits:

- no centralized registry
- no explicit capability graph structure
- strict separation of structure, identity, and behavior
- multiple behavior definitions per identity space (CRG-v1 capability)
- deterministic resolution via traversal
- emergent many-to-many binding across independent spaces

---

## 6. Interpretation

What appears as a "capability graph" is not stored.

It emerges from the interaction of:

- structural traversal (LinkedNode graph)
- identity evaluation (TypeErasedShell / Model space)
- behavior resolution (external definition space)

The graph is an observed runtime projection, not a data structure.

---

## 7. Application Domains

- plugin architectures without registries
- gameplay systems with external behavior binding
- runtime extensibility systems
- debugging and instrumentation layers
- compositional engine architectures

---

## END