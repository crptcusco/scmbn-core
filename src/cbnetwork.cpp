#include "cbnetwork/cbnetwork.hpp"
#include "cbnetwork/coupling.hpp"
#include "cbnetwork/customtext.hpp"
#include "cbnetwork/globaltopology.hpp"
#include "cbnetwork/internalvariable.hpp"
#include "cbnetwork/localtemplates.hpp"
#include "nlohmann/json.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <future>
#include <iostream>
#include <omp.h>
#include <random>
#include <set>

namespace cbnetwork {

bool CBN::update_network_by_index(
    std::shared_ptr<LocalNetwork> o_local_network_update) {
  if (!o_local_network_update)
    return false;
  for (size_t i = 0; i < l_local_networks.size(); ++i) {
    if (l_local_networks[i]->index == o_local_network_update->index) {
      l_local_networks[i] = o_local_network_update;
      return true;
    }
  }
  return false;
}

void CBN::process_output_signals() {
  std::map<int, std::shared_ptr<LocalNetwork>> local_network_dict;
  for (auto &net : l_local_networks) {
    if (net) {
      local_network_dict[net->index] = net;
      net->output_signals.clear();
    }
  }

  for (auto &edge : l_directed_edges) {
    if (!edge)
      continue;
    int source_network_index = edge->output_local_network;
    if (local_network_dict.count(source_network_index)) {
      local_network_dict[source_network_index]->output_signals.push_back(edge);
    }
  }
}

void CBN::order_edges_canonically() {
  std::sort(l_directed_edges.begin(), l_directed_edges.end(),
            [](const std::shared_ptr<DirectedEdge> &a,
               const std::shared_ptr<DirectedEdge> &b) {
              if (a->output_local_network != b->output_local_network)
                return a->output_local_network < b->output_local_network;
              if (a->input_local_network != b->input_local_network)
                return a->input_local_network < b->input_local_network;
              return a->index_variable < b->index_variable;
            });
}

void CBN::order_edges_by_compatibility() {
  if (l_directed_edges.empty())
    return;

  order_edges_canonically();

  auto is_compatible =
      [](const std::vector<std::shared_ptr<DirectedEdge>> &l_group_base,
         std::shared_ptr<DirectedEdge> o_group) {
        for (const auto &aux_par : l_group_base) {
          if (aux_par->input_local_network == o_group->input_local_network ||
              aux_par->input_local_network == o_group->output_local_network ||
              aux_par->output_local_network == o_group->output_local_network ||
              aux_par->output_local_network == o_group->input_local_network) {
            return true;
          }
        }
        return false;
      };

  std::vector<std::shared_ptr<DirectedEdge>> l_base = {l_directed_edges[0]};
  std::vector<std::shared_ptr<DirectedEdge>> aux_l_rest_groups(
      l_directed_edges.begin() + 1, l_directed_edges.end());

  while (!aux_l_rest_groups.empty()) {
    bool found = false;
    for (auto it = aux_l_rest_groups.begin(); it != aux_l_rest_groups.end();
         ++it) {
      if (is_compatible(l_base, *it)) {
        l_base.push_back(*it);
        aux_l_rest_groups.erase(it);
        found = true;
        break;
      }
    }
    if (!found) {
      for (auto &e : aux_l_rest_groups)
        l_base.push_back(e);
      aux_l_rest_groups.clear();
    }
  }
  l_directed_edges = l_base;
}

void CBN::order_edges_by_grade() {
  std::map<int, int> network_degrees;
  for (const auto &net : l_local_networks)
    network_degrees[net->index] = 0;
  for (const auto &edge : l_directed_edges) {
    network_degrees[edge->input_local_network]++;
    network_degrees[edge->output_local_network]++;
  }

  auto calculate_edge_grade = [&](std::shared_ptr<DirectedEdge> edge) {
    return network_degrees[edge->input_local_network] +
           network_degrees[edge->output_local_network];
  };

  std::sort(
      l_directed_edges.begin(), l_directed_edges.end(),
      [&](std::shared_ptr<DirectedEdge> a, std::shared_ptr<DirectedEdge> b) {
        return calculate_edge_grade(a) > calculate_edge_grade(b);
      });

  auto is_adjacent = [](std::shared_ptr<DirectedEdge> edge1,
                        std::shared_ptr<DirectedEdge> edge2) {
    return edge1->input_local_network == edge2->input_local_network ||
           edge1->input_local_network == edge2->output_local_network ||
           edge1->output_local_network == edge2->output_local_network ||
           edge1->output_local_network == edge2->input_local_network;
  };

  if (l_directed_edges.empty())
    return;

  std::vector<std::shared_ptr<DirectedEdge>> ordered_edges = {
      l_directed_edges[0]};
  l_directed_edges.erase(l_directed_edges.begin());

  while (!l_directed_edges.empty()) {
    bool found = false;
    for (auto it = l_directed_edges.begin(); it != l_directed_edges.end();
         ++it) {
      if (is_adjacent(ordered_edges.back(), *it)) {
        ordered_edges.push_back(*it);
        l_directed_edges.erase(it);
        found = true;
        break;
      }
    }
    if (!found) {
      ordered_edges.push_back(l_directed_edges[0]);
      l_directed_edges.erase(l_directed_edges.begin());
    }
  }
  l_directed_edges = ordered_edges;
}

void CBN::disorder_edges() {
  if (l_directed_edges.size() < 2)
    return;
  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(l_directed_edges.begin(), l_directed_edges.end(), g);

  auto have_common_vertex = [](std::shared_ptr<DirectedEdge> edge1,
                               std::shared_ptr<DirectedEdge> edge2) {
    return edge1->input_local_network == edge2->input_local_network ||
           edge1->input_local_network == edge2->output_local_network ||
           edge1->output_local_network == edge2->output_local_network ||
           edge1->output_local_network == edge2->input_local_network;
  };

  if (have_common_vertex(l_directed_edges[0], l_directed_edges[1])) {
    for (size_t i = 2; i < l_directed_edges.size(); ++i) {
      if (!have_common_vertex(l_directed_edges[0], l_directed_edges[i])) {
        std::swap(l_directed_edges[1], l_directed_edges[i]);
        break;
      }
    }
  }
}

std::vector<std::string>
CBN::_generate_local_scenes(std::shared_ptr<LocalNetwork> o_local_network) {
  if (!o_local_network)
    return {};
  int external_vars_count = o_local_network->external_variables.size();
  std::vector<std::string> scenes;
  if (external_vars_count > 0) {
    for (int i = 0; i < (1 << external_vars_count); ++i) {
      std::string scene = "";
      for (int bit = 0; bit < external_vars_count; ++bit) {
        scene += ((i >> (external_vars_count - 1 - bit)) & 1) ? "1" : "0";
      }
      scenes.push_back(scene);
    }
  } else {
    scenes.push_back("");
  }
  return scenes;
}

void CBN::find_local_attractors() {
#ifdef _OPENMP
  find_local_attractors_parallel_with_weights();
#else
  find_local_attractors_sequential();
#endif
}

void CBN::find_compatible_pairs_sequential() {
  for (auto &edge : l_directed_edges) {
    for (int val : {0, 1}) {
      edge->d_comp_pairs_attractors_by_value[val].clear();
      auto dst_attractors =
          get_attractors_by_input_signal_value(edge->index_variable, val);
      auto &src_attractors = edge->d_out_value_to_attractor[val];
      edge->d_comp_pairs_attractors_by_value[val].reserve(
          src_attractors.size() * dst_attractors.size());
      for (auto &src_attr : src_attractors) {
        for (auto &dst_attr : dst_attractors) {
          edge->d_comp_pairs_attractors_by_value[val].push_back(
              {src_attr->g_index, dst_attr->g_index});
        }
      }
    }
  }
}

void CBN::find_compatible_pairs() {
#ifdef _OPENMP
  find_compatible_pairs_parallel_with_weights();
#else
  find_compatible_pairs_sequential();
#endif
}

void CBN::mount_attractor_fields() { mount_stable_attractor_fields(); }

void CBN::find_local_attractors_sequential() {
  for (auto &net : l_local_networks) {
    auto scenes = _generate_local_scenes(net);
    LocalNetwork::find_local_attractors_duvrova(net, scenes);
    process_kind_signal(net);
  }
  _assign_global_indices_to_attractors();
  generate_attractor_dictionary();
}

void CBN::find_local_attractors_parallel() {
  find_local_attractors_parallel_simple();
}

void CBN::find_local_attractors_parallel_simple() {
#pragma omp parallel for schedule(dynamic)
  for (int i = 0; i < (int)l_local_networks.size(); ++i) {
    auto &net = l_local_networks[i];
    auto scenes = _generate_local_scenes(net);
    LocalNetwork::find_local_attractors_duvrova(net, scenes);
    process_kind_signal(net);
  }
  _assign_global_indices_to_attractors();
  generate_attractor_dictionary();
}

void CBN::find_local_attractors_parallel_with_weights() {
  struct WeightedNet {
    long weight;
    std::shared_ptr<LocalNetwork> net;
  };
  std::vector<WeightedNet> weighted_nets;
  for (auto &net : l_local_networks) {
    long w = (long)net->internal_variables.size() *
             (1L << net->external_variables.size());
    weighted_nets.push_back({w, net});
  }
  std::sort(weighted_nets.begin(), weighted_nets.end(),
            [](const auto &a, const auto &b) { return a.weight > b.weight; });

  int num_threads = omp_get_max_threads();
  std::vector<std::vector<std::shared_ptr<LocalNetwork>>> buckets(num_threads);
  std::vector<long> bucket_weights(num_threads, 0);
  for (auto &wn : weighted_nets) {
    int best_idx = 0;
    long min_w = bucket_weights[0];
    for (int i = 1; i < num_threads; ++i) {
      if (bucket_weights[i] < min_w) {
        min_w = bucket_weights[i];
        best_idx = i;
      }
    }
    buckets[best_idx].push_back(wn.net);
    bucket_weights[best_idx] += wn.weight;
  }

#pragma omp parallel
  {
    int tid = omp_get_thread_num();
    if (tid < (int)buckets.size()) {
      for (auto &net : buckets[tid]) {
        auto scenes = _generate_local_scenes(net);
        LocalNetwork::find_local_attractors_duvrova(net, scenes);
        process_kind_signal(net);
      }
    }
  }
  _assign_global_indices_to_attractors();
  generate_attractor_dictionary();
}

void CBN::generate_attractor_dictionary() {
  d_local_attractors.clear();
  d_local_attractors_ptr.clear();
  for (auto &net : l_local_networks) {
    for (auto &scene : net->local_scenes) {
      for (auto &attractor : scene->l_attractors) {
        d_local_attractors[attractor->g_index] = {net->index, scene->index,
                                                  attractor->l_index};
        d_local_attractors_ptr[attractor->g_index] = attractor;
      }
    }
  }
}

void CBN::process_kind_signal(std::shared_ptr<LocalNetwork> o_local_network) {
  if (!o_local_network)
    return;
  for (auto &edge : l_directed_edges) {
    if (!edge || edge->output_local_network != o_local_network->index)
      continue;

    edge->d_out_value_to_attractor[0].clear();
    edge->d_out_value_to_attractor[1].clear();

    std::set<int> signals_for_output;
    for (auto &scene : o_local_network->local_scenes) {
      std::set<int> signals_in_local_scene;
      for (auto &attractor : scene->l_attractors) {
        std::set<char> signals_in_attractor;
        for (auto &state : attractor->l_states) {
          std::string tt_index = "";
          for (int out_var : edge->l_output_variables) {
            auto it =
                std::find(o_local_network->total_variables.begin(),
                          o_local_network->total_variables.end(), out_var);
            if (it != o_local_network->total_variables.end()) {
              int pos =
                  std::distance(o_local_network->total_variables.begin(), it);
              tt_index += std::to_string(state->l_variable_values.at(pos));
            }
          }
          if (edge->true_table.count(tt_index))
            signals_in_attractor.insert(edge->true_table.at(tt_index).at(0));
        }

        if (signals_in_attractor.size() == 1) {
          int val = *signals_in_attractor.begin() - '0';
          signals_in_local_scene.insert(val);
          edge->d_out_value_to_attractor[val].push_back(attractor);
        } else {
          signals_in_local_scene.insert(-1);
        }
      }
      for (int s : signals_in_local_scene)
        signals_for_output.insert(s);
    }

    if (signals_for_output.size() == 1 && *signals_for_output.begin() != -1)
      edge->kind_signal = 1;
    else if (signals_for_output.count(-1))
      edge->kind_signal = 4;
    else if (signals_for_output.size() == 2)
      edge->kind_signal = 3;
  }
}

std::vector<std::shared_ptr<LocalAttractor>>
CBN::get_attractors_by_input_signal_value(int index_variable_signal,
                                          int signal_value) {
  std::vector<std::shared_ptr<LocalAttractor>> l_attractors;
  for (auto &net : l_local_networks) {
    for (auto &scene : net->local_scenes) {
      auto it = std::find(scene->l_index_signals.begin(),
                          scene->l_index_signals.end(), index_variable_signal);
      if (it != scene->l_index_signals.end()) {
        int pos = std::distance(scene->l_index_signals.begin(), it);
        for (const auto &val_str : scene->l_values) {
          if (pos < val_str.length() &&
              val_str.at(pos) == (signal_value + '0')) {
            for (auto &attr : scene->l_attractors)
              l_attractors.push_back(attr);
            break;
          }
        }
      }
    }
  }
  return l_attractors;
}

std::shared_ptr<LocalAttractor>
CBN::get_local_attractor_by_index(int i_attractor) {
  if (d_local_attractors_ptr.count(i_attractor))
    return d_local_attractors_ptr.at(i_attractor);
  return nullptr;
}

std::shared_ptr<LocalNetwork> CBN::get_network_by_index(int index) {
  for (auto &net : l_local_networks)
    if (net->index == index)
      return net;
  return nullptr;
}

void CBN::find_compatible_pairs_parallel() {
#pragma omp parallel for
  for (int i = 0; i < (int)l_directed_edges.size(); ++i) {
    auto &edge = l_directed_edges.at(i);
    if (!edge)
      continue;
    for (int val : {0, 1}) {
      edge->d_comp_pairs_attractors_by_value[val].clear();
      auto dst_attractors =
          get_attractors_by_input_signal_value(edge->index_variable, val);

      auto &src_attractors = edge->d_out_value_to_attractor[val];
      edge->d_comp_pairs_attractors_by_value[val].reserve(
          src_attractors.size() * dst_attractors.size());

      for (auto &src_attr : src_attractors) {
        if (!src_attr)
          continue;
        for (auto &dst_attr : dst_attractors) {
          if (!dst_attr)
            continue;
          edge->d_comp_pairs_attractors_by_value[val].push_back(
              {src_attr->g_index, dst_attr->g_index});
        }
      }
    }
  }
}

void CBN::find_compatible_pairs_parallel_with_weights() {
  find_compatible_pairs_parallel();
}

void CBN::mount_stable_attractor_fields_parallel() {
  mount_stable_attractor_fields();
}

void CBN::mount_stable_attractor_fields_parallel_chunks() {
  mount_stable_attractor_fields();
}

void CBN::mount_stable_attractor_fields() {
  if (l_directed_edges.empty()) {
    d_attractor_fields.clear();
    return;
  }

  // Step 1: Order edges by compatibility
  order_edges_by_compatibility();

  // Mapping from attractor to network index for fast compatibility checks
  int max_attr_idx = 0;
  for (auto const &[idx, _] : d_local_attractors) {
    max_attr_idx = std::max(max_attr_idx, idx);
  }
  std::vector<int> attr_to_network(max_attr_idx + 1, -1);
  for (auto const &[idx, info] : d_local_attractors) {
    attr_to_network[idx] = std::get<0>(info);
  }

  std::vector<std::vector<int>> current_fields;

  // Step 2: Initialize base with unique pairs from the first edge
  auto first_edge = l_directed_edges[0];
  std::set<std::vector<int>> unique_initial;
  for (int val : {0, 1}) {
    for (auto &p : first_edge->d_comp_pairs_attractors_by_value[val]) {
      std::vector<int> f = {p.first, p.second};
      std::sort(f.begin(), f.end());
      unique_initial.insert(f);
    }
  }
  for (auto &f : unique_initial)
    current_fields.push_back(f);

  // Step 3: Iteratively refine with remaining edges using Cartesian product
  for (size_t i = 1; i < l_directed_edges.size(); ++i) {
    std::set<std::vector<int>> unique_candidates;
    for (int val : {0, 1}) {
      for (auto &p :
           l_directed_edges[i]->d_comp_pairs_attractors_by_value[val]) {
        std::vector<int> f = {p.first, p.second};
        std::sort(f.begin(), f.end());
        unique_candidates.insert(f);
      }
    }
    if (unique_candidates.empty() || current_fields.empty()) {
      current_fields.clear();
      break;
    }

    std::set<std::vector<int>> unique_next_fields;
    for (const auto &field : current_fields) {
      // Optimization: Pre-build net_to_attr map for the current field
      std::map<int, int> field_net_to_attr;
      for (int attr : field)
        field_net_to_attr[attr_to_network[attr]] = attr;

      for (const auto &cand : unique_candidates) {
        // Compatibility check: Attractor consistency across local networks
        bool compatible = true;
        for (int attr : cand) {
          int net = attr_to_network[attr];
          if (field_net_to_attr.count(net) && field_net_to_attr[net] != attr) {
            compatible = false;
            break;
          }
        }

        if (compatible) {
          std::vector<int> new_f = field;
          new_f.insert(new_f.end(), cand.begin(), cand.end());
          std::sort(new_f.begin(), new_f.end());
          new_f.erase(std::unique(new_f.begin(), new_f.end()), new_f.end());
          unique_next_fields.insert(new_f);
        }
      }
    }
    current_fields.clear();
    for (auto &f : unique_next_fields)
      current_fields.push_back(f);
    if (current_fields.empty()) {
      break;
    }
  }

  // Final step: Populate the attractor fields dictionary
  d_attractor_fields.clear();
  for (size_t i = 0; i < current_fields.size(); ++i) {
    d_attractor_fields[i + 1] = current_fields[i];
  }
}

std::vector<std::vector<int8_t>> CBN::filter_compatible_pairs_kernel(
    const std::vector<std::vector<int>> &fields,
    const std::vector<std::vector<int>> &candidates,
    const std::vector<int> &attr_to_network) {

  size_t n_fields = fields.size();
  size_t n_cands = candidates.size();
  std::vector<std::vector<int8_t>> result(n_fields,
                                          std::vector<int8_t>(n_cands, 0));

#pragma omp parallel for
  for (int i = 0; i < (int)n_fields; ++i) {
    for (int j = 0; j < (int)n_cands; ++j) {
      bool compatible = true;
      std::map<int, int> net_to_attr;
      for (int attr : fields[i]) {
        net_to_attr[attr_to_network[attr]] = attr;
      }
      for (int attr : candidates[j]) {
        int net = attr_to_network[attr];
        if (net_to_attr.count(net) && net_to_attr[net] != attr) {
          compatible = false;
          break;
        }
      }
      if (compatible)
        result[i][j] = 1;
    }
  }
  return result;
}

std::shared_ptr<CBN>
CBN::cbn_generator(int v_topology, int n_networks, int n_var_network,
                   int n_input_variables, int n_output_variables,
                   int n_max_of_clauses, int n_max_of_literals, int n_edges,
                   std::shared_ptr<CouplingStrategy> coupling_strategy) {

  auto o_topo =
      GlobalTopology::generate_sample_topology(v_topology, n_networks, n_edges);
  LocalNetworkTemplate o_template(n_var_network, n_input_variables,
                                  n_output_variables, n_max_of_clauses,
                                  n_max_of_literals, v_topology);

  std::vector<std::shared_ptr<LocalNetwork>> networks;
  int var_cont = 1;
  for (int i = 1; i <= n_networks; ++i) {
    std::vector<int> ivars;
    for (int j = 0; j < n_var_network; ++j)
      ivars.push_back(var_cont++);
    networks.push_back(std::make_shared<LocalNetwork>(i, ivars));
  }

  std::vector<std::shared_ptr<DirectedEdge>> edges;
  int i_last_variable = var_cont;
  int i_directed_edge = 1;

  for (auto &rel : o_topo->l_edges) {
    int out_net_idx = rel.first;
    int in_net_idx = rel.second;

    auto &out_net = networks.at(out_net_idx - 1);
    std::vector<int> out_vars;
    for (int pos : o_template.l_output_var_indexes) {
      out_vars.push_back(out_net->internal_variables.at(pos - 1));
    }

    std::string coup_func =
        coupling_strategy->generate_coupling_function(out_vars);
    auto edge = std::make_shared<DirectedEdge>(
        i_directed_edge++, i_last_variable, in_net_idx, out_net_idx, out_vars,
        coup_func);

    auto coupling_cnf = coupling_strategy->to_cnf(out_vars, i_last_variable);
    auto coupling_variable =
        std::make_shared<InternalVariable>(i_last_variable, coupling_cnf);

    if (std::find(out_net->internal_variables.begin(),
                  out_net->internal_variables.end(),
                  i_last_variable) == out_net->internal_variables.end()) {
      out_net->internal_variables.push_back(i_last_variable);
    }
    out_net->descriptive_function_variables.push_back(coupling_variable);

    i_last_variable++;
    edges.push_back(edge);
  }

  for (auto &net : networks) {
    auto inputs = find_input_edges_by_network_index(net->index, edges);
    net->process_input_signals(inputs);

    for (int ivar : net->internal_variables) {
      int tvar = ivar - ((net->index - 1) * n_var_network);
      if (tvar < 1 || tvar > n_var_network)
        continue;

      auto it_cnf =
          o_template.d_variable_cnf_function.find(tvar + n_var_network);
      if (it_cnf == o_template.d_variable_cnf_function.end())
        continue;

      const auto &pre_cnf = it_cnf->second;
      std::vector<std::vector<int>> cnf;
      for (auto &pc : pre_cnf) {
        std::vector<int> clause;
        for (int v : pc) {
          int abs_v = std::abs(v);
          int lval;
          if (abs_v >= n_var_network + 1 && abs_v <= n_var_network * 2) {
            lval = (abs_v - n_var_network) + (net->index - 1) * n_var_network;
          } else if (abs_v > n_var_network * 2) {
            int signal_idx = abs_v - (n_var_network * 2) - 1;
            if (signal_idx < (int)net->input_signals.size()) {
              lval = net->input_signals[signal_idx]->index_variable;
            } else {
              lval = net->internal_variables[0];
            }
          } else {
            lval = abs_v;
          }
          clause.push_back(v < 0 ? -lval : lval);
        }
        cnf.push_back(clause);
      }
      net->descriptive_function_variables.push_back(
          std::make_shared<InternalVariable>(ivar, cnf));
    }
  }
  auto cbn = std::make_shared<CBN>(networks, edges);
  cbn->o_global_topology = o_topo;
  cbn->audit_indices();
  return cbn;
}

void CBN::show_resume() const {
  CustomText::make_title("CBN Detailed Resume");
  CustomText::make_sub_sub_title("Main Characteristics");
  CustomText::print_simple_line();
  std::cout << "Number of local networks: " << l_local_networks.size()
            << std::endl;
  std::cout << "Topology Type: "
            << (o_global_topology
                    ? std::to_string(o_global_topology->v_topology)
                    : "N/A")
            << std::endl;

  CustomText::make_sub_sub_title("Indicators");
  CustomText::print_simple_line();
  std::cout << "Number of local attractors: " << get_n_local_attractors()
            << std::endl;
  std::cout << "Number of attractor fields: " << get_n_attractor_fields()
            << std::endl;
  CustomText::print_simple_line();
}

void CBN::show_description() const {
  CustomText::make_title("CBN description");
  std::string nets = "Local Networks: [";
  for (size_t i = 0; i < l_local_networks.size(); ++i)
    nets += std::to_string(l_local_networks[i]->index) +
            (i == l_local_networks.size() - 1 ? "" : ", ");
  nets += "]";
  CustomText::make_sub_title(nets);
  for (auto &net : l_local_networks)
    if (net)
      net->show();
}

void CBN::show_global_scenes() const {
  CustomText::make_sub_title("LIST OF GLOBAL SCENES");
  for (auto &scene : l_global_scenes)
    if (scene)
      scene->show();
}

void CBN::show_stable_attractor_fields() const {
  CustomText::print_duplex_line();
  std::cout << "Show the list of attractor fields" << std::endl;
  std::cout << "Number Stable Attractor Fields: " << d_attractor_fields.size()
            << std::endl;
  for (auto const &[key, field] : d_attractor_fields) {
    CustomText::print_simple_line();
    std::cout << key << ": [";
    for (size_t i = 0; i < field.size(); ++i) {
      std::cout << field.at(i) << (i == field.size() - 1 ? "" : ", ");
    }
    std::cout << "]" << std::endl;
  }
}

void CBN::show_local_attractors_dictionary() const {
  CustomText::make_title("Global Dictionary of Local Attractors");
  for (auto const &[key, val] : d_local_attractors) {
    std::cout << key << " -> (" << std::get<0>(val) << ", " << std::get<1>(val)
              << ", " << std::get<2>(val) << ")" << std::endl;
  }
}

void CBN::show_local_attractors() const {
  CustomText::make_title("SHOW LOCAL ATTRACTORS");
  for (auto &net : l_local_networks) {
    net->show();
    for (auto &scene : net->local_scenes) {
      for (auto &attr : scene->l_attractors)
        attr->show();
    }
  }
}

void CBN::show_attractor_pairs() const {
  CustomText::make_title("SHOW ATTRACTOR PAIRS");
  for (auto &edge : l_directed_edges) {
    std::cout << "Edge " << edge->index << ": " << edge->output_local_network
              << " -> " << edge->input_local_network << std::endl;
    for (int val : {0, 1}) {
      if (edge->d_comp_pairs_attractors_by_value.count(val)) {
        for (auto &p : edge->d_comp_pairs_attractors_by_value.at(val)) {
          std::cout << "  (" << p.first << ", " << p.second << ")" << std::endl;
        }
      }
    }
  }
}

void CBN::show_coupled_signals_kind() const {
  CustomText::print_duplex_line();
  std::cout << "SHOW THE COUPLED SIGNALS KINDS" << std::endl;
  int n_restricted = 0;
  for (auto &edge : l_directed_edges) {
    if (!edge)
      continue;
    std::cout << "SIGNAL: " << edge->index_variable
              << ", RELATION: " << edge->output_local_network << " -> "
              << edge->input_local_network << ", KIND: " << edge->kind_signal
              << std::endl;
    if (edge->kind_signal == 1)
      n_restricted++;
  }
  std::cout << "Number of restricted signals: " << n_restricted << std::endl;
}

void CBN::show_stable_attractor_fields_detailed() const {
  show_stable_attractor_fields();
  for (auto const &[id, field] : d_attractor_fields) {
    std::cout << "Field " << id << ": ";
    for (int a : field) {
      if (d_local_attractors.count(a)) {
        auto val = d_local_attractors.at(a);
        std::cout << a << " -> (" << std::get<0>(val) << ", "
                  << std::get<1>(val) << ", " << std::get<2>(val) << ") ";
      }
    }
    std::cout << std::endl;
  }
}

void CBN::show_attractor_fields() const { show_stable_attractor_fields(); }

void CBN::show_directed_edges() const {
  CustomText::print_duplex_line();
  std::cout << "SHOW THE DIRECTED EDGES OF THE CBN" << std::endl;
  for (auto &edge : l_directed_edges) {
    if (edge)
      edge->show();
  }
}

void CBN::audit_indices() const {
  std::set<int> net_indices;
  std::set<int> all_internal_vars;

  for (const auto &net : l_local_networks) {
    if (net_indices.count(net->index)) {
      throw std::runtime_error("Duplicate network index detected: " +
                               std::to_string(net->index));
    }
    net_indices.insert(net->index);

    for (int v : net->internal_variables) {
      if (all_internal_vars.count(v)) {
        throw std::runtime_error(
            "Duplicate internal variable index detected: " + std::to_string(v));
      }
      all_internal_vars.insert(v);
    }
  }

  for (const auto &edge : l_directed_edges) {
    if (net_indices.find(edge->output_local_network) == net_indices.end()) {
      throw std::runtime_error(
          "Edge " + std::to_string(edge->index) + ": Output network " +
          std::to_string(edge->output_local_network) + " not found.");
    }
    if (net_indices.find(edge->input_local_network) == net_indices.end()) {
      throw std::runtime_error(
          "Edge " + std::to_string(edge->index) + ": Input network " +
          std::to_string(edge->input_local_network) + " not found.");
    }
  }

  for (const auto &net : l_local_networks) {
    std::set<int> net_vars(net->internal_variables.begin(),
                           net->internal_variables.end());
    net_vars.insert(net->external_variables.begin(),
                    net->external_variables.end());

    for (const auto &var : net->descriptive_function_variables) {
      for (const auto &clause : var->cnf_function) {
        for (int lit : clause) {
          int abs_lit = std::abs(lit);
          if (net_vars.find(abs_lit) == net_vars.end()) {
            bool is_input_signal = false;
            for (const auto &edge : l_directed_edges) {
              if (edge->input_local_network == net->index &&
                  edge->index_variable == abs_lit) {
                is_input_signal = true;
                break;
              }
            }
            if (!is_input_signal && abs_lit < 1000) {
            }
          }
        }
      }
    }
  }
}

void CBN::save_attractor_fields_to_json(const std::string &filepath) {
  using json = nlohmann::json;
  json j_fields = json::array();
  for (auto const &[id, field] : d_attractor_fields) {
    json j_field;
    j_field["field_id"] = id;
    j_field["attractor_indices"] = field;
    j_fields.push_back(j_field);
  }
  std::ofstream out(filepath);
  out << j_fields.dump(4) << std::endl;
}

void CBN::clear_dynamics() {
  d_local_attractors.clear();
  d_local_attractors_ptr.clear();
  d_attractor_fields.clear();
  l_global_scenes.clear();
  d_global_scenes_count.clear();

  for (auto &net : l_local_networks) {
    if (net) {
      net->local_scenes.clear();
      net->attractor_count = 0;
    }
  }

  for (auto &edge : l_directed_edges) {
    if (edge) {
      edge->d_out_value_to_attractor[0].clear();
      edge->d_out_value_to_attractor[1].clear();
      edge->d_comp_pairs_attractors_by_value[0].clear();
      edge->d_comp_pairs_attractors_by_value[1].clear();
      edge->kind_signal = 2; // NOT COMPUTE
    }
  }
}

std::shared_ptr<CBN> CBN::clone() const {
  std::vector<std::shared_ptr<LocalNetwork>> cloned_networks;
  std::map<int, std::shared_ptr<LocalNetwork>> net_map;
  for (const auto &net : l_local_networks) {
    auto cloned_net =
        std::make_shared<LocalNetwork>(net->index, net->internal_variables);
    cloned_net->variable_names = net->variable_names;
    cloned_net->descriptive_function_variables =
        net->descriptive_function_variables;
    cloned_networks.push_back(cloned_net);
    net_map[cloned_net->index] = cloned_net;
  }

  std::vector<std::shared_ptr<DirectedEdge>> cloned_edges;
  for (const auto &edge : l_directed_edges) {
    auto cloned_edge = std::make_shared<DirectedEdge>(
        edge->index, edge->index_variable, edge->input_local_network,
        edge->output_local_network, edge->l_output_variables,
        edge->coupling_function);
    cloned_edge->true_table = edge->true_table;
    cloned_edges.push_back(cloned_edge);
  }

  auto cloned_cbn = std::make_shared<CBN>(cloned_networks, cloned_edges);
  cloned_cbn->o_global_topology = o_global_topology;

  for (auto &net : cloned_cbn->l_local_networks) {
    std::vector<std::shared_ptr<DirectedEdge>> inputs;
    for (auto &edge : cloned_cbn->l_directed_edges) {
      if (edge->input_local_network == net->index)
        inputs.push_back(edge);
    }
    net->process_input_signals(inputs);
  }
  cloned_cbn->process_output_signals();

  return cloned_cbn;
}

void CBN::save_network_to_json(const std::string &filepath) const {
  using json = nlohmann::json;
  json j_cbn;

  json j_nets = json::array();
  for (const auto &net : l_local_networks) {
    json j_net;
    j_net["index"] = net->index;
    j_net["internal_variables"] = net->internal_variables;
    j_net["variable_names"] = net->variable_names;

    json j_vars = json::array();
    for (const auto &var : net->descriptive_function_variables) {
      json j_var;
      j_var["index"] = var->index;
      j_var["cnf"] = var->cnf_function;
      j_vars.push_back(j_var);
    }
    j_net["logic"] = j_vars;
    j_nets.push_back(j_net);
  }
  j_cbn["local_networks"] = j_nets;

  json j_edges = json::array();
  for (const auto &edge : l_directed_edges) {
    json j_edge;
    j_edge["index"] = edge->index;
    j_edge["signal_var"] = edge->index_variable;
    j_edge["source"] = edge->output_local_network;
    j_edge["target"] = edge->input_local_network;
    j_edge["output_vars"] = edge->l_output_variables;
    j_edge["function"] = edge->coupling_function;
    j_edges.push_back(j_edge);
  }
  j_cbn["directed_edges"] = j_edges;

  std::ofstream out(filepath);
  out << j_cbn.dump(4) << std::endl;
}

double CBN::calculate_canalization_density() const {
  int total_internal_vars = 0;
  std::set<int> used_vars;

  for (const auto &net : l_local_networks) {
    total_internal_vars += net->internal_variables.size();
    for (const auto &var : net->descriptive_function_variables) {
      for (const auto &clause : var->cnf_function) {
        for (int literal : clause) {
          used_vars.insert(std::abs(literal));
        }
      }
    }
  }

  if (total_internal_vars == 0)
    return 0.0;

  int redundant_vars = 0;
  for (const auto &net : l_local_networks) {
    for (int ivar : net->internal_variables) {
      if (used_vars.find(ivar) == used_vars.end()) {
        redundant_vars++;
      }
    }
  }

  return (double)redundant_vars / total_internal_vars;
}

std::vector<std::shared_ptr<DirectedEdge>>
CBN::find_output_edges_by_network_index(
    int index, const std::vector<std::shared_ptr<DirectedEdge>> &edges) {
  std::vector<std::shared_ptr<DirectedEdge>> result;
  for (auto &e : edges)
    if (e->output_local_network == index)
      result.push_back(e);
  return result;
}

std::vector<std::shared_ptr<DirectedEdge>>
CBN::find_input_edges_by_network_index(
    int index, const std::vector<std::shared_ptr<DirectedEdge>> &edges) {
  std::vector<std::shared_ptr<DirectedEdge>> result;
  for (auto &e : edges)
    if (e->input_local_network == index)
      result.push_back(e);
  return result;
}

size_t CBN::get_n_local_attractors() const {
  size_t count = 0;
  for (const auto &net : l_local_networks)
    count += net->attractor_count;
  return count;
}

size_t CBN::get_n_pair_attractors() const {
  size_t count = 0;
  for (const auto &edge : l_directed_edges) {
    count += edge->d_comp_pairs_attractors_by_value[0].size() +
             edge->d_comp_pairs_attractors_by_value[1].size();
  }
  return count;
}

size_t CBN::get_n_attractor_fields() const { return d_attractor_fields.size(); }

void CBN::_assign_global_indices_to_attractors() {
  int g_index = 1;
  for (auto &net : l_local_networks) {
    for (auto &scene : net->local_scenes) {
      for (auto &attr : scene->l_attractors) {
        attr->g_index = g_index++;
      }
    }
  }
}

int CBN::get_n_local_variables() const {
  if (l_local_networks.empty())
    return 0;
  return l_local_networks[0]->internal_variables.size();
}

std::shared_ptr<CBN> CBN::load_network_from_json(const std::string &filepath) {
  using json = nlohmann::json;
  std::ifstream i(filepath);
  if (!i.is_open())
    throw std::runtime_error("Could not open file: " + filepath);
  json j;
  i >> j;
  std::vector<std::shared_ptr<LocalNetwork>> networks;
  if (j.contains("local_networks")) {
    for (auto &net_j : j["local_networks"]) {
      int idx = net_j["index"];
      std::vector<int> internal_vars =
          net_j["internal_variables"].get<std::vector<int>>();
      auto net = std::make_shared<LocalNetwork>(idx, internal_vars);
      if (net_j.contains("variable_names")) {
        net->variable_names = net_j["variable_names"].get<std::vector<std::string>>();
      }
      std::string logic_key = net_j.contains("descriptive_function_variables")
                                  ? "descriptive_function_variables"
                                  : "logic";
      for (auto &var_j : net_j[logic_key]) {
        int v_idx = var_j["index"];
        std::vector<std::vector<int>> cnf =
            var_j["cnf"].get<std::vector<std::vector<int>>>();
        net->descriptive_function_variables.push_back(
            std::make_shared<InternalVariable>(v_idx, cnf));
      }
      networks.push_back(net);
    }
  }
  std::vector<std::shared_ptr<DirectedEdge>> edges;
  if (j.contains("directed_edges")) {
    for (auto &edge_j : j["directed_edges"]) {
      int idx = edge_j["index"];
      int idx_var = edge_j.contains("index_variable")
                        ? (int)edge_j["index_variable"]
                        : (int)edge_j["signal_var"];
      int out_net = edge_j.contains("output_local_network")
                        ? (int)edge_j["output_local_network"]
                        : (int)edge_j["source"];
      int in_net = edge_j.contains("input_local_network")
                       ? (int)edge_j["input_local_network"]
                       : (int)edge_j["target"];
      std::vector<int> out_vars =
          edge_j.contains("output_variables")
              ? edge_j["output_variables"].get<std::vector<int>>()
          : edge_j.contains("output_vars")
              ? edge_j["output_vars"].get<std::vector<int>>()
              : std::vector<int>();
      std::string coup_func =
          edge_j.contains("coupling_function")
              ? edge_j["coupling_function"].get<std::string>()
          : edge_j.contains("function") ? edge_j["function"].get<std::string>()
                                        : "";
      auto edge = std::make_shared<DirectedEdge>(idx, idx_var, in_net, out_net,
                                                 out_vars, coup_func);
      if (edge_j.contains("true_table"))
        edge->true_table =
            edge_j["true_table"].get<std::map<std::string, std::string>>();
      edges.push_back(edge);
    }
  }
  for (auto &net : networks) {
    std::vector<std::shared_ptr<DirectedEdge>> inputs;
    for (auto &edge : edges)
      if (edge->input_local_network == net->index)
        inputs.push_back(edge);
    net->process_input_signals(inputs);
  }
  auto cbn = std::make_shared<CBN>(networks, edges);
  cbn->process_output_signals();
  return cbn;
}

} // namespace cbnetwork
