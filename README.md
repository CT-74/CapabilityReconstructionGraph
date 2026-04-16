# Capability Reconstruction Graph (CRG)

### *"The Matrix doesn't manage; it observes."*

The **CRG** is a zero-allocation, N-dimensional architectural pattern designed for autonomous systems, high-frequency robotics, and Smart Factories. It solves the scalability and branching nightmares inherent to Centralized Managers (the "God Controller" anti-pattern).

## The Problem
In traditional OOP, as hardware complexity grows, routing logic degenerates into massive `dynamic_cast` switch statements. Adding a single new Drone or Sensor requires modifying the central core, leading to merge conflicts, brittle tests, and compromised thread safety.

## The CRG Solution
- **Zero-Allocation**: Transports utilize `HardwareHandle` powered by Small Buffer Optimization (SBO). No heap allocation (`new`/`unique_ptr`) occurs on the hot path.
- **Emergent Topology**: Components self-register via intrusive linked lists at link-time. There is no central runtime registry to mutate.
- **N-Dimensional Resolution**: Context (e.g., Weather, Security, Power State) is treated as a set of orthogonal coordinates. The variadic matrix resolves the correct capability in `O(N)` or `O(1)` without mutating the graph structure.

## The 10-Stage Journey
Explore the `demo/` folder to follow the exact narrative of the CppCon 2026 talk, moving step-by-step from a rigid baseline to a variadic hypergraph.
