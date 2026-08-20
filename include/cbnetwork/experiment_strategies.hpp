#ifndef EXPERIMENT_STRATEGIES_HPP
#define EXPERIMENT_STRATEGIES_HPP

#include <vector>
#include <memory>
#include <chrono>
#include <map>
#include <string>
#include <iostream>
#include "cbnetwork/cbnetwork.hpp"

namespace cbnetwork {

/**
 * Result structure for rigorous benchmarking.
 */
struct ExperimentResults {
    std::string strategy_name;
    double p1_ms; // Local Attractors
    double p2_ms; // Compatible Pairs
    double p3_ms; // Global Attractor Fields
    double total_ms;
    long max_rss_kb;
    double p1_mem_kb;
    double p2_mem_kb;
    double p3_mem_kb;
    double total_mem_kb;
    size_t global_attractors_count;
    bool success;

    void print() const {
        std::cout << "Strategy: " << strategy_name << std::endl;
        std::cout << "  - Phase 1 (Local):  " << p1_ms << " ms (" << p1_mem_kb << " KB)" << std::endl;
        std::cout << "  - Phase 2 (Pairs):  " << p2_ms << " ms (" << p2_mem_kb << " KB)" << std::endl;
        std::cout << "  - Phase 3 (Fields): " << p3_ms << " ms (" << p3_mem_kb << " KB)" << std::endl;
        std::cout << "  - Max RSS:          " << max_rss_kb << " KB" << std::endl;
        std::cout << "  - Total Heap Mem:   " << total_mem_kb << " KB" << std::endl;
        std::cout << "  - Total Fields:     " << global_attractors_count << std::endl;
    }
};

/**
 * Common interface for all execution paradigms.
 */
class ExperimentStrategy {
public:
    virtual ~ExperimentStrategy() = default;
    virtual ExperimentResults run(std::shared_ptr<CBN> cbn) = 0;
};

/**
 * 1. TraditionalExperiment: Strict Single-threaded Baseline.
 */
class TraditionalExperiment : public ExperimentStrategy {
public:
    ExperimentResults run(std::shared_ptr<CBN> cbn) override;
};

/**
 * 2. SimpleParallelExperiment: Flat OpenMP parallelism.
 */
class SimpleParallelExperiment : public ExperimentStrategy {
public:
    ExperimentResults run(std::shared_ptr<CBN> cbn) override;
};

/**
 * 3. AdvancedParallelExperiment: Bucket Balancing, O(1) Lookups, Asynchronous Binary Reduction.
 */
class AdvancedParallelExperiment : public ExperimentStrategy {
public:
    ExperimentResults run(std::shared_ptr<CBN> cbn) override;
};

} // namespace cbnetwork

#endif // EXPERIMENT_STRATEGIES_HPP
