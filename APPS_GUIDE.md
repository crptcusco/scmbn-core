# scmbn-core: Applications & Tools Guide

This directory contains the production binaries, benchmarking suites, and verification tools for **Stationary-Coupled Modular Boolean Networks (SC-MBNs)**.

---

## 1. scientific_benchmarking
**Purpose:** The heavy-duty scientific research engine. It runs massive stochastic iterations (or evaluates a deterministic JSON file), measures precise phase execution times and kernel RSS memory, writes a clean summary CSV, and exports deep JSON execution traces (`dynamics.json`) for auditing.

### Compilation Target:
`scientific_benchmarking`

### Usage Examples:
- **Run a stochastic batch (default settings):**
  ```bash
  ./build/scientific_benchmarking --samples 100 --topology 3 --networks 12 --vars 10 --output results.csv --dir output_data/
  ```
- **Evaluate a single deterministic network configuration:**
  ```bash
  ./build/scientific_benchmarking --input network_config.json --dir output_data/
  ```
- **Enable verbose debug dumps to console:**
  ```bash
  ./build/scientific_benchmarking --samples 1 --debug-dump
  ```

---

## 2. experiment_runner
**Purpose:** Autonomous Campaign Execution System. Designed for HPC environments and batch workflows. It reads a high-level JSON campaign file and coordinates multi-sample structural and dynamic analysis.

### Compilation Target:
`experiment_runner`

### Usage:
  ```bash
  ./build/experiment_runner campaign_config.json
  ```

---

## 3. run_pipeline
**Purpose:** Lightweight diagnostic and inspection tool. It takes an exact network JSON file, executes the three-phase pipeline, and dumps structured performance metrics directly to standard output as a JSON string. Perfect for integration with Python orchestrators (`scmbn-toolkit`).

### Compilation Target:
`run_pipeline`

### Usage:
  ```bash
  ./build/run_pipeline network_config.json
  ```

---

## 4. unit_tests
**Purpose:** Rigorous logic verification suite. It validates boolean function evaluators and coupling strategies (OR, AND, Threshold CNF transformations) to guarantee absolute mathematical correctness.

### Compilation Target:
`unit_tests`

### Usage:
  ```bash
  ./build/unit_tests
  ```
*(Can also be executed via CTest: `ctest --output-on-failure` inside the build directory)*