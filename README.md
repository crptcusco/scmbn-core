# scmbn-core

> High-performance native C++ computational engine for **Stationary-Coupled Modular Boolean Networks (SC-MBNs)**.

## 🧠 Overview

`scmbn-core` is the high-performance computation engine designed to model, simulate, and analyze **Stationary-Coupled Modular Boolean Networks (SC-MBNs)**. Moving away from standard high-level abstractions, this core relies on flat memory layouts, bit-packed representations (`std::bitset`), and shared-memory multi-core parallelism (OpenMP) to tackle combinatorial explosion in complex biological and network systems.

This repository powers the experimental evaluations presented for **Complex Networks 2026**.

---

## ⚡ Key Architectural Features

- **Bit-Packed States:** Utilizes `std::bitset` and contiguous flat vectors (`std::vector`) to maximize cache locality and minimize object overhead.
- **Advanced Attractor Discovery:** Implements optimized graph and boolean satisfiability approaches (Dubrova's method) for local module attractor identification.
- **Parallel Scalability:** Leverages OpenMP (`#pragma omp`) for efficient multi-core execution across independent scenario evaluations.
- **Native JSON Contract:** Seamlessly parses network templates and configuration files without heavy external Python dependencies.
- **Interpretable Output:** Full support for custom biological and gene nomenclature mapping (`variable_names`), translating raw bitsets into readable system components.

---

## 🗂️ Repository Structure

```text
scmbn-core/
├── include/cbnetwork/    # Core header files (.hpp)
├── src/                  # Native implementation files (.cpp)
├── examples/             # Executable drivers (benchmarks, pipelines, test suites)
├── CMakeLists.txt        # Modern CMake build configuration
└── README.md             # Technical documentation