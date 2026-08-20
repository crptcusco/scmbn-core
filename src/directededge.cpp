#include "cbnetwork/directededge.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <stack>
#include <vector>

namespace cbnetwork {

// Helper for Boolean expression evaluation matching Python's logic
class BooleanParser {
public:
    struct Node {
        virtual ~Node() = default;
        enum Type { CONSTANT, VARIABLE, UNARY, BINARY, THRESHOLD } type;
        bool const_val;
        char var_name;
        std::string op;
        std::unique_ptr<Node> left, right;

        Node(bool val) : type(CONSTANT), const_val(val) {}
        Node(char name) : type(VARIABLE), var_name(name) {}
        Node(std::string o, std::unique_ptr<Node> opnd) : type(UNARY), op(o), left(std::move(opnd)) {}
        Node(std::string o, std::unique_ptr<Node> l, std::unique_ptr<Node> r)
            : type(BINARY), op(o), left(std::move(l)), right(std::move(r)) {}
        Node(Type t) : type(t) {}
    };

    std::string input;
    size_t pos;

    BooleanParser(std::string s) : input(s), pos(0) {
        // Remove spaces for easier parsing
        input.erase(std::remove(input.begin(), input.end(), ' '), input.end());
    }

    std::string peekToken() {
        if (pos >= input.length()) return "";

        // Handle UTF-8 multi-byte characters
        unsigned char c = input[pos];
        if (c == 0xE2) { // Potential 3-byte character
            if (pos + 2 < input.length()) {
                std::string utf8 = input.substr(pos, 3);
                return utf8;
            }
        }
        return std::string(1, input[pos]);
    }

    void consumeToken(const std::string& t) {
        pos += t.length();
    }

    std::unique_ptr<Node> parse() {
        return disjunction();
    }

