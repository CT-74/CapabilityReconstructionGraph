# Capability Reconstruction Graph (CRG)

> A stateless architectural pattern for resolving contextual behavior in Data-Oriented Design without structural memory mutation.
> 📄 **[Read the full White Paper here](paper/paper.md)**

## Abstract
High-performance C++ applications often struggle to reconcile Data-Oriented Design (contiguous memory) with dynamic polymorphic behavior. In strict Archetype-based Entity Component Systems (ECS), adding or removing behavioral tags forces entities to migrate between memory chunks—a bottleneck known as Archetype Fragmentation.

The CRG solves this by decoupling logic from the data layout. It uses C++ variadic templates to project transient behavioral interfaces (Capabilities) onto immutable data based on N-Dimensional context axes (e.g., State, Authority, Environment). Behavior is resolved at runtime via O(1) bitwise intersection, entirely eliminating structural reallocations.

## Key Highlights & Performance Implications
* **Zero Structural Mutation:** Entities never change archetypes when context changes. Data remains perfectly contiguous and cache-friendly.
* **O(1) Resolution:** Behavior is resolved via static variadic intersections, eliminating runtime registry lookups or virtual dispatch overhead.
* **Registry-Free Extensibility:** Behaviors can be dynamically injected via loaded libraries (DLLs) without central registry locks.
* **Thread-Safe by Design:** The resolution topology is immutable at runtime, allowing lock-free concurrent observation across massive datasets.

## Performance Benchmarks
> Benchmarks executed on Apple M-Series (Clang 16, -O3) via Google Benchmark.
> **Dataset:** 256 bytes per entity | **Environment:** MacBook Air (Unified Memory).
> Full reproducible benchmark source code available in `/benchmarks/crg_benchmark.cpp`.

| Dataset Size | Pattern | Execution Time | Throughput | Latency vs CRG |
| :--- | :--- | :--- | :--- | :--- |
| **65,536 (Cache-bound)** | ECS Mutation | 1,024,633 ns | 30.50 Gi/s | **3.63x Slower** |
| | **CRG Projection** | **281,761 ns** | **110.91 Gi/s** | **Baseline** |
| **1,048,576 (Memory-bound)** | ECS Mutation | 21,896,037 ns | 22.84 Gi/s | **1.56x Slower** |
| | **CRG Projection** | **14,008,326 ns** | **35.69 Gi/s** | **Baseline** |

**Critical Observation:**
1. **Cache Dominance:** On a 16 MB dataset (64k entities), the CRG saturates the L2 cache with a throughput of **111 Gi/s**, outperforming traditional ECS by a factor of **3.63x**.
2. **RAM Saturation:** On a massive 256 MB dataset (1M entities), the CRG reaches the physical limits of the RAM (35.7 Gi/s). It maintains a **56% advantage** over ECS, proving that eliminating structural mutation is the only way to exploit 100% of the hardware bandwidth.

---

## Technical Q&A

### Q: Is this just another State Machine?
**A:** No. A traditional FSM is often internal to an object or managed by a controller. The CRG is an externalized, stateless projection. It reconstructs the necessary "capability" for an entity on-the-fly based on its current context coordinates, without the entity ever knowing its own state.

### Q: What is the "O(1)" cost exactly?
**A:** The "O(1)" refers to structural complexity. Resolution is local to the type's own matrix. The loop only iterates over the compile-time defined behaviors for that specific type (usually < 5). Since this is a small constant independent of the total system scale, the resolution is effectively O(1).

### Q: Why not just use a standard ECS?
**A:** The CRG is a symbiotic layer, not an ECS replacement. The ECS handles fast iteration over contiguous data; the CRG projects context-aware logic onto that data without triggering structural reallocations (Archetype Fragmentation).

### Q: Isn't the Match() function just a hidden if-else chain?
**A:** Yes, the CPU still branches. However, the CRG replaces structural mutation (expensive memory shuffling) with simple comparisons (bitwise/enum checks). The branch is moved out of the hot-path business logic into the contextual resolution phase.

---

## License & Legal Notice

**Author & Architect:** Cyril Tissier  
This project is licensed under the **Apache License 2.0**. See the [LICENSE](LICENSE) file for details.

*Legal & IP Disclaimer:* The CRG architectural pattern and its conceptual model are independent intellectual property. The C++ code provided in this repository consists strictly of clean-room implementations designed for educational and demonstrative purposes. They are entirely independent of, and do not contain, leak, or reference any proprietary production code, systems, or assets from the author's current or past professional capacities.