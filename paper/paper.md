# The Capability Routing Gateway (CRG)
### Reaching Hardware Limits through Zero-Cost Logic Mapping & Universal Decoupling

**Cyril Tissier** | April 2026

---

## 1. Abstract
The **Capability Routing Gateway (CRG)** is an architectural framework designed to eliminate the primary bottlenecks in modern AAA engine development: structural memory overhead and rigid architectural coupling. By decoupling identity from behavior through a dual-governance backend, the CRG enables expressive external polymorphism while maintaining 100% data residency for performance-critical simulation. The architecture eliminates the "Structural Tax" of entity migration, allowing logic execution to reach the **Memory Bandwidth (BW) Bound** limit, saturating the system bus at **30.83 Gi/s**.

## 2. Decoupling via Linker-Level Discovery
The CRG achieves absolute inversion of control during static initialization, preventing the dependency rot and "Include Hell" common in large-scale C++ projects.
* **Auto-Registration:** Leveraging C++17 `static inline` constants, features register themselves to the Gateway without centralized registries or "god-functions." 
* **Domain Isolation:** Gameplay domains (Combat, AI, UI) exist as isolated projects, remaining invisible to the core engine and each other until link-time.
* **Linker Optimization:** Unused behaviors are entirely removed via **Dead Code Stripping**, ensuring zero runtime overhead and minimal binary footprint for specialized builds (e.g., dedicated servers).

## 3. Dual-Backend Governance
The CRG does not force a single paradigm onto the entire codebase. Instead, it bridges two distinct execution regimes through a unified routing matrix:

| Architectural Feature | The DOD Hot Path (Surgical) | The Interface Path (Expressive) |
| :--- | :--- | :--- |
| **Target Use Case** | Physics, Movement, Core Combat | Quest Systems, UI, Stateful Add-ons |
| **Routed Payload** | Pure C-Style Function Pointers | Stateful Virtual Interfaces (`Interface*`) |
| **Data Contract** | Strict **Params Sandbox** (Context Struct) | Unrestricted (Free-for-all C++ logic) |
| **Memory Access** | Linear & Sequential (100% Cache hits) | Graph-based (Random access allowed) |
| **Primary Goal** | Raw Throughput (Bandwidth Saturation) | Architectural Richness & API Callbacks |

## 4. The Stateless Shell (TES): The Virtual-Template Bypass
The **Stateless Shell (TES)** is a stack-allocated transport unit that solves the fundamental C++ limitation where `virtual` functions cannot be templates. 
* **Mechanism:** The TES acts as a type-erased carrier for data identity across domain boundaries.
* **Execution:** A stable virtual interface accepts the TES as its primary argument; the concrete implementation retrieves type-specific data via `shell.GetAs<T>()`. This allows generic interfaces to drive highly specific, templated logic with zero heap allocation.

## 5. Universal Auto-Discovery & Dynamic Baking
The CRG utilizes automated discovery to build its deterministic O(1) routing matrix:
* **The DomainBaker:** For modeled simulation types, the Baker constructs a flat routing table during initialization.
* **Global Logic Discovery:** For data-less behaviors, the system uses auto-registered linked lists. 
* **Magic Static Cache:** To eliminate traversal costs, the first call to a global logic path caches pointers into a **contiguous static vector**. This transforms "cold" discovery into a "warm" cache-friendly iteration for all subsequent frames.

## 6. Performance Criteria & Benchmarks

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
The benchmark results reveal two distinct performance regimes after correcting for read-bias. The data proves that by mapping behaviors to a contiguous matrix rather than relying on standard OOP VTables, the abstraction overhead is effectively zero:
* **Structural Immunity:** Entities never move between memory chunks (Archetypes) to change behavior. By keeping data resident and immobile, the CPU prefetcher achieves 100% efficiency.
* **Bandwidth Saturation:** The benchmark proves that with stable data locality, the bottleneck shifts from the call-site (pointer chasing) to the physical limits of the RAM, hitting **30.83 Gi/s** at massive scale. The engine becomes purely Memory Bandwidth Bound.

