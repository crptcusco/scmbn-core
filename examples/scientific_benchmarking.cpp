#include "cbnetwork/cbnetwork.hpp"
#include "cbnetwork/experiment_strategies.hpp"
#include "nlohmann/json.hpp"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>

using namespace cbnetwork;
namespace fs = std::filesystem;
using json = nlohmann::json;

std::string get_filename(int sample_id, const std::string &strategy, const std::string &type) {
  return "cbn_sample_" + std::to_string(sample_id) + "_" + strategy + "_" + type;
}

void log_to_csv(const std::string &filename, const ExperimentResults &res, int sample_id, int topology, int networks, int vars) {
  bool file_exists = fs::exists(filename);
  std::ofstream out(filename, std::ios_base::app);
  if (!out.is_open()) return;
  if (!file_exists) out << "sample_id,strategy,topology,n_networks,n_vars,p1_ms,p2_ms,p3_ms,max_rss_kb,fields\n";
  out << sample_id << "," << res.strategy_name << "," << topology << "," << networks << "," << vars << "," << std::fixed << std::setprecision(4) << res.p1_ms << "," << std::fixed << std::setprecision(4) << res.p2_ms << "," << std::fixed << std::setprecision(4) << res.p3_ms << "," << res.max_rss_kb << "," << res.global_attractors_count << "\n";
}

void export_network_structure_json(int sample_id, std::shared_ptr<CBN> cbn, const std::string &output_dir) {
  std::string filepath = output_dir + "/" + get_filename(sample_id, "base", "network_structure.json");
  cbn->save_network_to_json(filepath);
}

void export_full_trace_json(int sample_id, const std::string &strategy_name, std::shared_ptr<CBN> cbn, const ExperimentResults &res, const std::string &output_dir, const std::string& input_id, int v_topology) {
  std::string filepath = output_dir + "/" + get_filename(sample_id, strategy_name, "dynamics.json");
  json j_out;

  // 1. Topology Metadata
  j_out["topology_metadata"] = {
      {"v_topology", v_topology},
      {"topology_strategy", strategy_name},
      {"topology_id", input_id},
      {"n_networks", (int)cbn->l_local_networks.size()},
      {"n_var_network", (int)cbn->get_n_local_variables()}
  };

  // 2. Pipeline Execution
  json j_pipeline;

  // Step 1: Local Attractors
  json j_step1 = json::array();
  for (auto const& net : cbn->l_local_networks) {
      for (auto const& scene : net->local_scenes) {
          json j_net_scene;
          j_net_scene["network_id"] = net->index;
          j_net_scene["applied_function"] = "Logic_CNF";

          json j_local_scenario = json::array();
          if (!scene->l_values.empty()) {
              for (char c : scene->l_values.at(0)) j_local_scenario.push_back(c - '0');
          }
          j_net_scene["local_scenario"] = j_local_scenario;

          json j_attrs = json::array();
          for (auto const& attr : scene->l_attractors) {
              json j_attr;
              j_attr["attractor_id"] = attr->l_index;
              j_attr["global_id"] = attr->g_index;
              j_attr["length"] = (int)attr->l_states.size();

              json j_states_unpacked = json::array();
              for (auto const& state : attr->l_states) {
                  json j_state_vars;
                  for (size_t i = 0; i < net->internal_variables.size(); ++i) {
                      j_state_vars["var_" + std::to_string(net->internal_variables.at(i))] = state->l_variable_values.at(i);
                  }
                  j_states_unpacked.push_back(j_state_vars);
              }
              j_attr["states_unpacked"] = j_states_unpacked;
              j_attrs.push_back(j_attr);
          }
          j_net_scene["attractors"] = j_attrs;
          j_step1.push_back(j_net_scene);
      }
  }
  j_pipeline["step_1_local_attractors"] = j_step1;

  // Step 2: Compatible Pairs
  json j_step2 = json::array();
  for (auto const& edge : cbn->l_directed_edges) {
      for (int val : {0, 1}) {
          if (edge->d_comp_pairs_attractors_by_value.count(val)) {
              for (auto const& p : edge->d_comp_pairs_attractors_by_value.at(val)) {
                  json j_pair;
                  j_pair["source_network"] = edge->output_local_network;
                  j_pair["target_network"] = edge->input_local_network;
                  j_pair["source_attractor_id"] = p.first;
                  j_pair["target_attractor_id"] = p.second;
                  j_pair["coupling_value"] = val;
                  j_pair["is_compatible"] = true;
                  j_step2.push_back(j_pair);
              }
          }
      }
  }
  j_pipeline["step_2_compatible_pairs"] = j_step2;

  // Step 3: Global Fields
  json j_step3;
  j_step3["global_scenario"] = "N/A";

  json j_fields = json::array();
  for (auto const& [id, field] : cbn->d_attractor_fields) {
      json j_f;
      j_f["field_id"] = id;
      j_f["attractor_indices"] = field;

      json j_assembled = json::array();
      for (int g_idx : field) {
          if (cbn->d_local_attractors.count(g_idx)) {
              auto info = cbn->d_local_attractors.at(g_idx);
              json j_inf;
              j_inf["net"] = std::get<0>(info);
              j_inf["attr"] = g_idx;
              j_assembled.push_back(j_inf);
          }
      }
      j_f["assembled_from_attractors"] = j_assembled;

      json j_global_unpacked = json::array();
      json j_global_state_vars;
      for (int g_idx : field) {
          auto attr_ptr = cbn->get_local_attractor_by_index(g_idx);
          if (attr_ptr && !attr_ptr->l_states.empty()) {
              auto net = cbn->get_network_by_index(attr_ptr->network_index);
              if (net) {
                  auto first_state = attr_ptr->l_states.at(0);
                  for (size_t i = 0; i < net->internal_variables.size(); ++i) {
                      j_global_state_vars["var_" + std::to_string(net->internal_variables.at(i))] = first_state->l_variable_values.at(i);
                  }
              }
          }
      }
      j_global_unpacked.push_back(j_global_state_vars);
      j_f["global_state_unpacked"] = j_global_unpacked;

      j_fields.push_back(j_f);
  }
  j_step3["attractor_fields"] = j_fields;
  j_pipeline["step_3_global_fields"] = j_step3;

  j_out["pipeline_execution"] = j_pipeline;

  // 3. Performance Metrics (Ensuring it is named "performance" to match solver)
  j_out["performance"] = {
      {"step_1_ms", res.p1_ms},
      {"step_2_ms", res.p2_ms},
      {"step_3_ms", res.p3_ms},
      {"total_ms", res.total_ms},
      {"max_rss_kb", (double)res.max_rss_kb},
      {"mem_p1_kb", res.p1_mem_kb},
      {"mem_p2_kb", res.p2_mem_kb},
      {"mem_p3_kb", res.p3_mem_kb},
      {"mem_total_kb", res.total_mem_kb},
      {"success", res.success}
  };

  std::ofstream out(filepath);
  if (out.is_open()) out << j_out.dump(4) << std::endl;
}