    // Precedence matching Python: disjunction < conjunction < implication < unary
    std::unique_ptr<Node> disjunction() {
        if (input.find("Threshold(") != std::string::npos) {
            // Handle Threshold(k, {A, B, C})
            size_t k_pos = input.find("Threshold(") + 10;
            size_t comma_pos = input.find(",", k_pos);
            int k = std::stoi(input.substr(k_pos, comma_pos - k_pos));

            size_t brace_start = input.find("{", comma_pos) + 1;
            size_t brace_end = input.find("}", brace_start);
            std::string vars_str = input.substr(brace_start, brace_end - brace_start);

            std::vector<char> vars;
            size_t v_pos = 0;
            while ((v_pos = vars_str.find_first_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ", v_pos)) != std::string::npos) {
                vars.push_back(vars_str[v_pos]);
                v_pos++;
            }

            return std::make_unique<ThresholdNode>(k, vars);
        }
        auto node = conjunction();
        while (peekToken() == "∨") {
            consumeToken("∨");
            node = std::make_unique<Node>("∨", std::move(node), disjunction());
        }
        return node;
    }

    std::unique_ptr<Node> conjunction() {
        auto node = implication();
        while (peekToken() == "∧") {
            consumeToken("∧");
            node = std::make_unique<Node>("∧", std::move(node), conjunction());
        }
        return node;
    }

    std::unique_ptr<Node> implication() {
        auto node = unary_expr();
        std::string t = peekToken();
        if (t == "→" || t == "↔") {
            consumeToken(t);
            node = std::make_unique<Node>(t, std::move(node), implication());
        }
        return node;
    }

    std::unique_ptr<Node> unary_expr() {
        if (peekToken() == "~") {
            consumeToken("~");
            return std::make_unique<Node>("~", unary_expr());
        }
        return term();
    }

    std::unique_ptr<Node> term() {
        std::string t = peekToken();
        if (t == "(") {
            consumeToken("(");
            auto node = disjunction();
            if (peekToken() == ")") consumeToken(")");
            return node;
        } else if (t == "0" || t == "1") {
            consumeToken(t);
            return std::make_unique<Node>(t == "1");
        } else if (!t.empty() && ((t[0] >= 'A' && t[0] <= 'Z') || (t[0] >= 'a' && t[0] <= 'z'))) {
            char name = t[0];
            consumeToken(t);
            return std::make_unique<Node>(name);
        }
        return nullptr;
    }

    static bool evaluate(const Node* node, const std::map<char, bool>& env) {
        if (!node) return false;
        if (node->type == Node::THRESHOLD) {
            const auto* tn = static_cast<const ThresholdNode*>(node);
            int count = 0;
            for (char v : tn->vars) if (env.count(v) && env.at(v)) count++;
            return count >= tn->k;
        }
        switch (node->type) {
            case Node::CONSTANT: return node->const_val;
            case Node::VARIABLE: return env.at(node->var_name);
            case Node::UNARY: return !evaluate(node->left.get(), env);
            case Node::BINARY:
                if (node->op == "∨") return evaluate(node->left.get(), env) || evaluate(node->right.get(), env);
                if (node->op == "∧") return evaluate(node->left.get(), env) && evaluate(node->right.get(), env);
                if (node->op == "→") return !evaluate(node->left.get(), env) || evaluate(node->right.get(), env);
                if (node->op == "↔") return evaluate(node->left.get(), env) == evaluate(node->right.get(), env);
        }
        return false;
    }

private:
    struct ThresholdNode : public Node {
        int k;
        std::vector<char> vars;
        ThresholdNode(int _k, std::vector<char> _vars) : Node(THRESHOLD), k(_k), vars(_vars) {}
    };
};

DirectedEdge::DirectedEdge(int idx, int idx_var, int in_net, int out_net,
                           const std::vector<int>& out_vars, const std::string& coup_func)
    : index(idx), index_variable(idx_var), input_local_network(in_net),
      output_local_network(out_net), l_output_variables(out_vars),
      coupling_function(coup_func), kind_signal(2) {

    d_kind_signal[1] = "RESTRICTED";
    d_kind_signal[2] = "NOT COMPUTE";
    d_kind_signal[3] = "STABLE";
    d_kind_signal[4] = "NOT STABLE";

    true_table = process_true_table();
}

void DirectedEdge::show() const {
    std::cout << "Index Edge: " << index << " - Relation: " << output_local_network
              << " -> " << input_local_network << " - Variable: " << index_variable << std::endl;
    std::cout << "Variables: [";
    for (size_t i = 0; i < l_output_variables.size(); ++i) {
        std::cout << l_output_variables[i] << (i == l_output_variables.size() - 1 ? "" : ", ");
    }
    std::cout << "], Coupling Function: " << coupling_function << std::endl;
    std::cout << "Kind signal: " << kind_signal << " - " << d_kind_signal.at(kind_signal) << std::endl;
}

void DirectedEdge::show_short() const {
    std::cout << output_local_network << " , " << input_local_network << std::endl;
}

std::pair<int, int> DirectedEdge::get_edge() const {
    return {output_local_network, input_local_network};
}

std::map<std::string, std::string> DirectedEdge::process_true_table() {
    std::map<std::string, std::string> r_true_table;
    int n = l_output_variables.size();
    if (n == 0) return r_true_table;

    std::string translated_func = coupling_function;
    std::vector<char> abecedario;
    for(char c='A'; c<='Z'; ++c) abecedario.push_back(c);

    std::map<int, char> var_to_char;
    for(int i=0; i<n; ++i) {
        var_to_char[l_output_variables[i]] = abecedario[i];
    }

    // Replace variable indices with A, B, C...
    // To match Python's replacement: dict_aux_var_saida[" " + str(variable_saida) + " "] = l_abecedario[cont_aux_abecedario]
    for(auto const& [var, ch] : var_to_char) {
        std::string pattern = " " + std::to_string(var) + " ";
        size_t start_pos = 0;
        while((start_pos = translated_func.find(pattern, start_pos)) != std::string::npos) {
            translated_func.replace(start_pos, pattern.length(), std::string(1, ch));
        }
    }

    BooleanParser parser(translated_func);
    auto root = parser.parse();

    int total_permutations = 1 << n;
    for (int i = 0; i < total_permutations; ++i) {
        std::map<char, bool> env;
        std::string key = "";
        for (int j = 0; j < n; ++j) {
            bool val = (i >> (n - 1 - j)) & 1;
            env[abecedario[j]] = val;
            key += val ? "1" : "0";
        }

        bool res = BooleanParser::evaluate(root.get(), env);
        r_true_table[key] = res ? "1" : "0";
    }

    return r_true_table;
}

} // namespace cbnetwork
