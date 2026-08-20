#ifndef GLOBALTOPOLOGY_HPP
#define GLOBALTOPOLOGY_HPP

#include <vector>
#include <utility>
#include <memory>
#include <string>
#include <map>

namespace cbnetwork {

class GlobalTopology {
public:
    int v_topology;
    std::vector<std::pair<int, int>> l_edges;
    static std::map<int, std::string> allowed_topologies;

    GlobalTopology(int v_topo, const std::vector<std::pair<int, int>>& edges)
        : v_topology(v_topo), l_edges(edges) {}

    virtual ~GlobalTopology() = default;

    static std::shared_ptr<GlobalTopology> generate_sample_topology(int v_topology, int n_nodes, int n_edges = -1);
};

class PathDigraph : public GlobalTopology {
public:
    PathDigraph(int n_nodes);
};

class CycleDigraph : public GlobalTopology {
public:
    CycleDigraph(int n_nodes);
};

class CompleteDigraph : public GlobalTopology {
public:
    CompleteDigraph(int n_nodes);
};

class AleatoryFixedDigraph : public GlobalTopology {
public:
    AleatoryFixedDigraph(int n_nodes, int n_edges);
};

class DorogovtsevMendesDigraph : public GlobalTopology {
public:
    DorogovtsevMendesDigraph(int n_nodes);
};

class SmallWorldGraph : public GlobalTopology {
public:
    SmallWorldGraph(int n_nodes, int k_neighbors = 3, double p_rewire = 0.5);
};

class ScaleFreeGraph : public GlobalTopology {
public:
    ScaleFreeGraph(int n_nodes, int m_edges = 2);
};

class RandomGraph : public GlobalTopology {
public:
    RandomGraph(int n_nodes, double p_edge = 0.5);
};

} // namespace cbnetwork

#endif // GLOBALTOPOLOGY_HPP
