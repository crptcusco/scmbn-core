#ifndef NETWORK_FACTORY_HPP
#define NETWORK_FACTORY_HPP

#include <string>
#include <memory>
#include "cbnetwork/cbnetwork.hpp"

namespace cbnetwork {

struct ExperimentConfig {
    std::string id;
    int v_topology;
    int n_networks;
    int n_var_network;
    int n_input_variables;
    int n_output_variables;
    int n_max_of_clauses;
    int n_max_of_literals;
    int n_edges;
    std::string coupling_type;
    int coupling_param; // value for CONSTANT or threshold for THRESHOLD
};

class NetworkFactory {
public:
    static std::shared_ptr<CBN> create_from_config(const ExperimentConfig& config);
};

} // namespace cbnetwork

#endif // NETWORK_FACTORY_HPP
