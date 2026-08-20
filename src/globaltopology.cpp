#include "cbnetwork/globaltopology.hpp"
#include <memory>
#include <random>
#include <algorithm>
#include <set>
#include <iostream>

namespace cbnetwork {

std::map<int, std::string> GlobalTopology::allowed_topologies = {
    {1, "complete"},
    {2, "aleatory_fixed_2_input_edges"},
    {3, "cycle"},
    {4, "path"},
    {5, "aleatory_gn"},
    {6, "aleatory_gnc"},
    {7, "dorogovtsev_mendes"},
    {8, "small_world"},
    {9, "scale_free"},
    {10, "random"}
};

PathDigraph::PathDigraph(int n_nodes) : GlobalTopology(4, {}) {
    for (int i = 1; i < n_nodes; ++i) {
        l_edges.push_back({i, i + 1});
    }
}

CycleDigraph::CycleDigraph(int n_nodes) : GlobalTopology(3, {}) {
    for (int i = 1; i < n_nodes; ++i) {
        l_edges.push_back({i, i + 1});
    }
    l_edges.push_back({n_nodes, 1});
}

CompleteDigraph::CompleteDigraph(int n_nodes) : GlobalTopology(1, {}) {
    for (int i = 1; i <= n_nodes; ++i) {
        for (int j = 1; j <= n_nodes; ++j) {
            if (i != j) {
                l_edges.push_back({i, j});
            }
        }
    }
}

AleatoryFixedDigraph::AleatoryFixedDigraph(int n_nodes, int n_edges) : GlobalTopology(2, {}) {
    if (n_edges == -1) n_edges = n_nodes;
    int max_edges = 2 * n_nodes;
    if (n_edges > max_edges) n_edges = max_edges;

    std::random_device rd;
    std::mt19937 gen(rd());

    // Spanning tree to ensure weak connectivity
    std::vector<int> nodes(n_nodes);
    for(int i=0; i<n_nodes; ++i) nodes[i] = i+1;

    for (int i = 1; i < n_nodes; ++i) {
        std::uniform_int_distribution<> dis(0, i - 1);
        int u = nodes[dis(gen)];
        int v = nodes[i];
        l_edges.push_back({u, v});
    }

    std::uniform_int_distribution<> dis(1, n_nodes);
    while ((int)l_edges.size() < n_edges) {
        int u = dis(gen);
        int v = dis(gen);
        if (u != v) {
            bool exists = false;
            for(auto& edge : l_edges) {
                if(edge.first == u && edge.second == v) {
                    exists = true;
                    break;
                }
            }
            if(!exists) l_edges.push_back({u, v});
        }
    }
}

DorogovtsevMendesDigraph::DorogovtsevMendesDigraph(int n_nodes) : GlobalTopology(7, {}) {
    if (n_nodes < 3) return;
    l_edges = {{1, 2}, {2, 3}, {3, 1}};

    std::random_device rd;
    std::mt19937 gen(rd());

    for (int new_node = 4; new_node <= n_nodes; ++new_node) {
        std::uniform_int_distribution<> dis(0, l_edges.size() - 1);
        auto edge = l_edges[dis(gen)];
        l_edges.push_back({new_node, edge.first});
        l_edges.push_back({new_node, edge.second});
    }
}

SmallWorldGraph::SmallWorldGraph(int n_nodes, int k_neighbors, double p_rewire) : GlobalTopology(8, {}) {
    // Basic ring lattice
    for (int i = 1; i <= n_nodes; ++i) {
        for (int j = 1; j <= k_neighbors / 2; ++j) {
            int neighbor = i + j;
            if (neighbor > n_nodes) neighbor -= n_nodes;
            l_edges.push_back({i, neighbor});
        }
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis_p(0, 1);
    std::uniform_int_distribution<> dis_n(1, n_nodes);

    for (auto& edge : l_edges) {
        if (dis_p(gen) < p_rewire) {
            int new_v = dis_n(gen);
            while (new_v == edge.first) new_v = dis_n(gen);
            edge.second = new_v;
        }
    }
}

ScaleFreeGraph::ScaleFreeGraph(int n_nodes, int m_edges) : GlobalTopology(9, {}) {
    if (n_nodes <= m_edges) return;

    // Initial complete graph of m_edges + 1 nodes
    for (int i = 1; i <= m_edges + 1; ++i) {
        for (int j = i + 1; j <= m_edges + 1; ++j) {
            l_edges.push_back({i, j});
        }
    }

    std::random_device rd;
    std::mt19937 gen(rd());

    for (int new_node = m_edges + 2; new_node <= n_nodes; ++new_node) {
        std::vector<int> degrees(new_node, 0);
        for (auto& edge : l_edges) {
            degrees[edge.first]++;
            degrees[edge.second]++;
        }

        std::vector<int> nodes;
        for(int i=1; i<new_node; ++i) {
            for(int j=0; j<degrees[i]; ++j) nodes.push_back(i);
        }

        std::set<int> targets;
        while ((int)targets.size() < m_edges && !nodes.empty()) {
            std::uniform_int_distribution<> dis(0, nodes.size() - 1);
            int idx = dis(gen);
            targets.insert(nodes[idx]);
        }

        for (int t : targets) {
            l_edges.push_back({new_node, t});
        }
    }
}

RandomGraph::RandomGraph(int n_nodes, double p_edge) : GlobalTopology(10, {}) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0, 1);

    for (int i = 1; i <= n_nodes; ++i) {
        for (int j = 1; j <= n_nodes; ++j) {
            if (i != j && dis(gen) < p_edge) {
                l_edges.push_back({i, j});
            }
        }
    }
}

std::shared_ptr<GlobalTopology> GlobalTopology::generate_sample_topology(int v_topology, int n_nodes, int n_edges) {
    if (v_topology == 1) return std::make_shared<CompleteDigraph>(n_nodes);
    if (v_topology == 2) return std::make_shared<AleatoryFixedDigraph>(n_nodes, n_edges);
    if (v_topology == 3) return std::make_shared<CycleDigraph>(n_nodes);
    if (v_topology == 4) return std::make_shared<PathDigraph>(n_nodes);
    if (v_topology == 7) return std::make_shared<DorogovtsevMendesDigraph>(n_nodes);
    if (v_topology == 8) return std::make_shared<SmallWorldGraph>(n_nodes);
    if (v_topology == 9) return std::make_shared<ScaleFreeGraph>(n_nodes);
    if (v_topology == 10) return std::make_shared<RandomGraph>(n_nodes);
    return nullptr;
}

} // namespace cbnetwork
