#include "cbnetwork/localnetwork.hpp"
#include <iostream>
#include <cmath>
#include <set>
#include <algorithm>
#include <functional>
#include <limits>
#include <stdexcept>
#include <omp.h>
#include <minisat/core/Solver.h>

namespace cbnetwork {

void LocalNetwork::show() const {
    std::cout << "Local Network: " << index << std::endl;
    std::cout << "Internal Variables: [";
    for (size_t i = 0; i < internal_variables.size(); ++i) {
        std::cout << internal_variables[i] << (i == internal_variables.size() - 1 ? "" : ", ");
    }
    std::cout << "]" << std::endl;
}

void LocalNetwork::process_input_signals(const std::vector<std::shared_ptr<DirectedEdge>>& inputs) {
    input_signals = inputs;
    external_variables.clear();
    for (const auto& signal : inputs) {
        external_variables.push_back(signal->index_variable);
    }
    total_variables = internal_variables;
    total_variables.insert(total_variables.end(), external_variables.begin(), external_variables.end());
    total_variables_count = total_variables.size();
}

int LocalNetwork::evaluate_boolean_function(const std::vector<std::vector<int>>& cnf,
                                             const std::map<int, int>& state,
                                             const std::map<int, int>& external_values) {
    for (const auto& clause : cnf) {
        bool clause_satisfied = false;
        for (int literal : clause) {
            int var_index = std::abs(literal);
            bool is_negated = literal < 0;
            int val = 0;
            if (state.count(var_index)) val = state.at(var_index);
            else if (external_values.count(var_index)) val = external_values.at(var_index);
            else continue;

            if (is_negated) val = 1 - val;
            if (val == 1) {
                clause_satisfied = true;
                break;
            }
        }
        if (!clause_satisfied) return 0;
    }
    return 1;
}

std::shared_ptr<LocalNetwork> LocalNetwork::find_local_attractors_brute_force(
    std::shared_ptr<LocalNetwork> local_network,
    const std::vector<std::string>& local_scenes_strings) {

    local_network->local_scenes.clear();
    std::vector<std::string> scenes_to_process = local_scenes_strings;
    if (scenes_to_process.empty()) scenes_to_process.push_back("");

    int network_attractor_count = 0;
    int scene_index = 1;

    for (const auto& scene_str : scenes_to_process) {
        std::map<int, int> external_values;
        for (size_t i = 0; i < scene_str.length(); ++i) {
            if (i < local_network->external_variables.size()) {
                external_values[local_network->external_variables.at(i)] = scene_str.at(i) - '0';
            }
        }

        int num_internal_vars = local_network->internal_variables.size();
        std::map<std::vector<int>, std::vector<int>> state_map;

        for (int i = 0; i < (1 << num_internal_vars); ++i) {
            std::vector<int> current_state_vals;
            std::map<int, int> current_state_dict;
            for (int bit = 0; bit < num_internal_vars; ++bit) {
                int val = (i >> bit) & 1;
                current_state_vals.push_back(val);
                current_state_dict[local_network->internal_variables.at(bit)] = val;
            }

            std::vector<int> next_state_vals;
            for (int var_idx : local_network->internal_variables) {
                int next_val = 0;
                for (const auto& var_model : local_network->descriptive_function_variables) {
                    if (var_model->index == var_idx) {
                        next_val = evaluate_boolean_function(var_model->cnf_function, current_state_dict, external_values);
                        break;
                    }
                }
                next_state_vals.push_back(next_val);
            }
            state_map[current_state_vals] = next_state_vals;
        }

        std::set<std::vector<int>> visited;
        std::vector<std::shared_ptr<LocalAttractor>> scene_attractors;

        for (auto const& [start_node, _] : state_map) {
            if (visited.count(start_node)) continue;

            std::vector<std::vector<int>> path;
            std::vector<int> curr = start_node;
            std::set<std::vector<int>> path_set;

            while (visited.find(curr) == visited.end()) {
                visited.insert(curr);
                path_set.insert(curr);
                path.push_back(curr);
                curr = state_map[curr];

                if (path_set.count(curr)) {
                    auto it = std::find(path.begin(), path.end(), curr);
                    std::vector<std::shared_ptr<LocalState>> l_states;
                    l_states.reserve(std::distance(it, path.end()));
                    for (; it != path.end(); ++it) {
                        l_states.push_back(std::make_shared<LocalState>(*it));
                    }
                    auto attractor = std::make_shared<LocalAttractor>(
                        0, scene_attractors.size() + 1, l_states, local_network->index, local_network->external_variables, scene_str, local_network->variable_names
                    );
                    scene_attractors.push_back(attractor);
                    break;
                }
            }
        }

        auto local_scene_obj = std::make_shared<LocalScene>(scene_index, std::vector<std::string>{scene_str}, local_network->external_variables);
        local_scene_obj->l_attractors = scene_attractors;
        local_network->local_scenes.push_back(local_scene_obj);

        scene_index++;
        network_attractor_count += scene_attractors.size();
    }

    local_network->attractor_count = network_attractor_count;
    return local_network;
}

std::shared_ptr<LocalNetwork> LocalNetwork::find_local_attractors_duvrova(
    std::shared_ptr<LocalNetwork> local_network,
    const std::vector<std::string>& local_scenes_strings) {

    local_network->local_scenes.clear();
    std::vector<std::string> scenes_to_process = local_scenes_strings;
    if (scenes_to_process.empty()) scenes_to_process.push_back("");

    const auto& internal = local_network->internal_variables;
    const auto& external = local_network->external_variables;
    std::vector<int> state_variables = internal;
    state_variables.insert(state_variables.end(), external.begin(), external.end());

    std::map<int, std::shared_ptr<InternalVariable>> functions;
    for (const auto& model : local_network->descriptive_function_variables) {
        if (std::find(internal.begin(), internal.end(), model->index) != internal.end())
            functions[model->index] = model;
    }

    int network_attractor_count = 0;
    int scene_index = 1;
    for (const auto& scene_str : scenes_to_process) {
        std::map<int, int> external_values;
        for (size_t i = 0; i < scene_str.size() && i < external.size(); ++i)
            external_values[external[i]] = scene_str[i] - '0';

        std::vector<std::vector<int>> blocked_states;
        std::vector<std::shared_ptr<LocalAttractor>> scene_attractors;
        size_t horizon = 3;

        while (true) {
            Minisat::Solver solver;
            std::vector<std::vector<Minisat::Var>> variables(
                horizon, std::vector<Minisat::Var>(state_variables.size()));
            for (size_t t = 0; t < horizon; ++t)
                for (size_t i = 0; i < state_variables.size(); ++i)
                    variables[t][i] = solver.newVar();

            auto variable_at = [&](size_t time, int index) {
                auto it = std::find(state_variables.begin(), state_variables.end(), index);
                return variables[time][std::distance(state_variables.begin(), it)];
            };
            auto add_clause = [&](const std::vector<Minisat::Lit>& literals) {
                Minisat::vec<Minisat::Lit> clause;
                for (const auto& literal : literals) clause.push(literal);
                solver.addClause(clause);
            };

            // Inputs are constants throughout a local scenario.
            for (size_t t = 0; t < horizon; ++t) {
                for (int ext : external) {
                    const int value = external_values[ext];
                    add_clause({Minisat::mkLit(variable_at(t, ext), value == 0)});
                }
            }

            // Encode x_i(t+1) <-> f_i(x(t)) in CNF.
            for (size_t t = 0; t + 1 < horizon; ++t) {
                for (int target : internal) {
                    const Minisat::Lit next = Minisat::mkLit(variable_at(t + 1, target));
                    const auto model_it = functions.find(target);
                    if (model_it == functions.end()) {
                        add_clause({~next});
                        continue;
                    }
                    const auto& cnf = model_it->second->cnf_function;

                    // next -> every clause of f.
                    for (const auto& source_clause : cnf) {
                        std::vector<Minisat::Lit> clause{~next};
                        for (int literal : source_clause)
                            clause.push_back(Minisat::mkLit(variable_at(t, std::abs(literal)), literal < 0));
                        add_clause(clause);
                    }

                    // f -> next.  Distribute over one literal selected from
                    // every source clause, as in Duvrova's SAT formulation.
                    std::vector<int> selected;
                    std::function<void(size_t)> add_reverse_clauses = [&](size_t clause_index) {
                        if (clause_index == cnf.size()) {
                            std::vector<Minisat::Lit> clause{next};
                            for (int literal : selected)
                                clause.push_back(Minisat::mkLit(variable_at(t, std::abs(literal)), literal >= 0));
                            add_clause(clause);
                            return;
                        }
                        for (int literal : cnf[clause_index]) {
                            selected.push_back(literal);
                            add_reverse_clauses(clause_index + 1);
                            selected.pop_back();
                        }
                    };
                    add_reverse_clauses(0);
                }
            }

            // Exclude every already discovered cycle state at every instant.
            for (const auto& state : blocked_states) {
                for (size_t t = 0; t < horizon; ++t) {
                    std::vector<Minisat::Lit> clause;
                    for (size_t i = 0; i < internal.size(); ++i)
                        clause.push_back(Minisat::mkLit(variable_at(t, internal[i]), state[i] == 1));
                    add_clause(clause);
                }
            }

            if (!solver.solve()) break;

            std::vector<std::vector<int>> path;
            path.reserve(horizon);
            for (size_t t = 0; t < horizon; ++t) {
                std::vector<int> state;
                state.reserve(internal.size());
                for (int var : internal)
                    state.push_back(solver.modelValue(variable_at(t, var)) == Minisat::lbool(true) ? 1 : 0);
                path.push_back(std::move(state));
            }

            std::map<std::vector<int>, size_t> first_position;
            size_t cycle_begin = path.size();
            size_t cycle_end = path.size();
            for (size_t i = 0; i < path.size(); ++i) {
                auto [it, inserted] = first_position.emplace(path[i], i);
                if (!inserted) {
                    cycle_begin = it->second;
                    cycle_end = i;
                    break;
                }
            }

            if (cycle_begin == path.size()) {
                if (horizon > std::numeric_limits<size_t>::max() / 2)
                    throw std::overflow_error("Duvrova path horizon overflow");
                horizon *= 2;
                continue;
            }

            std::vector<std::shared_ptr<LocalState>> states;
            for (size_t i = cycle_begin; i < cycle_end; ++i)
                states.push_back(std::make_shared<LocalState>(path[i]));
            if (states.empty()) states.push_back(std::make_shared<LocalState>(path[cycle_begin]));

            auto attractor = std::make_shared<LocalAttractor>(
                0, scene_attractors.size() + 1, states, local_network->index,
                external, scene_str, local_network->variable_names);
            scene_attractors.push_back(attractor);
            for (const auto& state : states) blocked_states.push_back(state->l_variable_values);
        }

        auto local_scene = std::make_shared<LocalScene>(
            scene_index++, std::vector<std::string>{scene_str}, external);
        local_scene->l_attractors = scene_attractors;
        local_network->local_scenes.push_back(local_scene);
        network_attractor_count += scene_attractors.size();
    }

    local_network->attractor_count = network_attractor_count;
    return local_network;
}


} // namespace cbnetwork
