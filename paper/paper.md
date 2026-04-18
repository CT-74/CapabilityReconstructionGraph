# Capability Reconstruction Graph (CRG): Stateless Behavior Projection in C++

**Author & Architect:** Cyril Tissier

## 1. Abstract
High-performance C++ systems require strict data locality, typically achieved through Data-Oriented Design (DOD). However, implementing dynamic contextual behavior often forces state mutations. This paper introduces the Capability Reconstruction Graph (CRG), a pure C++ architectural pattern that provides O(1) contextual polymorphism. By projecting transient behavioral interfaces onto immutable data structures via compile-time variadic matrices, the CRG eliminates structural reallocations and central registry bottlenecks.

## 2. The Problem: Archetype Fragmentation
Traditional Entity Component Systems (ECS) maximize cache locality by grouping entities with identical components into contiguous arrays (Archetypes). However, when behavior is coupled to these components (e.g., using a `StateTag` to define logic), dynamically changing behavior requires modifying structural composition.

In strict Archetype-based ECS implementations (such as Unity DOTS [1] or Flecs [2]), adding or removing a component triggers a structural change: the system must allocate memory in a new archetype chunk, copy the entity's data, and free the old memory. This process, known as Archetype Fragmentation, causes implicit `memcpy` operations and introduces synchronization barriers (mutexes) that halt multithreaded pipelines [3].

## 3. The Solution: Stateless Behavior Projection
The CRG eliminates structural mutation by treating "State" as a coordinate on an N-Dimensional axis rather than a component modification. Instead of moving data to match a behavior, the CRG reconstructs the behavior to match the data.

## 4. Implementation Strategy
Using variadic templates, the CRG builds a static matrix of "Capabilities" (interfaces). When an entity's context changes (e.g., switching from `Idle` to `Combat`), the system performs an O(1) lookup in the capability matrix. The underlying entity data remains perfectly contiguous in its original memory location.

## 5. Performance Criteria & Benchmarks
To validate the efficiency of the CRG, a comparative benchmark was conducted against a traditional Archetype-based mutation model. 

### Methodology
* **Target:** 65,536 Entities.
* **Payload:** 256 bytes per entity (simulating Transform + Physics data).
* **Environment:** Apple M-Series (Clang 16, -O3 optimization) via Google Benchmark.
* **Scenario:** Measuring the cost of a dual-state transition (A -> B -> A) for the entire dataset.

### Results
| Implementation | Execution Time (65k) | Throughput | Observation |
| :--- | :--- | :--- | :--- |
| **ECS Archetype Mutation** | 1,033,657 ns | 30.23 Gi/s | Significant cache-miss overhead. |
| **CRG Projection** | **317,634 ns** | **98.38 Gi/s** | **Near-native memory bandwidth.** |

### Technical Analysis
The benchmark reveals a **3.25x performance advantage** for the CRG pattern. The performance gap is primarily driven by the "Memory Wall": 
1. **Structural Mutation (ECS):** Each state change forces a `memcpy` of 256 bytes. At scale, this saturates the memory bus and invalidates L1/L2 caches.
2. **Contextual Observation (CRG):** Data residency is 100%. The CPU prefetcher remains effective as data never leaves its contiguous array. The O(1) resolution cost is negligible compared to the cost of memory movement.

## 6. The 10-Stage Implementation Architecture
1. **Type-Safety:** Enforcing static interface requirements.
2. **Entity Space:** Stable runtime identity without central registries.
3. **Identity-Based Resolution:** Traversal-based lookups.
4. **External Semantics:** Separating logic from memory layout.
5. **Composition:** Building a deterministic capability matrix.
6. **Deterministic Fusion:** Achieving constant-time $O(1)$ resolution.
7. **The Local Mutation Limit:** Demonstrating why RAII fails to scale in graph-behaviors.
8. **The Pivot:** Introducing the Temporal Axis; replacing mutation with observation.
9. **N-Dimensional Expansion:** Implementing Variadic Axes and Small Buffer Optimization (SBO).
10. **The Symbiosis:** Projecting N-Dimensional logic onto inert, contiguous ECS data arrays.

## 7. References
[1] Unity Technologies. "Structural Changes." Unity Data-Oriented Technology Stack (DOTS) Documentation.
[2] Mertens, S. "Building an ECS #2: Archetypes and Vectorization." Flecs Architecture Documentation.
[3] Caini, M. "Sparse Sets vs Archetypes." EnTT Architecture Notes.
[4] Source Code: Reproducible benchmarks available in `/benchmarks/crg_benchmark.cpp`.

---

## License & Legal Notice
**Author & Architect:** Cyril Tissier  
Licensed under the **Apache License 2.0**.

*Legal & IP Disclaimer:* The CRG architectural pattern and its conceptual model are independent intellectual property. The C++ code provided consists of clean-room implementations designed for educational purposes. They are entirely independent of, and do not contain, leak, or reference any proprietary production code, systems, or assets from the author's current or past professional capacities.