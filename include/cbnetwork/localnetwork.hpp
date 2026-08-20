#ifndef LOCALNETWORK_HPP
#define LOCALNETWORK_HPP

#include <vector>
#include <string>
#include <map>
#include <memory>
#include "cbnetwork/localscene.hpp"
#include "cbnetwork/internalvariable.hpp"
#include "cbnetwork/directededge.hpp"

namespace cbnetwork {

class LocalNetwork {
public:
    int index;
    std::vector<int> internal_variables;
    std::vector<std::string> variable_names;
    std::vector<std::shared_ptr<InternalVariable>> descriptive_function_variables;
    std::vector<int> external_variables;
    std::vector<int> total_variables;
    int total_variables_count;

    std::vector<std::shared_ptr<DirectedEdge>> input_signals;
    std::vector<std::shared_ptr<DirectedEdge>> output_signals;

    int attractor_count;
    std::vector<std::shared_ptr<LocalScene>> local_scenes;

    LocalNetwork(int idx, const std::vector<int>& inter_vars)
        : index(idx), internal_variables(inter_vars), total_variables_count(0), attractor_count(0) {
        for (size_t i = 0; i < inter_vars.size(); ++i) {
            variable_names.push_back("Var_" + std::to_string(i));
        }
    }

    void show() const;
    void process_input_signals(const std::vector<std::shared_ptr<DirectedEdge>>& inputs);

    static int evaluate_boolean_function(const std::vector<std::vector<int>>& cnf,
                                         const std::map<int, int>& state,
                                         const std::map<int, int>& external_values);

    static std::shared_ptr<LocalNetwork> find_local_attractors_brute_force(
        std::shared_ptr<LocalNetwork> local_network,
        const std::vector<std::string>& local_scenes_strings = {});

    // SAT-based attractor search following Duvrova's increasing-path method.
    static std::shared_ptr<LocalNetwork> find_local_attractors_duvrova(
        std::shared_ptr<LocalNetwork> local_network,
        const std::vector<std::string>& local_scenes_strings = {});

};

} // namespace cbnetwork

#endif // LOCALNETWORK_HPP
