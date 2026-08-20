#include <iostream>
#include <fstream>
#include <chrono>
#include <string>
#include <vector>
#include <memory>
#include "cbnetwork/cbnetwork.hpp"
#include "cbnetwork/coupling.hpp"

using namespace cbnetwork;
using namespace std::chrono;

int main() {
    std::cout << "Starting Strategic Experimentation Campaign..." << std::endl;

    std::ofstream csv("campaign_results.csv");
    csv << "sample_id,topology_type,coupling_type,n_local_attractors,n_attractor_fields,canalization_density,execution_time_s\n";

    // 10 contrasting examples
    for (int i = 1; i <= 10; ++i) {
        int topology_type = (i % 2 == 0) ? 3 : 4; // Alternate Cycle (3) and Path (4)
        std::string topo_name = (topology_type == 3) ? "cycle" : "path";

        std::shared_ptr<CouplingStrategy> coupling;
        std::string coupling_name;

        switch (i % 5) {
            case 1:
                coupling = std::make_shared<OrCoupling>();
                coupling_name = "OR";
                break;
            case 2:
                coupling = std::make_shared<AndCoupling>();
                coupling_name = "AND";
                break;
            case 3:
                coupling = std::make_shared<XorCoupling>();
                coupling_name = "XOR";
                break;
            case 4:
                coupling = std::make_shared<ConstantCoupling>(0);
                coupling_name = "CONST0";
                break;
            case 0:
                coupling = std::make_shared<ThresholdCoupling>(2);
                coupling_name = "THRESHOLD2";
                break;
        }

        std::cout << "Running Sample " << i << ": Topology=" << topo_name << ", Coupling=" << coupling_name << std::endl;

        auto start = high_resolution_clock::now();

        // Generate CBN
        auto cbn = CBN::cbn_generator(
            topology_type,
            5,    // n_networks
            8,    // n_var_network
            2,    // n_input_variables
            2,    // n_output_variables
            3,    // n_max_of_clauses
            2,    // n_max_of_literals
            -1,   // n_edges
            coupling
        );

        // Run pipeline
        cbn->find_local_attractors_parallel();
        cbn->find_compatible_pairs_parallel();
        cbn->mount_stable_attractor_fields_parallel();

        auto end = high_resolution_clock::now();
        double duration = duration_cast<milliseconds>(end - start).count() / 1000.0;

        // Metrics
        int n_attr = cbn->get_n_local_attractors();
        int n_fields = cbn->get_n_attractor_fields();
        double canalization = cbn->calculate_canalization_density();

        // Save Results
        csv << i << "," << topo_name << "," << coupling_name << "," << n_attr << "," << n_fields << "," << canalization << "," << duration << "\n";

        std::string json_file = "examples/test_topology_" + topo_name + "_" + coupling_name + "_" + std::to_string(i) + ".json";
        cbn->save_network_to_json(json_file);

        std::cout << "  -> Result: " << n_fields << " fields, Canalization: " << canalization << std::endl;
    }

    csv.close();
    std::cout << "Campaign completed. Results saved to campaign_results.csv" << std::endl;
    return 0;
}
