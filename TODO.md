# TODO: scmbn-core (High-Performance C++ Engine)

## ✅ Completed & Verified
- [x] **Biological/Custom Labels Support:** C++ JSON parser successfully reads `variable_names` and maps internal bitset states to readable gene/protein labels in exported results.
- [x] **Core Pipeline Implementation:** Local attractor discovery (Dubrova method), compatible pair matching, and stable attractor field assembly.
- [x] **JSON Data Contract:** Native integration with `nlohmann/json` for fast configuration parsing and topology serialization.

## 🚀 Active Priorities (Complex Networks 2026)
- [ ] **OpenMP Multi-threading Validation:** Profile thread scaling efficiency up to 8 logical threads during Step 1 to document Amdahl's Law constraints.
- [ ] **Memory Bounds & OOM Profiling:** Run automated stress tests from $N=3$ up to the kernel OOM limit to construct the Memory Wall curve.
- [ ] **Parity Verification:** Execute comparative test suites against the Python baseline to ensure 100% numerical consistency across all stages.

## 🛠️ Code Quality & Refactoring
- [ ] **Automated Test Suite:** Expand `test_parity` and `unit_tests` to run as a unified regression test suite via CTest.
- [ ] **Error Handling:** Implement custom error codes for malformed JSON topologies or invalid coupling rules.
- [ ] **Memory Hygiene:** Run Valgrind/AddressSanitizer audits to verify zero heap fragmentation or leaks during intensive Step 3 assemblies.

## 📚 Documentation & Packaging
- [ ] **CMake Install Targets:** Add proper `install()` directives in `CMakeLists.txt` for clean binary deployment.
- [ ] **API Documentation:** Add structured docstrings to core classes (`CBN`, `LocalNetwork`, `GlobalTopology`) for Doxygen.