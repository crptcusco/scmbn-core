#include "cbnetwork/campaign_manager.hpp"
#include "cbnetwork/network_factory.hpp"
#include "cbnetwork/experiment_strategies.hpp"
#include "nlohmann/json.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>

namespace cbnetwork {

namespace fs = std::filesystem;
using json = nlohmann::json;

void CampaignManager::run_campaign(const std::string& config_path) {
    std::ifstream file(config_path);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open campaign config file: " << config_path << std::endl;
        return;
    }

    json j_config;
    file >> j_config;

    if (!j_config.contains("experiments") || !j_config["experiments"].is_array()) {
        std::cerr << "Error: Invalid campaign config format. Missing 'experiments' array." << std::endl;
        return;
    }

    fs::create_directories("experimentos");

    for (const auto& j_exp : j_config["experiments"]) {
        ExperimentConfig config;
        config.id = j_exp["id"].get<std::string>();
        config.v_topology = j_exp["v_topology"].get<int>();
        config.n_networks = j_exp["n_networks"].get<int>();
        config.n_var_network = j_exp["n_var_network"].get<int>();
        config.n_input_variables = j_exp["n_input_variables"].get<int>();
        config.n_output_variables = j_exp["n_output_variables"].get<int>();
        config.n_max_of_clauses = j_exp.value("n_max_of_clauses", 2);
        config.n_max_of_literals = j_exp.value("n_max_of_literals", 3);
        config.n_edges = j_exp.value("n_edges", -1);
        config.coupling_type = j_exp["coupling_type"].get<std::string>();
        config.coupling_param = j_exp.value("coupling_param", 0);

        std::cout << "Executing Experiment: " << config.id << " ..." << std::endl;

        std::string exp_dir = "experimentos/" + config.id;
        fs::create_directories(exp_dir);

        auto cbn = NetworkFactory::create_from_config(config);

        AdvancedParallelExperiment strategy;
        ExperimentResults results = strategy.run(cbn);

        if (results.success) {
            export_experiment_data(exp_dir + "/" + config.id, config, cbn, results);
            std::cout << "Experiment " << config.id << " completed successfully." << std::endl;
        } else {
            std::cerr << "Experiment " << config.id << " failed." << std::endl;
        }
    }
}

void CampaignManager::export_experiment_data(const std::string& base_path,
                                            const ExperimentConfig& config,
                                            std::shared_ptr<CBN> cbn,
                                            const ExperimentResults& results) {
    // 1. [id]_network.json
    cbn->save_network_to_json(base_path + "_network.json");

    // 2. [id]_dynamics.json
    json j_dynamics;

    // Local Attractors
    json j_local_attractors = json::array();
    for (const auto& net : cbn->l_local_networks) {
        json j_net_attractors;
        j_net_attractors["network_index"] = net->index;
        json j_attrs = json::array();
        for (const auto& scene : net->local_scenes) {
            for (const auto& attr : scene->l_attractors) {
                json j_attr;
                j_attr["global_index"] = attr->g_index;
                j_attr["local_index"] = attr->l_index;
                j_attr["scene_index"] = scene->index;

                json j_states = json::array();
                for (const auto& state : attr->l_states) {
                    j_states.push_back(state->l_variable_values);
                }
                j_attr["states"] = j_states;
                j_attrs.push_back(j_attr);
            }
        }
        j_net_attractors["attractors"] = j_attrs;
        j_local_attractors.push_back(j_net_attractors);
    }
    j_dynamics["local_attractors"] = j_local_attractors;

    // Compatible Pairs (from edges)
    json j_edges_pairs = json::array();
    for (const auto& edge : cbn->l_directed_edges) {
        json j_edge;
        j_edge["edge_index"] = edge->index;
        json j_vals = json::array();
        for (int val : {0, 1}) {
            json j_val_pairs;
            j_val_pairs["signal_value"] = val;
            j_val_pairs["pairs"] = edge->d_comp_pairs_attractors_by_value[val];
            j_vals.push_back(j_val_pairs);
        }
        j_edge["compatible_pairs"] = j_vals;
        j_edges_pairs.push_back(j_edge);
    }
    j_dynamics["compatible_pairs_by_edge"] = j_edges_pairs;

    // Global Attractor Fields
    json j_fields = json::array();
    for (auto const& [id, field] : cbn->d_attractor_fields) {
        json j_field;
        j_field["field_id"] = id;
        j_field["attractor_indices"] = field;
        j_fields.push_back(j_field);
    }
    j_dynamics["global_attractor_fields"] = j_fields;

    std::ofstream dynamics_out(base_path + "_dynamics.json");
    dynamics_out << j_dynamics.dump(4) << std::endl;

    // 3. [id]_performance.json
    json j_performance;
    j_performance["execution_time_ms"] = {
        {"total", results.total_ms},
        {"phase1_local", results.p1_ms},
        {"phase2_pairs", results.p2_ms},
        {"phase3_fields", results.p3_ms}
    };
    j_performance["memory_usage_kb"] = results.max_rss_kb;
    j_performance["canalization_density"] = cbn->calculate_canalization_density();
    j_performance["counts"] = {
        {"local_attractors", cbn->get_n_local_attractors()},
        {"global_fields", results.global_attractors_count}
    };

    std::ofstream performance_out(base_path + "_performance.json");
    performance_out << j_performance.dump(4) << std::endl;
}

} // namespace cbnetwork