int main(int argc, char **argv) {
  int n_samples = 100, n_networks = 12, n_vars = 10, topology = 3;
  bool debug_dump = false;
  std::string custom_output_file = "", custom_output_dir = "", input_json = "";
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--samples" && i + 1 < argc) n_samples = std::stoi(argv[++i]);
    else if ((arg == "--networks" || arg == "--nodes") && i + 1 < argc) n_networks = std::stoi(argv[++i]);
    else if (arg == "--vars" && i + 1 < argc) n_vars = std::stoi(argv[++i]);
    else if (arg == "--topology" && i + 1 < argc) topology = std::stoi(argv[++i]);
    else if (arg == "--output" && i + 1 < argc) custom_output_file = argv[++i];
    else if (arg == "--dir" && i + 1 < argc) custom_output_dir = argv[++i];
    else if (arg == "--input" && i + 1 < argc) { input_json = argv[++i]; n_samples = 1; }
    else if (arg == "--debug-dump") debug_dump = true;
  }
  std::string output_dir = custom_output_dir.empty() ? "../experiments/results" : custom_output_dir;
  fs::create_directories(output_dir);
  std::string output_file = custom_output_file.empty() ? output_dir + "/benchmark_results.csv" : custom_output_file;

  std::string input_id = "random_gen";
  if (!input_json.empty()) {
      input_id = fs::path(input_json).stem().string();
  }

  std::vector<std::pair<std::string, std::unique_ptr<ExperimentStrategy>>> strategies;
  strategies.push_back({"Traditional", std::make_unique<TraditionalExperiment>()});
  strategies.push_back({"SimpleParallel", std::make_unique<SimpleParallelExperiment>()});
  strategies.push_back({"AdvancedParallel", std::make_unique<AdvancedParallelExperiment>()});

  for (int s = 0; s < n_samples; ++s) {
    int sample_id = s + 1;
    try {
      std::shared_ptr<CBN> reference_cbn;
      if (!input_json.empty()) {
        reference_cbn = CBN::load_network_from_json(input_json);
      } else {
        reference_cbn = CBN::cbn_generator(topology, n_networks, n_vars, 1, 1);
      }
      if (!reference_cbn)
        throw std::runtime_error("CBN load failed");
      if (input_json.empty())
        export_network_structure_json(sample_id, reference_cbn, output_dir);
      for (auto &s_pair : strategies) {
        auto cbn = reference_cbn->clone();
        auto results = s_pair.second->run(cbn);
        if (debug_dump && s_pair.first == "AdvancedParallel") {
           std::cout << "\n--- DEBUG DUMP: LOCAL ATTRACTORS ---" << std::endl;
           cbn->show_local_attractors();
           std::cout << "\n--- DEBUG DUMP: COMPATIBLE PAIRS ---" << std::endl;
           cbn->show_attractor_pairs();
           std::cout << "\n--- DEBUG DUMP: ATTRACTOR FIELDS ---" << std::endl;
           cbn->show_stable_attractor_fields();
        }
        log_to_csv(output_file, results, sample_id, topology, n_networks, n_vars);
        export_full_trace_json(sample_id, s_pair.first, cbn, results, output_dir, input_id, topology);
      }
    } catch (const std::exception &e) { std::cerr << "Error: " << e.what() << std::endl; }
  }
  return 0;
}
