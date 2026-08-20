#include "cbnetwork/experiment_strategies.hpp"
#include <algorithm>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <numeric>
#include <omp.h>
#include <sys/resource.h>
#include <malloc.h>
#include <new>
#include <stdexcept>

namespace cbnetwork {

using namespace std::chrono;

static size_t get_heap_allocated_bytes() {
#if defined(__GLIBC__) && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 33))
  struct mallinfo2 mi = mallinfo2();
  return mi.uordblks;
#else
  struct mallinfo mi = mallinfo();
  return (size_t)mi.uordblks;
#endif
}

static long get_max_rss() {
  struct rusage usage;
  getrusage(RUSAGE_SELF, &usage);
  return usage.ru_maxrss;
}

// Memory-efficient vector merge
static std::vector<int> merge_two_sorted_vectors(const std::vector<int> &v1,
                                                 const std::vector<int> &v2) {
  std::vector<int> res;
  res.reserve(v1.size() + v2.size());
  std::set_union(v1.begin(), v1.end(), v2.begin(), v2.end(),
                 std::back_inserter(res));
  return res;
}

// The local merge_field_lists_sequentially was removed in favor of the correct
// combinatorial implementation in CBN::mount_stable_attractor_fields().

// 1. TRADITIONAL EXPERIMENT
ExperimentResults TraditionalExperiment::run(std::shared_ptr<CBN> cbn) {
  ExperimentResults res;
  res.strategy_name = "Traditional";
  int prev_threads = omp_get_max_threads();
  omp_set_num_threads(1);
  auto start_total = high_resolution_clock::now();

  res.p1_ms = 0.0;
  res.p1_mem_kb = 0.0;
  res.p2_ms = 0.0;
  res.p2_mem_kb = 0.0;
  res.p3_ms = 0.0;
  res.p3_mem_kb = 0.0;
  res.total_ms = 0.0;
  res.max_rss_kb = 0;
  res.total_mem_kb = 0.0;
  res.global_attractors_count = 0;
  res.success = false;

  try {
    size_t mem_before_p1 = get_heap_allocated_bytes();
    auto start1 = high_resolution_clock::now();
    cbn->find_local_attractors_sequential();
    auto end1 = high_resolution_clock::now();
    size_t mem_after_p1 = get_heap_allocated_bytes();
    res.p1_ms = duration<double, std::milli>(end1 - start1).count();
    res.p1_mem_kb = (mem_after_p1 > mem_before_p1) ? (double)(mem_after_p1 - mem_before_p1) / 1024.0 : 0.0;
  } catch (const std::bad_alloc &e) {
    std::cerr << "[TraditionalExperiment] Out of memory in Phase 1 (Local Attractors): " << e.what() << std::endl;
    omp_set_num_threads(prev_threads);
    return res;
  } catch (const std::exception &e) {
    std::cerr << "[TraditionalExperiment] Exception in Phase 1 (Local Attractors): " << e.what() << std::endl;
    omp_set_num_threads(prev_threads);
    return res;
  }

  try {
    size_t mem_before_p2 = get_heap_allocated_bytes();
    auto start2 = high_resolution_clock::now();
    cbn->find_compatible_pairs_sequential(); // Strictly sequential execution
    auto end2 = high_resolution_clock::now();
    size_t mem_after_p2 = get_heap_allocated_bytes();
    res.p2_ms = duration<double, std::milli>(end2 - start2).count();
    res.p2_mem_kb = (mem_after_p2 > mem_before_p2) ? (double)(mem_after_p2 - mem_before_p2) / 1024.0 : 0.0;
  } catch (const std::bad_alloc &e) {
    std::cerr << "[TraditionalExperiment] Out of memory in Phase 2 (Compatible Pairs): " << e.what() << std::endl;
    omp_set_num_threads(prev_threads);
    return res;
  } catch (const std::exception &e) {
    std::cerr << "[TraditionalExperiment] Exception in Phase 2 (Compatible Pairs): " << e.what() << std::endl;
    omp_set_num_threads(prev_threads);
    return res;
  }

  try {
    size_t mem_before_p3 = get_heap_allocated_bytes();
    auto start3 = high_resolution_clock::now();
    cbn->mount_attractor_fields();
    auto end3 = high_resolution_clock::now();
    size_t mem_after_p3 = get_heap_allocated_bytes();
    res.p3_ms = duration<double, std::milli>(end3 - start3).count();
    res.p3_mem_kb = (mem_after_p3 > mem_before_p3) ? (double)(mem_after_p3 - mem_before_p3) / 1024.0 : 0.0;
  } catch (const std::bad_alloc &e) {
    std::cerr << "[TraditionalExperiment] Out of memory in Phase 3 (Attractor Fields): " << e.what() << std::endl;
    omp_set_num_threads(prev_threads);
    return res;
  } catch (const std::exception &e) {
    std::cerr << "[TraditionalExperiment] Exception in Phase 3 (Attractor Fields): " << e.what() << std::endl;
    omp_set_num_threads(prev_threads);
    return res;
  }

  auto end_total = high_resolution_clock::now();
  res.total_ms = duration<double, std::milli>(end_total - start_total).count();
  res.max_rss_kb = get_max_rss();
  res.total_mem_kb = res.p1_mem_kb + res.p2_mem_kb + res.p3_mem_kb;
  res.global_attractors_count = cbn->d_attractor_fields.size();
  res.success = true;
  omp_set_num_threads(prev_threads);
  return res;
}

// 2. SIMPLE PARALLEL EXPERIMENT
ExperimentResults SimpleParallelExperiment::run(std::shared_ptr<CBN> cbn) {
  ExperimentResults res;
  res.strategy_name = "SimpleParallel";
  auto start_total = high_resolution_clock::now();

  res.p1_ms = 0.0;
  res.p1_mem_kb = 0.0;
  res.p2_ms = 0.0;
  res.p2_mem_kb = 0.0;
  res.p3_ms = 0.0;
  res.p3_mem_kb = 0.0;
  res.total_ms = 0.0;
  res.max_rss_kb = 0;
  res.total_mem_kb = 0.0;
  res.global_attractors_count = 0;
  res.success = false;

  try {
    size_t mem_before_p1 = get_heap_allocated_bytes();
    auto start1 = high_resolution_clock::now();
    cbn->find_local_attractors_parallel();
    auto end1 = high_resolution_clock::now();
    size_t mem_after_p1 = get_heap_allocated_bytes();
    res.p1_ms = duration<double, std::milli>(end1 - start1).count();
    res.p1_mem_kb = (mem_after_p1 > mem_before_p1) ? (double)(mem_after_p1 - mem_before_p1) / 1024.0 : 0.0;
  } catch (const std::bad_alloc &e) {
    std::cerr << "[SimpleParallelExperiment] Out of memory in Phase 1 (Local Attractors): " << e.what() << std::endl;
    return res;
  } catch (const std::exception &e) {
    std::cerr << "[SimpleParallelExperiment] Exception in Phase 1 (Local Attractors): " << e.what() << std::endl;
    return res;
  }

  try {
    size_t mem_before_p2 = get_heap_allocated_bytes();
    auto start2 = high_resolution_clock::now();
    cbn->find_compatible_pairs_parallel();
    auto end2 = high_resolution_clock::now();
    size_t mem_after_p2 = get_heap_allocated_bytes();
    res.p2_ms = duration<double, std::milli>(end2 - start2).count();
    res.p2_mem_kb = (mem_after_p2 > mem_before_p2) ? (double)(mem_after_p2 - mem_before_p2) / 1024.0 : 0.0;
  } catch (const std::bad_alloc &e) {
    std::cerr << "[SimpleParallelExperiment] Out of memory in Phase 2 (Compatible Pairs): " << e.what() << std::endl;
    return res;
  } catch (const std::exception &e) {
    std::cerr << "[SimpleParallelExperiment] Exception in Phase 2 (Compatible Pairs): " << e.what() << std::endl;
    return res;
  }

  try {
    size_t mem_before_p3 = get_heap_allocated_bytes();
    auto start3 = high_resolution_clock::now();
    cbn->mount_attractor_fields();
    auto end3 = high_resolution_clock::now();
    size_t mem_after_p3 = get_heap_allocated_bytes();
    res.p3_ms = duration<double, std::milli>(end3 - start3).count();
    res.p3_mem_kb = (mem_after_p3 > mem_before_p3) ? (double)(mem_after_p3 - mem_before_p3) / 1024.0 : 0.0;
  } catch (const std::bad_alloc &e) {
    std::cerr << "[SimpleParallelExperiment] Out of memory in Phase 3 (Attractor Fields): " << e.what() << std::endl;
    return res;
  } catch (const std::exception &e) {
    std::cerr << "[SimpleParallelExperiment] Exception in Phase 3 (Attractor Fields): " << e.what() << std::endl;
    return res;
  }

  auto end_total = high_resolution_clock::now();
  res.total_ms = duration<double, std::milli>(end_total - start_total).count();
  res.max_rss_kb = get_max_rss();
  res.total_mem_kb = res.p1_mem_kb + res.p2_mem_kb + res.p3_mem_kb;
  res.global_attractors_count = cbn->d_attractor_fields.size();
  res.success = true;
  return res;
}

// 3. ADVANCED PARALLEL EXPERIMENT
ExperimentResults AdvancedParallelExperiment::run(std::shared_ptr<CBN> cbn) {
  ExperimentResults res;
  res.strategy_name = "AdvancedParallel";
  auto start_total = high_resolution_clock::now();

  res.p1_ms = 0.0;
  res.p1_mem_kb = 0.0;
  res.p2_ms = 0.0;
  res.p2_mem_kb = 0.0;
  res.p3_ms = 0.0;
  res.p3_mem_kb = 0.0;
  res.total_ms = 0.0;
  res.max_rss_kb = 0;
  res.total_mem_kb = 0.0;
  res.global_attractors_count = 0;
  res.success = false;

  try {
    size_t mem_before_p1 = get_heap_allocated_bytes();
    auto start1 = high_resolution_clock::now();
    cbn->find_local_attractors();
    auto end1 = high_resolution_clock::now();
    size_t mem_after_p1 = get_heap_allocated_bytes();
    res.p1_ms = duration<double, std::milli>(end1 - start1).count();
    res.p1_mem_kb = (mem_after_p1 > mem_before_p1) ? (double)(mem_after_p1 - mem_before_p1) / 1024.0 : 0.0;
  } catch (const std::bad_alloc &e) {
    std::cerr << "[AdvancedParallelExperiment] Out of memory in Phase 1 (Local Attractors): " << e.what() << std::endl;
    return res;
  } catch (const std::exception &e) {
    std::cerr << "[AdvancedParallelExperiment] Exception in Phase 1 (Local Attractors): " << e.what() << std::endl;
    return res;
  }

  try {
    size_t mem_before_p2 = get_heap_allocated_bytes();
    auto start2 = high_resolution_clock::now();
    cbn->find_compatible_pairs();
    auto end2 = high_resolution_clock::now();
    size_t mem_after_p2 = get_heap_allocated_bytes();
    res.p2_ms = duration<double, std::milli>(end2 - start2).count();
    res.p2_mem_kb = (mem_after_p2 > mem_before_p2) ? (double)(mem_after_p2 - mem_before_p2) / 1024.0 : 0.0;
  } catch (const std::bad_alloc &e) {
    std::cerr << "[AdvancedParallelExperiment] Out of memory in Phase 2 (Compatible Pairs): " << e.what() << std::endl;
    return res;
  } catch (const std::exception &e) {
    std::cerr << "[AdvancedParallelExperiment] Exception in Phase 2 (Compatible Pairs): " << e.what() << std::endl;
    return res;
  }

  try {
    size_t mem_before_p3 = get_heap_allocated_bytes();
    auto start3 = high_resolution_clock::now();
    cbn->mount_attractor_fields();
    auto end3 = high_resolution_clock::now();
    size_t mem_after_p3 = get_heap_allocated_bytes();
    res.p3_ms = duration<double, std::milli>(end3 - start3).count();
    res.p3_mem_kb = (mem_after_p3 > mem_before_p3) ? (double)(mem_after_p3 - mem_before_p3) / 1024.0 : 0.0;
  } catch (const std::bad_alloc &e) {
    std::cerr << "[AdvancedParallelExperiment] Out of memory in Phase 3 (Attractor Fields): " << e.what() << std::endl;
    return res;
  } catch (const std::exception &e) {
    std::cerr << "[AdvancedParallelExperiment] Exception in Phase 3 (Attractor Fields): " << e.what() << std::endl;
    return res;
  }

  auto end_total = high_resolution_clock::now();
  res.total_ms = duration<double, std::milli>(end_total - start_total).count();
  res.max_rss_kb = get_max_rss();
  res.total_mem_kb = res.p1_mem_kb + res.p2_mem_kb + res.p3_mem_kb;
  res.global_attractors_count = cbn->d_attractor_fields.size();
  res.success = true;
  return res;
}

} // namespace cbnetwork
