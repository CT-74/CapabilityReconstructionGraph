# Capability Routing Gateway (CRG)

**A Zero-Cost Data-Oriented Polymorphism Framework for C++ Game Engines**

[Read the White Paper](paper/paper.md) | [Launch Interactive Simulator](https://ct-74.github.io/CapabilityReconstructionGraph/demo/final_simulator/index.html)

## Abstract
CRG solves two fundamental bottlenecks in modern game engine development: execution overhead (cache misses and VTable pointer chasing) and architectural coupling (include hell and bloated compile times). It routes behaviors to entities based on their state, combining the flexibility of External Polymorphism with the raw throughput of Data-Oriented Design (DOD).

## Technical Architecture

### 1. Complete Decoupling & Auto-Discovery
CRG enforces Inversion of Control at compile/link time, eliminating monolithic registries.
* **Isolated Domains:** Gameplay features live in standalone projects (e.g., separate `.vcxproj` files). The core engine is blind to them, and they do not depend on each other.
* **Linker-Driven Registration:** Using C++17 `inline static` variables and SFINAE fold expressions, behaviors self-register during static initialization. There are no centralized `RegisterAll()` god-functions.
* **Frictionless Iteration:** Modifying a combat behavior only recompiles the combat domain. Removing a feature (like UI or a specific DLC) is done by simply excluding the project from the build. The linker resolves the rest.
* **Zero-Cost Feature Toggling:** Adding or removing behaviors (e.g., for dedicated servers) relies on C++ variadic templates (`TypeList`). Excluded behaviors are dropped via dead code stripping, ensuring optimal binary size and zero runtime checks.

### 2. Zero-Cost Polymorphism (The Hot Path)
CRG replaces cache-thrashing virtual objects (`Interface*`) with a static N-Dimensional Tensor (a flat `std::vector` of `std::array`).
* **Dense Mapping:** Type hashes map to contiguous, dense IDs. Routing becomes a strict O(1) array access.
* **Devirtualization:** The payload delivered to the ECS is a raw C-style function pointer (`void(*)(const Params&)`).
* **Hardware-Friendly:** By grouping entities by type and state, the engine resolves the function pointer once per batch. This allows contiguous memory processing, perfect branch prediction, and compiler auto-vectorization (SIMD).

### 3. Strict Data Contracts (The Params Sandbox)
To enforce DOD and prevent random memory access, CRG uses a Struct-of-Context pattern.
* **Payload-Driven:** Systems strictly define what data a behavior can access via a `Params` struct (e.g., `Combat::Params`).
* **No Global ECS Access:** Gameplay programmers do not query the ECS in their `Update()` methods. The engine handles the query, packs the contiguous data into the `Params` struct, and feeds it to the CRG.
* **Compile-Time Enforcement:** SFINAE bakers ensure the gameplay function signature perfectly matches the system's `Params` contract, guaranteeing type safety at compile time.

### 4. Dual-Backend Flexibility
CRG adapts the payload to the system's specific requirements, acting as a true gateway rather than a simple function pointer wrapper.
* **DOD Backend (Hot Path):** Yields raw function pointers for high-frequency systems (Movement, Physics, Combat) where CPU throughput is paramount.
* **Rich OOP Backend (Cold Path):** Yields stateful, virtual `Interface*` objects for low-frequency, highly complex systems (Quest logic, UI, Callbacks) where API richness outweighs raw performance.

### 5. Native Live-Coding & Tamper Resistance
Because CRG routes behaviors through a centralized tensor rather than embedded VTables, it natively supports hot-reloading without compromising retail security.
* **Development & Modding:** Domain DLLs (e.g., `CombatDomain.dll`) can be recompiled and swapped at runtime. CRG overwrites the function pointers in the tensor, applying the new logic on the very next frame without restarting the engine.
* **Production Security:** Hot-patching is compiled out via `constexpr` flags in retail builds. The tensor is locked post-initialization. Any malicious attempt to inject or redirect logic (e.g., aimbots) triggers an immediate crash.

## The Result: Hitting the Hardware Limit
By eliminating VTables, ensuring L1-Cache residency, and removing branching from the hot path, CRG’s abstraction overhead drops to zero. In read-only ECS benchmarks, the architecture executes instructions faster than RAM can supply data, effectively shifting the bottleneck entirely to memory bandwidth. It pushes the engine to the physical limits of the silicon.

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

**Q: Why not just use virtual functions?** Virtual dispatch requires pointer chasing through VTables, which is a cache-miss nightmare in a hot loop. CRG resolves the address once per batch and executes it across a data span, keeping the pipeline full.

**Q: How is this different from a state machine?** Traditional FSMs are usually internal to the object. CRG is an external, stateless dispatcher. The entity doesn't "have" a state; the Gateway routes the correct "capability" to it based on external context axes.

**Q: Is there a compilation cost?** The use of heavy template metaprogramming (SFINAE, Variadics) increases build times. However, the architectural benefit of not having to recompile the core engine when a gameplay behavior changes usually offsets this in large-scale production.

---

**Author:** Cyril Tissier  
**License:** Apache 2.0  
*Disclaimer: This is a clean-room implementation for educational purposes, independent of any proprietary production code.*