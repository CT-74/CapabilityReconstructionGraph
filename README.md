# Capability Reconstruction Graph (CRG)

> A stateless architectural pattern for resolving contextual behavior in Data-Oriented Design without structural memory mutation.
> 📄 **[Read the full White Paper here](paper/paper.md)**
> 🎮 **[Try the Interactive Simulator (ECS vs CRG)](https://ct-74.github.io/CapabilityReconstructionGraph/demo/final_simulator/index.html)**

## Abstract
High-performance C++ applications often struggle to reconcile Data-Oriented Design (contiguous memory) with dynamic polymorphic behavior. In strict Archetype-based Entity Component Systems (ECS), adding or removing behavioral tags forces entities to migrate between memory chunks—a bottleneck known as Archetype Fragmentation.

The CRG solves this by decoupling logic from the data layout. It uses C++ variadic templates to project transient behavioral interfaces (Capabilities) onto immutable data based on N-Dimensional context axes (e.g., State, Authority, Environment). Behavior is resolved at runtime via O(1) bitwise intersection, entirely eliminating structural reallocations.

## Key Highlights & Performance Implications
* **Zero Structural Mutation:** Entities never change archetypes when context changes. Data remains perfectly contiguous and cache-friendly.
* **O(1) Resolution:** Behavior is resolved via static variadic intersections, eliminating runtime registry lookups or virtual dispatch overhead.
* **Registry-Free Extensibility:** Behaviors can be dynamically injected via loaded libraries (DLLs) without central registry locks.
* **Thread-Safe by Design:** The resolution topology is immutable at runtime, allowing lock-free concurrent observation across massive datasets.

## 🎮 Interactive Simulator
Experience the architectural difference visually! We have included a real-time HTML simulator that demonstrates the impact of Archetype Fragmentation (Classic ECS) versus Structural Immunity (CRG) under heavy mutation loads.

👉 **[Launch the Simulator : ECS vs CRG](https://ct-74.github.io/CapabilityReconstructionGraph/demo/final_simulator/index.html)** *(Open the file in your browser to run the simulation).*

## Performance Benchmarks
> Benchmarks executed on Apple M-Series (Clang 16, -O3) via Google Benchmark.
> **Dataset:** 256 bytes per entity | **Environment:** MacBook Air (Unified Memory).
> **Verification:** Read-bias corrected (forcing full 256-byte cache line load).

| Dataset Size | Pattern | Execution Time | Throughput | Latency vs CRG |
| :--- | :--- | :--- | :--- | :--- |
| **65,536 (Cache-bound)** | ECS Mutation | 887,026 ns | 35.23 Gi/s | **1.99x Slower** |
| | **CRG Projection** | **444,965 ns** | **70.23 Gi/s** | **Baseline** |
| **1,048,576 (Memory-bound)** | ECS Mutation | 25,956,111 ns | 19.26 Gi/s | **1.60x Slower** |
| | **CRG Projection** | **16,220,465 ns** | **30.83 Gi/s** | **Baseline** |

**Critical Observation:**
1. **Cache Regimes:** On a 16 MB dataset (64k entities), CRG effectively doubles performance (70.23 Gi/s) by avoiding the hidden overhead of archetype migration logic, even when forced to load full cache lines.
2. **Memory Wall:** On a 256 MB dataset (1M entities), CRG sustains 30.83 Gi/s, saturating the physical RAM bandwidth. It maintains a 60% lead over ECS, proving that structural mutation is the primary bottleneck for hardware-limit performance.

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
This project is licensed under the **Apache License 2.0**.

*Legal & IP Disclaimer:* The CRG architectural pattern and its conceptual model are independent intellectual property. The C++ code provided in this repository consists strictly of clean-room implementations designed for educational and demonstrative purposes. They are entirely independent of, and do not contain, leak, or reference any proprietary production code, systems, or assets from the author's current or past professional capacities.