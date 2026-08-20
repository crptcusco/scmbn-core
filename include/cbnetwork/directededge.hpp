#ifndef DIRECTEDEDGE_HPP
#define DIRECTEDEDGE_HPP

#include <vector>
#include <string>
#include <map>
#include <memory>

namespace cbnetwork {

class LocalAttractor; // Forward declaration

class DirectedEdge {
public:
    int index;
    int index_variable;
    int input_local_network;
    int output_local_network;
    std::vector<int> l_output_variables;
    std::string coupling_function;

    std::map<std::string, std::string> true_table;
    std::map<int, std::string> d_kind_signal;
    int kind_signal;
    std::map<int, std::vector<std::shared_ptr<LocalAttractor>>> d_out_value_to_attractor;
    std::map<int, std::vector<std::pair<int, int>>> d_comp_pairs_attractors_by_value;

    DirectedEdge(int idx, int idx_var, int in_net, int out_net,
                 const std::vector<int>& out_vars, const std::string& coup_func);

    void show() const;
    void show_short() const;
    std::pair<int, int> get_edge() const;

    std::map<std::string, std::string> process_true_table();
};

} // namespace cbnetwork

#endif // DIRECTEDEDGE_HPP
