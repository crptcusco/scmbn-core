#ifndef CNFLIST_HPP
#define CNFLIST_HPP

#include <vector>
#include <random>
#include <algorithm>
#include <set>

namespace cbnetwork {

class CNFList {
public:
    static std::vector<std::vector<int>> generate_cnf(const std::vector<int>& l_inter_vars,
                                                    int* input_coup_sig_index,
                                                    int max_clauses = 2,
                                                    int max_literals = 3) {
        std::random_device rd;
        std::mt19937 gen(rd());

        int max_attempts = 100;
        for (int attempt = 0; attempt < max_attempts; ++attempt) {
            std::uniform_int_distribution<> d_clauses(1, max_clauses);
            int num_clauses = d_clauses(gen);
            std::vector<std::vector<int>> l_cnf;

            if (input_coup_sig_index != nullptr) {
                int var = *input_coup_sig_index;
                std::uniform_int_distribution<> d_bool(0, 1);
                if (d_bool(gen)) var = -var;
                l_cnf.push_back({var});
            }

            for (int i = 0; i < num_clauses; ++i) {
                if (l_inter_vars.empty()) break;

                int effective_max_literals = std::min(max_literals, (int)l_inter_vars.size());
                std::uniform_int_distribution<> d_literals(1, effective_max_literals);
                int num_literals = d_literals(gen);

                std::vector<int> current_inter_vars = l_inter_vars;
                std::shuffle(current_inter_vars.begin(), current_inter_vars.end(), gen);

                std::vector<int> clause;
                for (int j = 0; j < num_literals; ++j) {
                    int var = current_inter_vars[j];
                    std::uniform_int_distribution<> d_bool(0, 1);
                    if (d_bool(gen)) var = -var;
                    clause.push_back(var);
                }

                clause = simplify_clause(clause);
                if (!clause.empty()) {
                    l_cnf.push_back(clause);
                }
            }

            l_cnf = remove_duplicates(l_cnf);
            if (!l_cnf.empty()) return l_cnf;
        }

        return {}; // Should not reach here with retry
    }

    static std::vector<int> simplify_clause(std::vector<int> clause) {
        std::set<int> s(clause.begin(), clause.end());
        std::vector<int> simplified;
        for (int literal : s) {
            if (s.find(-literal) == s.end()) {
                simplified.push_back(literal);
            }
        }
        return simplified;
    }

    static std::vector<std::vector<int>> remove_duplicates(std::vector<std::vector<int>> l_cnf) {
        std::set<std::vector<int>> unique_clauses;
        for (auto& clause : l_cnf) {
            std::sort(clause.begin(), clause.end());
            unique_clauses.insert(clause);
        }
        return std::vector<std::vector<int>>(unique_clauses.begin(), unique_clauses.end());
    }
};

} // namespace cbnetwork

#endif // CNFLIST_HPP
