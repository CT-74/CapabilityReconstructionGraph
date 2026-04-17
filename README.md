# Capability Reconstruction Graph (CRG)

**Author & Architect:** Cyril Tissier

**Architectural & Legal Notice:**
The "Capability Reconstruction Graph" (CRG) architectural pattern and its underlying conceptual model are the independent intellectual property of the Author. The C++ code examples provided in this repository are functional, clean-room implementations created specifically for educational demonstration and public presentation. While they successfully compile and illustrate the core mechanics, they are intentionally simplified and unoptimized. They are entirely independent of, and do not contain, reflect, or leak any highly optimized proprietary production code, algorithms, or assets developed by the Author in a professional corporate capacity.

---

## Overview

The CRG is a structural model for composing external behavior in C++ systems where functionality is not embedded in objects, but is **auto-discovered** and reconstructed from the interaction of independent runtime-defined spaces.

Instead of relying on classical inheritance hierarchies, ECS architectures, or centralized registries, a capability system emerges as an **N-Dimensional Hypergraph**.

### The Core Paradigm: From Mutation to Observation

Traditional architectures manage state changes through **Mutation** (modifying data in memory). The CRG shifts this to **Contextual Observation**. 

Identity is no longer a container of data, but a set of coordinates in a phase space defined by **Variadic Axes** (e.g., Time, Environment, Authority). This allows for a strictly non-intrusive design where the "graph" is an emergent projection rather than a stored data structure.

---

## The 10 Stages of Evolution (0 to 9)

This repository contains pedagogical demonstration code that progressively builds the CRG model:

* **Stages 0 to 3: The Foundations (1D - Structural Axis)**
  Escaping the centralized "God Controller" by building zero-allocation transports (SBO) and establishing emergent structures through intrusive linking.
* **Stages 4 to 6: Emergent Composition (2D - The Matrix)**
  Decoupling behavior and identity to reconstruct a capability matrix without any centralized maps or registries. Auto-discovery of behaviors.
* **Stage 7: The Contextual Lifecycle Trap (The Wall)**
  Exploring why embedding behavior lifecycles within objects (RAII) or dynamically mutating the 2D matrix to manage local state is a fundamental thread-safety fallacy. 
* **Stage 8: The Pivot (3D - The Temporal Axis)**
  Introducing the 3rd Dimension (Time). We demonstrate how changing a coordinate on a temporal axis replaces the need for state mutation. State transitions become observations.
* **Stage 9: The N-Dimensional Hypergraph (N-D - Variadic Axes)**
  Generalizing the model to an open set of **Variadic Axes**. Capabilities are reconstructed as emergent projections at the intersection of arbitrary dimensional types (Biome, Security, etc.).

---

## Key Architectural Concepts

### Emergent Auto-Discovery
Structure and behavior "find each other" at runtime through traversal-based resolution. The system remains non-intrusive: you can layer CRG over existing codebases without refactoring core hierarchies.

### N-Dimensional Hypergraph (Variadic Axes)
Identity is a coordinate $(d_1, d_2, ..., d_n)$ where dimensions are defined by C++ types. 
Example: `crg::axis<Time>`, `crg::axis<Environment>`, or any custom domain dimension.

### Zero-Mutation State Transitions
The graph topology remains immuable. "State changes" are merely shifts in the observation coordinates, providing native thread-safety by design.

---

## System Properties

* **Zero-Allocation:** All resolution paths leverage **SBO** to avoid heap allocations on the hot path.
* **No Centralized Registry:** No maps, no registries, no global locks.
* **Massively Concurrent:** Optimized for multi-threaded observation through immutable topologies.

---

*This repository serves as the companion code to the CppCon presentation: "The Capability Reconstruction Graph: Emergent Behavior Composition via N-Dimensional Hypergraphs in C++".*
