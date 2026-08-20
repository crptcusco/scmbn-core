#include "cbnetwork/network_factory.hpp"
#include "cbnetwork/coupling.hpp"
#include <algorithm>

namespace cbnetwork {

std::shared_ptr<CBN> NetworkFactory::create_from_config(const ExperimentConfig& config) {
    std::shared_ptr<CouplingStrategy> coupling;
    std::string type = config.coupling_type;
    std::transform(type.begin(), type.end(), type.begin(), ::toupper);

    if (type == "OR") {
        coupling = std::make_shared<OrCoupling>();
    } else if (type == "AND") {
        coupling = std::make_shared<AndCoupling>();
    } else if (type == "XOR") {
        coupling = std::make_shared<XorCoupling>();
    } else if (type == "CONSTANT") {
        coupling = std::make_shared<ConstantCoupling>(config.coupling_param);
    } else if (type == "THRESHOLD") {
        coupling = std::make_shared<ThresholdCoupling>(config.coupling_param);
    } else {
        // Default to OR
        coupling = std::make_shared<OrCoupling>();
    }

    return CBN::cbn_generator(
        config.v_topology,
        config.n_networks,
        config.n_var_network,
        config.n_input_variables,
        config.n_output_variables,
        config.n_max_of_clauses,
        config.n_max_of_literals,
        config.n_edges,
        coupling
    );
}

} // namespace cbnetwork
