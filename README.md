# Capability Routing Gateway (CRG)

**Zero-Cost Data-Oriented Polymorphism for C++ Game Engines**

[Read the White Paper](paper/paper.md) | [Launch Interactive Simulator](https://ct-74.github.io/CapabilityReconstructionGraph/demo/final_simulator/index.html)

## Abstract
Modern AAA engines often struggle to reconcile Data-Oriented Design (DOD) with the need for dynamic, contextual behavior. Traditional ECS models solve this via Archetype mutation, but this leads to memory fragmentation and structural overhead when entities change state frequently.

The **Capability Routing Gateway (CRG)** provides a decoupled alternative. By using C++17 variadic templates and linker-driven auto-discovery, it projects behavioral interfaces onto immutable data layouts. Logic is resolved through an O(1) flat tensor lookup, delivering the flexibility of polymorphism with the performance of raw function pointers.

## Technical Architecture

* **Linker-Based Auto-Discovery:** Behaviors are registered via `inline static const` markers and fold expressions during static initialization. This allows total decoupling: gameplay domains (DLLs/Modules) can be added or removed from the build without modifying the core engine or central registries.
* **Structural Immunity:** Since logic is routed externally, entities never migrate between memory chunks. This eliminates Archetype Fragmentation and keeps memory access patterns perfectly linear.
* **Compile-Time Feature Toggling:** Leveraging `TypeList` and variadic packs, features (e.g., debug tools, server-side logic) can be toggled via macros. The compiler performs dead-code stripping on unused branches, ensuring zero runtime or binary overhead.
* **The Params Sandbox:** To prevent random memory access (the "Get" anti-pattern), behaviors are restricted to a `Params` struct context. This enforces strict data ownership and allows the compiler to optimize for SIMD and register pressure.
* **Dual-Backend Dispatch:** The gateway handles both the **Hot Path** (direct function pointers for high-frequency systems like Combat/Physics) and the **Cold Path** (virtual interfaces for stateful systems like Quests or UI).

## Implementation Note
> **Important:** The current demo implementation provided in this repository serves as a functional prototype and does not yet reflect the fully optimized production core. For architectural simplicity, the demo still utilizes **virtual `Match()` calls** within the hot-path resolution logic. The finalized CRG architecture replaces these virtual dispatches with the branchless, static tensor-based routing described above to achieve true hardware-limit performance.

## Performance Analysis
> Hardware: Apple M-Series (Clang 16, -O3)
> Dataset: 256 bytes per entity (Cache-line biased)

| Dataset Size | Pattern | Execution Time | Throughput | Latency vs CRG |
| :--- | :--- | :--- | :--- | :--- |
| **64k (Cache-bound)** | ECS Mutation | 887,026 ns | 35.23 Gi/s | **1.99x Slower** |
| | **CRG Routing** | **444,965 ns** | **70.23 Gi/s** | **Baseline** |
| **1M (Memory-bound)** | ECS Mutation | 25,956,111 ns | 19.26 Gi/s | **1.60x Slower** |
| | **CRG Routing** | **16,220,465 ns** | **30.83 Gi/s** | **Baseline** |

**Observations:** At scale, CRG saturates the physical memory bandwidth (30.83 Gi/s). The delta between ECS and CRG represents the "Structural Tax" of archetype migration. By moving state resolution from memory layout to routing logic, we reclaim up to 60% of lost throughput.

## Technical FAQ

**Q: Why not just use virtual functions?**
Virtual dispatch requires pointer chasing through VTables, which is a cache-miss nightmare in a hot loop. CRG resolves the address once per batch and executes it across a data span, keeping the pipeline full.

**Q: How is this different from a state machine?**
Traditional FSMs are usually internal to the object. CRG is an external, stateless dispatcher. The entity doesn't "have" a state; the Gateway routes the correct "capability" to it based on external context axes.

**Q: Is there a compilation cost?**
The use of heavy template metaprogramming (SFINAE, Variadics) increases build times. However, the architectural benefit of not having to recompile the core engine when a gameplay behavior changes usually offsets this in large-scale production.

---

**Author:** Cyril Tissier  
**License:** Apache 2.0  
*Disclaimer: This is a clean-room implementation for educational purposes, independent of any proprietary production code.*