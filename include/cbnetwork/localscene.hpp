#ifndef LOCALSCENE_HPP
#define LOCALSCENE_HPP

#include <vector>
#include <string>
#include <iostream>
#include <memory>

namespace cbnetwork {

class LocalState {
public:
    std::vector<int> l_variable_values;

    LocalState(const std::vector<int>& values) : l_variable_values(values) {}
    LocalState(const std::vector<std::string>& values) {
        for (const auto& v : values) {
            l_variable_values.push_back(std::stoi(v));
        }
    }
};

class LocalAttractor {
public:
    int g_index;
    int l_index;
    std::vector<std::shared_ptr<LocalState>> l_states;
    int network_index;
    std::vector<int> relation_index;
    std::string local_scene;
    std::vector<std::string> variable_names;

    LocalAttractor(int g_idx, int l_idx, const std::vector<std::shared_ptr<LocalState>>& states,
                   int net_idx, const std::vector<int>& rel_idx = {}, const std::string& scene = "",
                   const std::vector<std::string>& var_names = {})
        : g_index(g_idx), l_index(l_idx), l_states(states), network_index(net_idx),
          relation_index(rel_idx), local_scene(scene), variable_names(var_names) {}

    void show() const;
    void show_short() const;
};

class LocalScene {
public:
    int index;
    std::vector<std::string> l_values;
    std::vector<int> l_index_signals;
    std::vector<std::shared_ptr<LocalAttractor>> l_attractors;

    LocalScene(int idx, const std::vector<std::string>& values = {}, const std::vector<int>& signals = {})
        : index(idx), l_values(values), l_index_signals(signals) {}
};

} // namespace cbnetwork

#endif // LOCALSCENE_HPP
