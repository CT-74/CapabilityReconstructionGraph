# CRG Technical Specification (The Canon)

The Capability Reconstruction Graph is governed by strict architectural constraints. Violating these principles breaks the guarantees of deterministic performance and thread safety.

## 1. The Zero-Heap Mandate
Dynamic memory allocation is strictly forbidden during runtime capability resolution. 
* **Transport**: All hardware entities MUST be transported using the `HardwareHandle` interface.
* **Storage**: The `HardwareHandle` MUST employ Small Buffer Optimization (SBO) to guarantee stack-only semantics. If a payload exceeds the `SBO_SIZE` (typically 48-64 bytes), it is a compilation error (`static_assert`), forcing the developer to reconsider the payload weight.

## 2. Emergent Topology (No Registries)
The system MUST NOT rely on centralized, heap-allocated containers (like `std::vector` or `std::unordered_map`) to store behaviors.
* **Intrusive Linking**: Nodes are self-registering. They utilize CRTP and Intrusive Linked Lists (`LinkedNode`).
* **Link-Time Construction**: The graph topology emerges purely from object instantiation (typically static globals). The graph *is* the memory layout.

## 3. Immutability & The RAII Fallacy
The runtime topology of the behavior space MUST remain immutable.
* **No Runtime Unlinking**: Attempting to bind behavior availability to RAII scopes (registering on construct, unregistering on destruct) is a critical anti-pattern (The False Peak). It introduces severe thread-safety issues and race conditions.
* **Observation over Mutation**: The system does not change shape when the environment changes. Instead, context (Time, State, Authority) is passed as a coordinate to the observer.

## 4. Orthogonal Variadic Resolution
The routing matrix MUST resolve coordinates using C++17 fold expressions over an `Args...` pack. It does not store state; it merely computes the intersection of context to project the correct capability.
