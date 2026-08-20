# SC-MBN C++ High-Performance Engine (HPC Guide)

This directory contains the pure C++ implementation of the SC-MBN simulation pipeline, optimized for mass data production in HPC environments.

## Features
- **Native Generation**: Stochastic topology and dynamics generation (Complete, Cycle, Path digraphs).
- **HPC Optimized**: Constant memory footprint via strict scoping and pre-allocated vectors.
- **Resilient Logging**: Direct CSV output with immediate flushing.
- **SLURM Ready**: CLI support for easy integration with Job Arrays.

## Compilation

```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)