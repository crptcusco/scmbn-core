#ifndef LOCALNETWORKTEMPLATE_HPP
#define LOCALNETWORKTEMPLATE_HPP

#include <vector>
#include <map>
#include <memory>
#include <random>
#include <algorithm>
#include "cbnetwork/cnflist.hpp"

namespace cbnetwork {

class LocalNetworkTemplate {
public:
    int v_topology;
    int n_var_network;
    int n_input_variables;
    int n_output_variables;
    int n_max_of_clauses;
    int n_max_of_literals;

    std::vector<int> l_output_var_indexes;
    std::map<int, std::vector<std::vector<int>>> d_variable_cnf_function;

    LocalNetworkTemplate(int vars_net, int in_vars, int out_vars, int max_clauses = 2, int max_literals = 3, int topo = 1)
        : v_topology(topo), n_var_network(vars_net), n_input_variables(in_vars),
          n_output_variables(out_vars), n_max_of_clauses(max_clauses), n_max_of_literals(max_literals) {
        generate_local_dynamic();
    }

    void generate_local_dynamic() {
        std::random_device rd;
        std::mt19937 gen(rd());

        std::vector<int> l_internal_var_indexes;
        for (int i = n_var_network + 1; i <= n_var_network * 2; ++i) {
            l_internal_var_indexes.push_back(i);
        }

        std::vector<int> l_input_coupling_signal_indexes;
        for (int i = n_var_network * 2 + 1; i <= n_var_network * 2 + n_input_variables; ++i) {
            l_input_coupling_signal_indexes.push_back(i);
        }

        std::vector<int> shuffled_internal = l_internal_var_indexes;
        std::shuffle(shuffled_internal.begin(), shuffled_internal.end(), gen);
        std::vector<int> l_input_variables(shuffled_internal.begin(), shuffled_internal.begin() + n_input_variables);

        for (int i_variable : l_internal_var_indexes) {
            int* input_coup_sig_index = nullptr;
            if (std::find(l_input_variables.begin(), l_input_variables.end(), i_variable) != l_input_variables.end()) {
                std::uniform_int_distribution<> d_sig(0, n_input_variables - 1);
                input_coup_sig_index = &l_input_coupling_signal_indexes[d_sig(gen)];
            }

            d_variable_cnf_function[i_variable] = CNFList::generate_cnf(
                l_internal_var_indexes,
                input_coup_sig_index,
                n_max_of_clauses,
                n_max_of_literals
            );
        }

        std::vector<int> output_options;
        for (int i = 1; i <= n_var_network; ++i) output_options.push_back(i);
        std::shuffle(output_options.begin(), output_options.end(), gen);
        l_output_var_indexes.assign(output_options.begin(), output_options.begin() + n_output_variables);
    }
};

} // namespace cbnetwork

#endif // LOCALNETWORKTEMPLATE_HPP
