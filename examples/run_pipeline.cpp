#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <memory>
#include <string>
#include <map>
#include "nlohmann/json.hpp"
#include "cbnetwork/cbnetwork.hpp"
#include "cbnetwork/localnetwork.hpp"
#include "cbnetwork/directededge.hpp"
#include "cbnetwork/internalvariable.hpp"

using json = nlohmann::json;
using namespace cbnetwork;
using namespace std::chrono;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <network_config.json>" << std::endl;
        return 1;
    }

    std::ifstream i(argv[1]);
    if (!i.is_open()) {
        std::cerr << "Could not open file: " << argv[1] << std::endl;
        return 1;
    }

    json j;
    try {
        i >> j;
    } catch (json::parse_error& e) {
        std::cerr << "Parse error: " << e.what() << std::endl;
        return 1;
    }

    std::vector<std::shared_ptr<LocalNetwork>> networks;
    std::map<int, std::shared_ptr<LocalNetwork>> net_map;

    // Load Local Networks
    if (!j.contains("local_networks")) {
        std::cerr << "JSON missing 'local_networks'" << std::endl;
        return 1;
    }

    for (auto& net_j : j["local_networks"]) {
        int idx = net_j["index"];
        std::vector<int> internal_vars = net_j["internal_variables"].get<std::vector<int>>();
        auto net = std::make_shared<LocalNetwork>(idx, internal_vars);

        for (auto& var_j : net_j["descriptive_function_variables"]) {
            int v_idx = var_j["index"];
            std::vector<std::vector<int>> cnf = var_j["cnf"].get<std::vector<std::vector<int>>>();
            auto var = std::make_shared<InternalVariable>(v_idx, cnf);
            net->descriptive_function_variables.push_back(var);
        }
        networks.push_back(net);
        net_map[idx] = net;
    }

    std::vector<std::shared_ptr<DirectedEdge>> edges;
    // Load Directed Edges
    if (j.contains("directed_edges")) {
        for (auto& edge_j : j["directed_edges"]) {
            int idx = edge_j["index"];
            int idx_var = edge_j["index_variable"];
            int in_net = edge_j["input_local_network"];
            int out_net = edge_j["output_local_network"];
            std::vector<int> out_vars = edge_j["output_variables"].get<std::vector<int>>();
            std::string coup_func = edge_j["coupling_function"];

            auto edge = std::make_shared<DirectedEdge>(idx, idx_var, in_net, out_net, out_vars, coup_func);

            if (edge_j.contains("true_table")) {
                std::map<std::string, std::string> tt = edge_j["true_table"].get<std::map<std::string, std::string>>();
                edge->true_table = tt;
            }

            edges.push_back(edge);
        }
    }

    // Connect input signals to networks
    for (auto& net : networks) {
        std::vector<std::shared_ptr<DirectedEdge>> inputs;
        for (auto& edge : edges) {
            if (edge->input_local_network == net->index) {
                inputs.push_back(edge);
            }
        }
        net->process_input_signals(inputs);
    }

    auto start_total = high_resolution_clock::now();

    CBN cbn(networks, edges);
    cbn.process_output_signals();

    // Step 1
    auto s1_start = high_resolution_clock::now();
    cbn.find_local_attractors_sequential();
    auto s1_end = high_resolution_clock::now();
    double s1_time = duration<double>(s1_end - s1_start).count();

    // Step 2
    auto s2_start = high_resolution_clock::now();
    cbn.find_compatible_pairs();
    auto s2_end = high_resolution_clock::now();
    double s2_time = duration<double>(s2_end - s2_start).count();

    // Step 3
    auto s3_start = high_resolution_clock::now();
    cbn.mount_stable_attractor_fields();
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
        {"local_attractors_count", 0},
        {"compatible_pairs_count", 0},
        {"attractor_fields_count", (int)cbn.d_attractor_fields.size()}
    };

    int total_local_attractors = 0;
    for(auto& net : cbn.l_local_networks) {
        total_local_attractors += net->attractor_count;
    }
    res["metrics"]["local_attractors_count"] = total_local_attractors;

    int total_pairs = 0;
    for(auto& edge : cbn.l_directed_edges) {
        total_pairs += edge->d_comp_pairs_attractors_by_value[0].size();
        total_pairs += edge->d_comp_pairs_attractors_by_value[1].size();
    }
    res["metrics"]["compatible_pairs_count"] = total_pairs;

    std::cout << res.dump(4) << std::endl;

    return 0;
}
