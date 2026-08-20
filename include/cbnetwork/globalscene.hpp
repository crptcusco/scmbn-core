#ifndef GLOBALSCENE_HPP
#define GLOBALSCENE_HPP

#include <vector>
#include <iostream>
#include <string>

namespace cbnetwork {

class GlobalScene {
public:
    std::vector<int> l_indexes;
    std::vector<int> values;
    int number_attractor_fields;

    GlobalScene(const std::vector<int>& indexes, const std::vector<int>& vals)
        : l_indexes(indexes), values(vals), number_attractor_fields(0) {}

    void show() const {
        std::cout << "Indexes: [";
        for (size_t i = 0; i < l_indexes.size(); ++i) {
            std::cout << l_indexes[i] << (i == l_indexes.size() - 1 ? "" : ", ");
        }
        std::cout << "]" << std::endl;

        std::cout << "Values: [";
        for (size_t i = 0; i < values.size(); ++i) {
            std::cout << values[i] << (i == values.size() - 1 ? "" : ", ");
        }
        std::cout << "]" << std::endl;
    }
};

} // namespace cbnetwork

#endif // GLOBALSCENE_HPP
