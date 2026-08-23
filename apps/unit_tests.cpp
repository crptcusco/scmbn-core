#include "cbnetwork/coupling.hpp"
#include "cbnetwork/localnetwork.hpp"
#include "cbnetwork/cbnetwork.hpp"
#include "nlohmann/json.hpp"
#include <iostream>
#include <fstream>
#include <cassert>
#include <vector>
#include <map>
#include <stdexcept>
#include <cstdlib>

using namespace cbnetwork;

void test_or_coupling() {
    OrCoupling strategy;
    std::vector<int> output_vars = {1, 2, 3};
    int coupling_var = 10;

    std::string func = strategy.generate_coupling_function(output_vars);
    assert(func == " 1 ∨ 2 ∨ 3 ");

    auto cnf = strategy.to_cnf(output_vars, coupling_var);
    std::vector<std::vector<int>> expected = {{-1, 10}, {-2, 10}, {-3, 10}, {1, 2, 3, -10}};
    assert(cnf == expected);
    std::cout << "test_or_coupling passed" << std::endl;
}

void test_and_coupling() {
    AndCoupling strategy;
    std::vector<int> output_vars = {1, 2, 3};
    int coupling_var = 10;

    std::string func = strategy.generate_coupling_function(output_vars);
    assert(func == " 1 ∧ 2 ∧ 3 ");

    auto cnf = strategy.to_cnf(output_vars, coupling_var);
    std::vector<std::vector<int>> expected = {{1, -10}, {2, -10}, {3, -10}, {-1, -2, -3, 10}};
    assert(cnf == expected);
    std::cout << "test_and_coupling passed" << std::endl;
}

void test_threshold_coupling() {
    ThresholdCoupling strategy(2);
    std::vector<int> output_vars = {1, 2, 3};
    int coupling_var = 10;

    auto cnf = strategy.to_cnf(output_vars, coupling_var);
    // Implication 1: (sum >= 2) => C.  Combos of 2 from {1,2,3}: {1,2}, {1,3}, {2,3}. Clauses: {-1,-2,10}, {-1,-3,10}, {-2,-3,10}
    // Implication 2: C => (sum >= 2).  Combos of 3-2+1=2 from {1,2,3}: {1,2}, {1,3}, {2,3}. Clauses: {-10,1,2}, {-10,1,3}, {-10,2,3}

    std::vector<std::vector<int>> expected = {
        {-1, -2, 10}, {-1, -3, 10}, {-2, -3, 10},
        {-10, 1, 2}, {-10, 1, 3}, {-10, 2, 3}
    };
    assert(cnf == expected);
    std::cout << "test_threshold_coupling passed" << std::endl;
}

void test_evaluate_boolean_function() {
    // v1 = v2 OR v3
    // CNF: (-v2 | v1), (-v3 | v1), (v2 | v3 | -v1)
    std::vector<std::vector<int>> cnf = {{-2, 1}, {-3, 1}, {2, 3, -1}};

    std::map<int, int> state = {{1, 1}};
    std::map<int, int> external = {{2, 1}, {3, 0}};
    assert(LocalNetwork::evaluate_boolean_function(cnf, state, external) == 1);

    state[1] = 0;
    assert(LocalNetwork::evaluate_boolean_function(cnf, state, external) == 0);

    external[2] = 0;
    external[3] = 0;
    state[1] = 0;
    assert(LocalNetwork::evaluate_boolean_function(cnf, state, external) == 1);

    std::cout << "test_evaluate_boolean_function passed" << std::endl;
}

void test_json_threshold_with_true_table() {
    std::string filename = "test_threshold_tt.json";
    nlohmann::json j = {
        {"local_networks", {
            {{"index", 1}, {"internal_variables", {1, 2}}, {"logic", {
                {{"index", 1}, {"cnf", {{1}}}},
                {{"index", 2}, {"cnf", {{2}}}}
            }}},
            {{"index", 2}, {"internal_variables", {3, 4}}, {"logic", {
                {{"index", 3}, {"cnf", {{3}}}},
                {{"index", 4}, {"cnf", {{4}}}}
            }}}
        }},
        {"directed_edges", {
            {
                {"index", 1},
                {"signal_var", 5},
                {"source", 1},
                {"target", 2},
                {"output_vars", {1, 2}},
                {"function", "Threshold"},
                {"true_table", {{"00", "0"}, {"01", "1"}, {"10", "1"}, {"11", "1"}}}
            }
        }}
    };
    std::ofstream out(filename);
    out << j.dump(4);
    out.close();

    auto cbn = CBN::load_network_from_json(filename);
    assert(cbn != nullptr);
    assert(cbn->l_directed_edges.size() == 1);
    assert(cbn->l_directed_edges[0]->coupling_function == "Threshold");
    assert(cbn->l_directed_edges[0]->true_table.at("00") == "0");
    assert(cbn->l_directed_edges[0]->true_table.at("11") == "1");

    std::remove(filename.c_str());
    std::cout << "test_json_threshold_with_true_table passed" << std::endl;
}

void test_json_invalid_function_without_true_table() {
    std::string filename = "test_invalid_func.json";
    nlohmann::json j = {
        {"local_networks", {
            {{"index", 1}, {"internal_variables", {1, 2}}, {"logic", {
                {{"index", 1}, {"cnf", {{1}}}},
                {{"index", 2}, {"cnf", {{2}}}}
            }}}
        }},
        {"directed_edges", {
            {
                {"index", 1},
                {"signal_var", 5},
                {"source", 1},
                {"target", 1},
                {"output_vars", {1, 2}},
                {"function", "Threshold"}
            }
        }}
    };
    std::ofstream out(filename);
    out << j.dump(4);
    out.close();

    bool caught = false;
    try {
        auto cbn = CBN::load_network_from_json(filename);
    } catch (const std::runtime_error& e) {
        caught = true;
        std::string err_msg = e.what();
        assert(err_msg.find("Failed to parse coupling function") != std::string::npos);
    }
    assert(caught);

    std::remove(filename.c_str());
    std::cout << "test_json_invalid_function_without_true_table passed" << std::endl;
}

void test_scientific_benchmarking_exit_code() {
    std::string filename = "test_invalid_func_bench.json";
    nlohmann::json j = {
        {"local_networks", {
            {{"index", 1}, {"internal_variables", {1, 2}}, {"logic", {
                {{"index", 1}, {"cnf", {{1}}}},
                {{"index", 2}, {"cnf", {{2}}}}
            }}}
        }},
        {"directed_edges", {
            {
                {"index", 1},
                {"signal_var", 5},
                {"source", 1},
                {"target", 1},
                {"output_vars", {1, 2}},
                {"function", "Threshold"}
            }
        }}
    };
    std::ofstream out(filename);
    out << j.dump(4);
    out.close();

    int code = std::system(("./scientific_benchmarking --input " + filename + " > /dev/null 2>&1").c_str());
    int exit_status = WEXITSTATUS(code);
    assert(exit_status != 0);

    std::remove(filename.c_str());
    std::cout << "test_scientific_benchmarking_exit_code passed" << std::endl;
}

int main() {
    test_or_coupling();
    test_and_coupling();
    test_threshold_coupling();
    test_evaluate_boolean_function();
    test_json_threshold_with_true_table();
    test_json_invalid_function_without_true_table();
    test_scientific_benchmarking_exit_code();
    std::cout << "All C++ unit tests passed!" << std::endl;
    return 0;
}
