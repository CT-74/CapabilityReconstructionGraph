# Capability Reconstruction Graph (CRG)

**Author & Architect:** Cyril Tissier

**Architectural & Legal Notice:**
The "Capability Reconstruction Graph" (CRG) architectural pattern and its underlying conceptual model are the independent intellectual property of the Author. The C++ code examples provided in this repository are functional, clean-room implementations created specifically for educational demonstration and public presentation. While they successfully compile and illustrate the core mechanics, they are intentionally simplified and unoptimized. They are entirely independent of, and do not contain, reflect, or leak any highly optimized proprietary production code, algorithms, or assets developed by the Author in a professional corporate capacity.

---

## Overview

The CRG is a structural model for composing external behavior in C++ systems where functionality is not embedded in objects, but is reconstructed from the interaction of independent runtime-defined spaces.

Instead of relying on classical inheritance hierarchies, ECS architectures, or centralized registries (the "God Controller" anti-pattern), a capability system emerges from:
1. **A Structural Graph:** Defined purely by traversal (no centralized containers).
2. **An Identity Space:** Stable runtime identity independent of structure and behavior.
3. **A Behavior Space:** Fully decoupled external behavior definitions.
4. **Contextual Coordinates:** N-Dimensional axes (e.g., Time, Environment, Authority).

**The Core Insight:** No explicit capability graph is ever constructed, stored, or mutated. Instead, what appears as a graph is *reconstructed* upon observation at the intersection of these independent dimensions.

---

## The 10-Stage Evolution

This repository contains the pedagogical demonstration code that progressively builds the CRG model through 10 evolutionary stages:

* **Stages 0 to 3:** Escaping the centralized "God Controller" by building zero-allocation transports (Small Buffer Optimization) and establishing emergent structures through intrusive linking.
* **Stages 4 to 6:** Decoupling behavior and identity to reconstruct a capability matrix without any centralized maps or registries.
* **Stage 7:** The Non-Global Lifecycle Trap (aka The RAII Trap) — Exploring why embedding behavior lifecycles within objects (or dynamically mutating the graph to manage local state) is a fundamental thread-safety fallacy. 
* **Stages 8 to 9 (The Hypergraph):** Moving from a 2D matrix to an N-dimensional phase space. We introduce variadic resolution across orthogonal axes (e.g., Epoch, Biome, Security). We demonstrate how "Context Observation" gracefully replaces "State Mutation".

---

## System Properties

* **Zero-Allocation:** Transports utilize SBO to avoid heap allocations on the hot path.
* **No Centralized Registry:** Structure emerges from object lifetime side-effects.
* **Orthogonal Contextual Resolution:** Deterministic resolution via variadic traversal.
* **Zero-Mutation State Transitions:** The graph remains immutable; the observation coordinates change, solving the non-global lifecycle trap.

---

*This repository serves as the companion code to the CppCon presentation: "Capability Reconstruction Graph: External Behavior Composition in C++".*
