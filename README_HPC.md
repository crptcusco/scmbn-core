# SC-MBN C++ High-Performance Engine (HPC Guide)

This directory contains the pure C++ implementation of the SC-MBN simulation pipeline, optimized for mass data production in HPC environments.

## Architecture
The engine provides a direct, low-overhead parallel execution pipeline built on OpenMP multi-core scheduling. Abstraction layers and strategy wrappers have been removed to ensure minimum memory footprint and cache efficiency.

Core pipeline invocation:
```cpp
cbn->find_local_attractors();
cbn->find_compatible_pairs();
cbn->mount_attractor_fields();
```

## Compilation

```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

## Executables Reference for HPC Clusters

### Batch Experiments (`experiment_runner`)
Ideal for SLURM array jobs measuring scaling behavior across network parameters:
```bash
./experiment_runner --samples 100 --topology 1 --networks 10 --vars 12 --output_file hpc_results_${SLURM_ARRAY_TASK_ID}.csv
```

### Scientific Benchmarking (`scientific_benchmarking`)
Provides granular timing and RSS memory tracking per execution phase:
```bash
./scientific_benchmarking --samples 50 --networks 16 --vars 10 --dir ./benchmark_output
```

### Single Verification (`run_pipeline`)
Fast verification of network JSON models in high-throughput pipelines:
```bash
./run_pipeline network_model.json
```

### Campaign Execution (`campaign_runner`)
Processes structured JSON campaign configurations in batch:
```bash
./campaign_runner campaign_batch.json
```

### Verification (`unit_tests`)
Validates core boolean logic and coupling strategies prior to launching large HPC campaigns:
```bash
./unit_tests
```
