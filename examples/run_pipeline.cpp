#include <iostream>
#include <chrono>
#include <memory>
#include <string>
#include "nlohmann/json.hpp"
#include "cbnetwork/cbnetwork.hpp"

using json = nlohmann::json;
using namespace cbnetwork;
using namespace std::chrono;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <network_config.json>" << std::endl;
        return 1;
    }

    std::shared_ptr<CBN> cbn;
    try {
        cbn = CBN::load_network_from_json(argv[1]);
    } catch (const std::exception& e) {
        std::cerr << "Error loading JSON network: " << e.what() << std::endl;
        return 1;
    }

    if (!cbn) {
        std::cerr << "Failed to load network from JSON." << std::endl;
        return 1;
    }

    auto start_total = high_resolution_clock::now();

    // Step 1
    auto s1_start = high_resolution_clock::now();
    cbn->find_local_attractors();
    auto s1_end = high_resolution_clock::now();
    double s1_time = duration<double>(s1_end - s1_start).count();

    // Step 2
    auto s2_start = high_resolution_clock::now();
    cbn->find_compatible_pairs();
    auto s2_end = high_resolution_clock::now();
    double s2_time = duration<double>(s2_end - s2_start).count();

    // Step 3
    auto s3_start = high_resolution_clock::now();
    cbn->mount_attractor_fields();
    auto s3_end = high_resolution_clock::now();
    double s3_time = duration<double>(s3_end - s3_start).count();

    auto end_total = high_resolution_clock::now();
    double total_time = duration<double>(end_total - start_total).count();

    // Collect metrics
    json res;
    res["status"] = "success";
    res["metrics"] = {
        {"step1_time", s1_time},
        {"step2_time", s2_time},
        {"step3_time", s3_time},
        {"total_time", total_time},
        {"local_attractors_count", (int)cbn->get_n_local_attractors()},
        {"compatible_pairs_count", (int)cbn->get_n_pair_attractors()},
        {"attractor_fields_count", (int)cbn->get_n_attractor_fields()}
    };

    std::cout << res.dump(4) << std::endl;

    return 0;
}
