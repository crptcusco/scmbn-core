#ifndef INTERNALVARIABLE_HPP
#define INTERNALVARIABLE_HPP

#include <vector>
#include <iostream>

namespace cbnetwork {

class InternalVariable {
public:
    int index;
    std::vector<std::vector<int>> cnf_function;

    InternalVariable(int idx, const std::vector<std::vector<int>>& cnf)
        : index(idx), cnf_function(cnf) {}

    void show() const {
        std::cout << "Variable Index: " << index << " -> CNF : [";
        for (size_t i = 0; i < cnf_function.size(); ++i) {
            std::cout << "[";
            for (size_t j = 0; j < cnf_function[i].size(); ++j) {
                std::cout << cnf_function[i][j] << (j == cnf_function[i].size() - 1 ? "" : ", ");
            }
            std::cout << "]" << (i == cnf_function.size() - 1 ? "" : ", ");
        }
        std::cout << "]" << std::endl;
    }
};

} // namespace cbnetwork

#endif // INTERNALVARIABLE_HPP
