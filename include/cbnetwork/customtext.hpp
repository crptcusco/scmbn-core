#ifndef CUSTOMTEXT_HPP
#define CUSTOMTEXT_HPP

#include <iostream>
#include <string>
#include <algorithm>

namespace cbnetwork {

class CustomText {
public:
    static void print_duplex_line() {
        std::cout << std::string(50, '=') << std::endl;
    }

    static void print_simple_line() {
        std::cout << std::string(50, '-') << std::endl;
    }

    static void print_message(const std::string& message, bool show) {
        if (show) {
            std::cout << message << std::endl;
        }
    }

    static void print_stars() {
        std::cout << std::string(50, '*') << std::endl;
    }

    static void print_dollars() {
        std::cout << std::string(50, '$') << std::endl;
    }

    static void make_principal_title(std::string title) {
        print_dollars();
        std::transform(title.begin(), title.end(), title.begin(), ::toupper);
        std::cout << title << std::endl;
    }

    static void make_title(std::string title) {
        print_stars();
        std::transform(title.begin(), title.end(), title.begin(), ::toupper);
        std::cout << title << std::endl;
    }

    static void make_sub_title(std::string sub_title) {
        print_duplex_line();
        std::transform(sub_title.begin(), sub_title.end(), sub_title.begin(), ::toupper);
        std::cout << sub_title << std::endl;
    }

    static void make_sub_sub_title(const std::string& sub_sub_title) {
        print_simple_line();
        std::cout << sub_sub_title << std::endl;
    }

    static void send_warning(const std::string& message) {
        std::cout << "WARNING: " << message << std::endl;
    }
};

} // namespace cbnetwork

#endif // CUSTOMTEXT_HPP
