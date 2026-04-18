CRG : README

# Capability Reconstruction Graph (CRG)

**Author & Architect:** Cyril Tissier

**Architectural & Legal Notice:**
The "Capability Reconstruction Graph" (CRG) architectural pattern and its underlying conceptual model are the independent intellectual property of the Author. The C++ code examples provided in this repository are functional, clean-room implementations created specifically for educational demonstration. They are intentionally simplified and unoptimized. They are entirely independent of, and do not contain, reflect, or leak any proprietary production code, algorithms, or assets developed by the Author in a professional capacity.

---

## Overview

The CRG is a structural model for composing external behavior in C++ systems where functionality is not embedded in objects, but is reconstructed from the interaction of independent runtime-defined spaces.

Instead of relying on inheritance hierarchies, static ECS layouts, or centralized registries, capabilities emerge through an **N-Dimensional Hypergraph** resolution.

### The Paradigm: From Mutation to Observation

Traditional architectures manage state changes through **Mutation** (modifying data in memory). The CRG shifts this to **Contextual Observation**. 

Identity is defined as a set of coordinates in a phase space governed by **Variadic Axes** (e.g., Time, Environment, Authority). Behavior is not "added" to an entity; it is "observed" when the entity's coordinates intersect with a capability layer.

---

## The 10-Stage Evolution

This repository provides a step-by-step progression of the CRG implementation:

* **Stage 1-4: Foundation** — Establishing emergent discovery and escaping the "God Controller" pattern.
* **Stage 5-7: The Lifecycle Wall** — Analysis of why RAII and standard object lifecycles fail in complex graph behaviors.
* **Stage 8: The Pivot to Observation** — Moving from 2D mutation to 3D observation using a Temporal Axis.
* **Stage 9: N-Dimensional Expansion** — Generalizing the model to Variadic Axes. Identity becomes a coordinate in an open phase space.
* **Stage 10: The Symbiosis (ECS)** — Final integration. Using the CRG to project behavior onto Data-Oriented structures, bypassing Archetype Fragmentation in ECS architectures.

---

## Technical Q&A & Design Decisions

### Q: How does this claim O(1) resolution if there is a loop in Resolve?
**A:** The O(1) refers to **Structural Complexity**. In traditional systems, finding a behavior scales with the total number of entities (N) or total registered types (T). In the CRG, resolution is local to the type's own BehaviorMatrix. The loop only iterates over the number of behaviors (K) defined for that specific type (usually < 5). Since K is a small constant independent of N or T, the resolution is effectively O(1) relative to the system's scale.

### Q: Why not just use a standard ECS?
**A:** The CRG is a **symbiotic layer**, not an ECS replacement. Standard ECS architectures suffer from **Archetype Fragmentation**: adding a "Tag Component" to change an entity's behavior forces the ECS to move data to a different memory chunk. The CRG solves this via **Contextual Observation**: behavior changes by shifting coordinates in the Hypergraph, leaving the underlying ECS data contiguous and untouched.

### Q: What happens if two behaviors match the same context?
**A:** The CRG uses **Intrusive Precedence**. The last behavior registered (or the one appearing first in the variadic list) wins. This ensures deterministic results without the overhead of complex priority scoring or weight arbitration systems.

### Q: Isn't the Match() function just a hidden if-else chain?
**A:** Yes, the CPU must still perform a branch. However, the CRG replaces **Structural Mutation** (expensive memory shuffling) with **Simple Comparison** (cheap enum/ID checks). The branch is moved out of the hot-path business logic and into the contextual resolution phase.

### Q: How does this handle multi-threading?
**A:** The CRG topology is **Immutable at Runtime**. Once the matrix is instantiated, no nodes are added or removed. "State changes" are simply different coordinates provided to the Resolve function. Since no shared state is mutated during observation, the system is natively thread-safe for concurrent reads.

### Q: Why use a SBO (Small Buffer Optimization) Handle?
**A:** To ensure **Zero-Allocation**. By keeping the component data on the stack during the update, we avoid heap allocations on the hot path while maintaining strict decoupling between the raw data and the behavioral interface.

---

*This repository serves as the companion code to the CppCon presentation: "The Capability Reconstruction Graph: Emergent Behavior Composition via N-Dimensional Hypergraphs in C++".*
