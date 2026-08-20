# scmbn-core

> High-performance native C++ computational engine for **Stationary-Coupled Modular Boolean Networks (SC-MBNs)**.

## 🧠 Overview

`scmbn-core` is a streamlined, high-performance C++ computation engine designed to model, simulate, and analyze **Stationary-Coupled Modular Boolean Networks (SC-MBNs)**. The architecture directly invokes our high-performance parallel execution pipeline, eliminating intermediate strategy wrappers and unnecessary abstraction layers.

This repository powers experimental evaluations for complex modular network systems.

---

## ⚡ Key Architectural Features

- **Unified Parallel Pipeline:** Core routines (`find_local_attractors()`, `find_compatible_pairs()`, `mount_attractor_fields()`) execute directly on the `CBN` engine using OpenMP multi-core parallelism.
- **Bit-Packed States & Flat Layouts:** Utilizes contiguous flat vectors and bitset representations to maximize cache locality and minimize memory allocations.
- **Advanced Attractor Discovery:** Implements SAT-based (Dubrova's method) local attractor identification combined with parallel Cartesian compatibility filtering.
- **Native JSON Parser & Exporter:** Fast JSON importing/exporting for network configurations, trace execution dynamics, and performance metrics without external Python runtime dependencies.

---

## 🛠️ Building the Project

Ensure OpenMP and MiniSat development libraries are installed on your system.

```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

---

## 🚀 Executables & Usage

`scmbn-core` provides five specialized executables in the `build/` directory:

### 1. `run_pipeline` — Single Network JSON Verification
Executes the full pipeline on a single network definition specified in a JSON file and outputs verification metrics.

```bash
./run_pipeline <path_to_network.json>
```

### 2. `experiment_runner` — Batch Synthetic Production
Generates synthetic network topologies (Complete, Cycle, Path) and executes batch simulations across customizable parameters.

```bash
./experiment_runner --samples 10 --topology 1 --networks 5 --vars 8 --output_file results.csv --save_fields
```

### 3. `scientific_benchmarking` — Production Benchmarking & Trace Generation
Runs scientific benchmarks on random or loaded JSON networks, recording stage timing, RSS memory usage, CSV summary logs, and detailed JSON execution traces.

```bash
./scientific_benchmarking --samples 10 --topology 3 --networks 12 --vars 10 --dir ../experiments/results
```

### 4. `campaign_runner` — Autonomous Config-Driven Campaigns
Executes a suite of experiments defined in a JSON campaign file, exporting network structures, dynamic traces, and performance data into the `experimentos/` directory.

```bash
./campaign_runner <campaign_config.json>
```

### 5. `unit_tests` — Core Verification Suite
Executes unit tests for logical operations, CNF evaluation, and coupling strategies (OR, AND, Threshold).

```bash
./unit_tests
```

---

## 🗂️ Repository Structure

```text
scmbn-core/
├── include/cbnetwork/    # Core header files (.hpp)
├── src/                  # Native engine implementation files (.cpp)
├── examples/             # Production drivers and test suites
│   ├── campaign_runner.cpp
│   ├── experiment_runner.cpp
│   ├── run_pipeline.cpp
│   ├── scientific_benchmarking.cpp
│   └── unit_tests.cpp
├── CMakeLists.txt        # Build system configuration
├── README.md             # Main documentation
└── README_HPC.md         # High-Performance Computing guide
```
