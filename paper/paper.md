# Capability Reconstruction Graph (CRG): Stateless Behavior Projection in C++

**Author & Architect:** Cyril Tissier

## 1. Abstract
High-performance C++ systems require strict data locality. However, implementing dynamic contextual behavior often forces state mutations. This paper introduces the Capability Reconstruction Graph (CRG), providing O(1) contextual polymorphism. By projecting transient behavioral interfaces onto immutable data structures via compile-time variadic matrices, the CRG eliminates structural reallocations and central registry bottlenecks.

## 2. The Problem: Archetype Fragmentation
In strict Archetype-based ECS implementations, adding or removing a component triggers a structural change: the system must allocate memory in a new archetype chunk, copy the entity's data, and free the old memory. This process, known as Archetype Fragmentation, causes implicit `memcpy` operations and introduces synchronization barriers that halt multithreaded pipelines.

## 3. The Solution: Stateless Behavior Projection
The CRG eliminates structural mutation by treating "State" as a coordinate on an N-Dimensional axis. Instead of moving data to match a behavior, the CRG reconstructs the behavior to match the data.

## 5. Performance Criteria & Benchmarks

### Methodology
* **Target:** Variable scale from 1,024 to 1,048,576 Entities.
* **Payload:** 256 bytes per entity (forced full cache line load).
* **Environment:** MacBook Air (Apple M-Series, Clang 16, -O3).

### Results
| Entities | ECS Mutation (Throughput) | CRG Projection (Throughput) | Ratio |
| :--- | :--- | :--- | :--- |
| 65,536 | 35.23 Gi/s | **70.23 Gi/s** | **1.99x** |
| 1,048,576 | 19.26 Gi/s | **30.83 Gi/s** | **1.60x** |

### Technical Analysis: Read-Bias Correction & RAM Saturation
The benchmark results reveal two distinct performance regimes after correcting for read-bias:

1. **The Cache Regime (< 16 MB):** When data resides in L2 cache, the CRG achieves 70 Gi/s. The ECS model, despite cache residency, is bottlenecked by the logic of mutation and structural management, performing 50% slower.

2. **The RAM Regime (> 64 MB):** At 1M entities (256 MB), the CRG throughput of 30.8 Gi/s represents the physical saturation of the memory bus. Traditional ECS suffers from structural overhead, dropping below 20 Gi/s.

**Conclusion:** The CRG is "Optimal by Design." It allows software to reach hardware-limit speeds. By maintaining total data residency, the CPU prefetcher remains 100% efficient.

## 6. The 10-Stage Implementation Architecture
1. **Type-Safety:** Enforcing static interface requirements for capabilities.
2. **Entity Space:** Stable runtime identity without central registries.
3. **Identity-Based Resolution:** Traversal-based lookups for context.
4. **External Semantics:** Separating logic from memory layout.
5. **Composition:** Building a deterministic capability matrix.
6. **Deterministic Fusion:** Achieving constant-time $O(1)$ resolution.
7. **The Local Mutation Limit:** Demonstrating why RAII fails to scale in graph-behaviors.
8. **The Pivot:** Introducing the Temporal Axis; replacing mutation with observation.
9. **N-Dimensional Expansion:** Implementing Variadic Axes and Small Buffer Optimization (SBO).
10. **The Symbiosis:** Projecting N-Dimensional logic onto inert, contiguous ECS data arrays.

## 7. References
[1] Unity Technologies. "Structural Changes." Unity DOTS Documentation.
[2] Mertens, S. "Building an ECS #2: Archetypes and Vectorization." Flecs Architecture.
[3] Source Code: Reproducible benchmarks available in `/benchmarks/crg_benchmark.cpp`.

---

## License & Legal Notice
**Author & Architect:** Cyril Tissier  
Licensed under the **Apache License 2.0**.