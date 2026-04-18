# Capability Reconstruction Graph (CRG)

**Author & Architect:** Cyril Tissier

**Legal Notice:**
The CRG architectural pattern and its conceptual model are my independent intellectual property. The C++ code provided here consists of clean-room implementations designed for educational purposes. They are independent of, and do not contain or leak, any proprietary production code or assets from my professional capacity.

---

## Overview

The CRG is a structural model for behavior composition in C++ systems. Instead of embedding functionality in objects, behavior is reconstructed through an **N-Dimensional Hypergraph** resolution.

It addresses a fundamental limitation in high-performance architectures (like Entity Component Systems): the trade-off between **Data Locality** and **Logical Branching**.

### The Core Concept: Stateless MVC

Traditional MVC/MVP patterns hit a "Coupling Wall" in massive-scale, data-oriented architectures. The CRG evolves the pattern by deconstructing the "Controller" into a **Stateless Projection**:

* **Model (Data):** Inert ECS components (POD). The data is unaware of the logic operating on it.
* **View (Capability):** A functional interface projected onto the data at the moment of observation.
* **Controller (Resolution):** Transient and emergent. It only exists during the $O(1)$ resolution phase through coordinate intersection.

### Comparison Matrix

| Aspect | Classic MVC/MVP | CRG (Stateless Projection) |
| :--- | :--- | :--- |
| **Binding** | Explicit (Controller points to Model) | Implicit (Coordinate intersection) |
| **Lifecycle** | Instance-based (Stateful) | Transient (Reconstructed on the stack) |
| **Memory Cost** | 1 instance per object | 1 Matrix for N entities ($O(1)$) |
| **State Change** | Mutation (Controller modifies Model) | Observation (Context shifts, Capability changes) |

---

## Why CRG?

Traditional ECS architectures suffer from **Archetype Fragmentation**. Changing an entity's behavior usually requires adding or removing tags, forcing the ECS to move data to a different memory chunk (memcpy) and wrecking cache locality.

The CRG solves this by shifting from **Mutation** (changing memory) to **Contextual Observation** (shifting coordinates). Behavior is reconstructed on the fly, keeping the underlying memory contiguous and untouched.

### Universal Applications

While built for high-performance engines, this zero-mutation approach fits any domain requiring deterministic, zero-latency logic shifts:

- **Agentic AI:** Real-time logical morphing for autonomous agents. Switch between reasoning modes in O(1) without structural reconfiguration or prompt-tax.
- **High-Frequency Trading (HFT):** Hot-swapping execution strategies based on market context (volatility/liquidity axes) with zero latency.
- **Digital Twins & Robotics:** Context-aware logic for massive-scale sensor fusion without state synchronization overhead.

---

## Performance & Benchmarks

To quantify the "Management Tax" and the impact of Archetype Fragmentation, the CRG architecture must be evaluated against traditional ECS component mutation.

While this repository contains educational implementations, the underlying architecture is designed to be benchmarked on the following metrics:

1. **Resolution Latency ($O(1)$):** Time taken to resolve a capability via N-Dimensional coordinate intersection vs. searching through a standard VTable or ECS registry.
2. **Cache-Miss Rate:** Comparing the L1/L2 cache misses when shifting behavior via ECS tag insertion (which triggers data relocation) versus CRG contextual observation (where data remains strictly contiguous).
3. **Concurrency Scaling:** Measuring lock contention when multiple threads shift entity contexts simultaneously across a large dataset (100k+ entities).

*(Note: Specific micro-benchmarks utilizing Google Benchmark are being integrated into the repository to formally verify these performance characteristics.)*

---

## The 10-Stage Evolution

This repository follows a step-by-step progression:

1. **Structural Primitive:** Intrusive self-registration.
2. **Identity Space:** Stable runtime identity without central registries.
3. **Identity-Based Resolution:** Traversal-based lookups.
4. **External Semantics:** Separating behavior from memory layout.
5. **Composition:** Building a deterministic capability matrix.
6. **Deterministic Fusion:** Achieving constant-time $O(1)$ resolution.
7. **The False Peak:** Why RAII and local mutations fail to scale in graph-behaviors.
8. **The Pivot:** Introducing the Temporal Axis. Replacing state mutation with contextual observation.
9. **N-Dimensional Expansion:** Implementing Variadic Axes and Zero-Cost Transports (SBO).
10. **The Symbiosis:** Final integration. Projecting N-Dimensional logic onto inert, contiguous ECS data.

---

## Technical Q&A

### Q: How does this claim O(1) resolution if there is a loop in Resolve?
**A:** The O(1) refers to structural complexity. Resolution is local to the type's own matrix. The loop only iterates over the number of behaviors defined for that specific type (usually < 5). Since this is a small constant independent of the total system scale, the resolution is effectively O(1).

### Q: Why not just use a standard ECS?
**A:** The CRG is a symbiotic layer, not an ECS replacement. It solves the Archetype Fragmentation problem. The ECS handles fast iteration over contiguous data; the CRG projects context-aware logic onto that data without triggering structural reallocations.

### Q: Isn't the Match() function just a hidden if-else chain?
**A:** Yes, the CPU still branches. However, the CRG replaces structural mutation (expensive memory shuffling) with simple comparisons (cheap enum/ID checks). The branch is moved out of the hot-path business logic into the contextual resolution phase.

### Q: How does this handle multi-threading?
**A:** The CRG topology is immutable at runtime. "State changes" are simply different coordinates provided to the Resolve function. Since no shared state is mutated during observation, the system is natively thread-safe for concurrent reads.

### Q: Why use a SBO (Small Buffer Optimization) Handle?
**A:** To ensure zero-allocation. By keeping the capability instance on the stack during the update tick, we avoid heap allocations on the hot path while maintaining strict decoupling.

---

## License

This project is licensed under the **Apache License 2.0**. 
See the [LICENSE](LICENSE) file for details.

*Copyright (c) 2026 Cyril Tissier*