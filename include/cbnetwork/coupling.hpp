#ifndef COUPLING_HPP
#define COUPLING_HPP

#include <vector>
#include <string>
#include <numeric>
#include <algorithm>
#include <stdexcept>
#include <functional>

namespace cbnetwork {

class CouplingStrategy {
public:
    virtual ~CouplingStrategy() = default;
    virtual std::string generate_coupling_function(const std::vector<int>& output_variables) const = 0;
    virtual std::vector<std::vector<int>> to_cnf(const std::vector<int>& output_variables, int coupling_variable) const = 0;
};

class ConstantCoupling : public CouplingStrategy {
public:
    int value;
    ConstantCoupling(int v) : value(v != 0 ? 1 : 0) {}

    std::string generate_coupling_function(const std::vector<int>& output_variables) const override {
        return " " + std::to_string(value) + " ";
    }

    std::vector<std::vector<int>> to_cnf(const std::vector<int>& output_variables, int coupling_variable) const override {
        if (value == 1) return {{coupling_variable}};
        return {{-coupling_variable}};
    }
};

class OrCoupling : public CouplingStrategy {
public:
    std::string generate_coupling_function(const std::vector<int>& output_variables) const override {
        if (output_variables.empty()) return " ";
        std::string result = " ";
        for (size_t i = 0; i < output_variables.size(); ++i) {
            result += std::to_string(output_variables[i]);
            if (i < output_variables.size() - 1) result += " ∨ ";
        }
        result += " ";
        return result;
    }

    std::vector<std::vector<int>> to_cnf(const std::vector<int>& output_variables, int coupling_variable) const override {
        std::vector<std::vector<int>> clauses;
        for (int var : output_variables) {
            clauses.push_back({-var, coupling_variable});
        }
        std::vector<int> last_clause = output_variables;
        last_clause.push_back(-coupling_variable);
        clauses.push_back(last_clause);
        return clauses;
    }
};

class AndCoupling : public CouplingStrategy {
public:
    std::string generate_coupling_function(const std::vector<int>& output_variables) const override {
        if (output_variables.empty()) return " ";
        std::string result = " ";
        for (size_t i = 0; i < output_variables.size(); ++i) {
            result += std::to_string(output_variables[i]);
            if (i < output_variables.size() - 1) result += " ∧ ";
        }
        result += " ";
        return result;
    }

    std::vector<std::vector<int>> to_cnf(const std::vector<int>& output_variables, int coupling_variable) const override {
        std::vector<std::vector<int>> clauses;
        for (int var : output_variables) {
            clauses.push_back({var, -coupling_variable});
        }
        std::vector<int> last_clause;
        for (int var : output_variables) {
            last_clause.push_back(-var);
        }
        last_clause.push_back(coupling_variable);
        clauses.push_back(last_clause);
        return clauses;
    }
};

class XorCoupling : public CouplingStrategy {
public:
    std::string generate_coupling_function(const std::vector<int>& output_variables) const override {
        if (output_variables.empty()) return " 0 ";
        std::string result = " ";
        for (size_t i = 0; i < output_variables.size(); ++i) {
            result += std::to_string(output_variables[i]);
            if (i < output_variables.size() - 1) result += " ⊕ ";
        }
        result += " ";
        return result;
    }

    std::vector<std::vector<int>> to_cnf(const std::vector<int>& output_variables, int coupling_variable) const override {
        int n = output_variables.size();
        std::vector<std::vector<int>> clauses;
        for (int i = 0; i < (1 << (n + 1)); ++i) {
            int ones = 0;
            for (int j = 0; j <= n; ++j) if ((i >> j) & 1) ones++;
            if (ones % 2 != 0) {
                std::vector<int> clause;
                if (i & 1) clause.push_back(-coupling_variable);
                else clause.push_back(coupling_variable);
                for (int j = 0; j < n; ++j) {
                    if ((i >> (j + 1)) & 1) clause.push_back(-output_variables[j]);
                    else clause.push_back(output_variables[j]);
                }
                clauses.push_back(clause);
            }
        }
        return clauses;
    }
};

class ThresholdCoupling : public CouplingStrategy {
public:
    int threshold;

    ThresholdCoupling(int t) : threshold(t) {
        if (t <= 0) {
            throw std::invalid_argument("Threshold must be a positive integer.");
        }
    }

    std::string generate_coupling_function(const std::vector<int>& output_variables) const override {
        std::string result = " Threshold(" + std::to_string(threshold) + ", {";
        for (size_t i = 0; i < output_variables.size(); ++i) {
            result += std::to_string(output_variables[i]);
            if (i < output_variables.size() - 1) result += ", ";
        }
        result += "}) ";
        return result;
    }

    std::vector<std::vector<int>> to_cnf(const std::vector<int>& output_variables, int coupling_variable) const override {
        int n = output_variables.size();
        int k = threshold;
        std::vector<std::vector<int>> clauses;

        if (k > n) {
            return {{-coupling_variable}};
        }

        // Implication 1: (sum(Vi) >= k) => C
        // Encode as (¬V_i1 ∨ ¬V_i2 ∨ ... ∨ ¬V_ik ∨ C) for all combinations of k vars.
        std::vector<int> combination(k);
        std::function<void(int, int)> combinations_k = [&](int offset, int k_idx) {
            if (k_idx == k) {
                std::vector<int> clause;
                for (int v : combination) clause.push_back(-v);
                clause.push_back(coupling_variable);
                clauses.push_back(clause);
                return;
            }
            for (int i = offset; i <= n - k + k_idx; ++i) {
                combination[k_idx] = output_variables[i];
                combinations_k(i + 1, k_idx + 1);
            }
        };
        combinations_k(0, 0);

        // Implication 2: C => (sum(Vi) >= k)
        // Clause: (¬C ∨ V_i1 ∨ V_i2 ∨ ... ∨ V_i(n-k+1)) for all combinations of n-k+1 vars.
        int k2 = n - k + 1;
        std::vector<int> combination2(k2);
        std::function<void(int, int)> combinations_nk1 = [&](int offset, int k_idx) {
            if (k_idx == k2) {
                std::vector<int> clause = {-coupling_variable};
                for (int v : combination2) clause.push_back(v);
                clauses.push_back(clause);
                return;
            }
            for (int i = offset; i <= n - k2 + k_idx; ++i) {
                combination2[k_idx] = output_variables[i];
                combinations_nk1(i + 1, k_idx + 1);
            }
        };
        combinations_nk1(0, 0);

        return clauses;
    }
};

} // namespace cbnetwork

#endif // COUPLING_HPP
