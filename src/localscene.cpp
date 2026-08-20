#include "cbnetwork/localscene.hpp"
#include <iostream>

namespace cbnetwork {

void LocalAttractor::show() const {
    std::cout << "Network Index: " << network_index << ", Scene: " << local_scene
              << ", Global Index: " << g_index << ", Local Index: " << l_index << ", States: ";
    for (auto& state : l_states) {
        std::cout << "[";
        for (size_t i = 0; i < state->l_variable_values.size(); ++i) {
            std::string name = (i < variable_names.size()) ? variable_names[i] : ("Var_" + std::to_string(i));
            std::cout << name << "=" << state->l_variable_values[i] << (i == state->l_variable_values.size() - 1 ? "" : ", ");
        }
        std::cout << "]";
    }
    std::cout << std::endl;
}

void LocalAttractor::show_short() const {
    std::cout << "Net. Index: " << network_index << ", Attrac. Index: " << l_index << ", States: ";
    for (auto& state : l_states) {
        std::cout << "[";
        for (size_t i = 0; i < state->l_variable_values.size(); ++i) {
            std::string name = (i < variable_names.size()) ? variable_names[i] : ("Var_" + std::to_string(i));
            std::cout << name << "=" << state->l_variable_values[i] << (i == state->l_variable_values.size() - 1 ? "" : ", ");
        }
        std::cout << "]";
    }
    std::cout << std::endl;
}

} // namespace cbnetwork
