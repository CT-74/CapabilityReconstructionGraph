# Capability Reconstruction Graph (CRG)

**Author & Architect:** Cyril Tissier

**Architectural & Legal Notice:**
The "Capability Reconstruction Graph" (CRG) architectural pattern and its underlying conceptual model are the independent intellectual property of the Author. The C++ code examples provided in this repository are functional, clean-room implementations created specifically for educational demonstration and public presentation. While they successfully compile and illustrate the core mechanics, they are intentionally simplified and unoptimized. They are entirely independent of, and do not contain, reflect, or leak any highly optimized proprietary production code, algorithms, or assets developed by the Author in a professional corporate capacity.

---

## Overview

The CRG is a structural model for composing external behavior in C++ systems where functionality is not embedded in objects, but is **auto-discovered** and reconstructed from the interaction of independent runtime-defined spaces.

While classical approaches rely heavily on inheritance hierarchies or centralized registries, the Capability Reconstruction Graph acts as a symbiotic, context-oriented layer that perfectly complements Data-Oriented Design (like ECS). A capability system emerges as an **N-Dimensional Hypergraph**.

### The Core Paradigm: From Mutation to Observation

Traditional architectures manage state changes through **Mutation** (modifying data in memory). The CRG shifts this to **Contextual Observation**. 

Identity is no longer a container of data, but a set of coordinates in a phase space defined by **Variadic Axes** (e.g., Time, Environment, Authority). This allows for a strictly non-intrusive design where the "graph" is an emergent projection rather than a stored data structure.

---

## The 10 Stages of Evolution

This repository contains pedagogical demonstration code that progressively builds the CRG model, navigating through a critical architectural "False Peak":

* **Stage 1 — Structural Primitive:** A runtime-linked graph forming through intrusive self-registration.
* **Stage 2 — Identity Space:** Adding stable runtime identity, independent of structure.
* **Stage 3 — Identity-Based Resolution:** A traversal-based lookup layer (avoiding registries).
* **Stage 4 — External Behavior Definitions:** Decoupling behavior semantics from identity.
* **Stage 5 — Composition:** Creating a runtime capability matrix through interaction.
* **Stage 6 — Deterministic Reconstruction (Fusion):** Achieving a working system with decoupled behavior and structure via an `O(1)` resolution matrix.
* **Stage 7 — The Trap (False Peak):** Dealing with the "Contextual Lifecycle" — Why RAII and local mutations fail to scale and trigger concurrency hazards in graph-based behaviors.
* **Stage 8 — The Pivot:** From Mutation to Observation — Introducing the Temporal Axis as a first dimension to replace state changes with contextual resolution.
* **Stage 9 — N-Dimensional Expansion:** Implementing the Hypergraph via Variadic Axes and Zero-Cost Transports (SBO).
* **Stage 10 — The Symbiosis (Emergent Projections):** Final synthesis showing how the CRG integrates flawlessly inside an ECS loop—separating pure Data Localisation from N-Dimensional Contextual Projection.

---

## FAQ: How does this fit with ECS (Entity Component System)?

**They are perfectly complementary.** The CRG does not replace an ECS; it solves the **Archetype Fragmentation** problem inherent to pure ECS architectures.

* **The ECS (The Body):** Excels at Data-Oriented Design. It iterates through contiguous memory at blinding speed.
* **The CRG (The Mind):** Excels at Context-Oriented Design. It projects N-Dimensional behavior onto that data.

In a pure ECS, changing an entity's behavior (e.g., from "Day Mode" to "Night Stealth Mode") often requires adding or removing *Tag Components*. This forces the ECS to move the entity to a new archetype array in memory, causing cache misses and requiring structural locks in multithreading. 

With the CRG, the ECS iterates purely on the data. Inside the loop, the CRG resolves the behavior based on contextual axes (like Time or Environment) via a zero-allocation SBO handle, **without ever mutating the entity's structure or archetype in the ECS.**

---

## System Properties

* **Zero-Allocation:** All resolution paths leverage **SBO** to avoid heap allocations on the hot path.
* **No Centralized Registry:** No maps, no global registries, no locks.
* **Massively Concurrent:** Optimized for multi-threaded observation through immutable topologies.
* **ECS Symbiosis:** Natively designed to plug into existing DOD loops without competing for data layout.

---

*This repository serves as the companion code to the CppCon presentation: "The Capability Reconstruction Graph: Emergent Behavior Composition via N-Dimensional Hypergraphs in C++".*
