#include "cbnetwork/coupling.hpp"
#include "cbnetwork/localnetwork.hpp"
#include "cbnetwork/cbnetwork.hpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <map>
#include <stdexcept>

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

int main() {
    test_or_coupling();
    test_and_coupling();
    test_threshold_coupling();
    test_evaluate_boolean_function();
    std::cout << "All C++ unit tests passed!" << std::endl;
    return 0;
}
