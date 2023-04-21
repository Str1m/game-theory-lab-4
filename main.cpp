#include <iostream>
#include <vector>
#include <random>
#include <fstream>

const int DEPTH = 5;
const int NUM_PLAYERS = 3;
const int MIN_REWARD = 0;
const int MAX_REWARD = 50;
std::vector<int> NUM_STRATEGIES = {2, 2, 2};

struct Node {
    std::vector<std::vector<int>> values;
    std::vector<Node> children;
    bool optimal;
    bool optimal_path;
};

void treeGenerator(Node &node, int player, int depth, std::mt19937 &gen) {
    std::uniform_int_distribution<> dist(MIN_REWARD, MAX_REWARD);
    node.values.resize(NUM_PLAYERS);
    for (int i = 0; i < NUM_PLAYERS; ++i) {
        node.values[i] = {dist(gen)};;
    }
    node.optimal = false;
    if (depth > 0) {
        int strategies = NUM_STRATEGIES[player];
        node.children.resize(strategies);
        for (int i = 0; i < strategies; ++i) {
            treeGenerator(node.children[i], (player + 1) % NUM_PLAYERS, depth - 1, gen);
        }
    }
}

void backInduction(Node &node, int depth) {
    int player = depth % NUM_PLAYERS;
    if (node.children.empty()) {
        return;
    }
    for (auto &child: node.children) {
        backInduction(child, depth + 1);
    }
    int max_value = -1;
    for (size_t i = 0; i < node.children.size(); ++i) {
        auto &child = node.children[i];
        for (size_t j = 0; j < child.values[player].size(); ++j) {
            if (child.values[player][j] > max_value) {
                max_value = child.values[player][j];
                for (auto &sibling: node.children) {
                    sibling.optimal = false;
                }
                child.optimal = true;
                for (int k = 0; k < NUM_PLAYERS; ++k) {
                    node.values[k].clear();
                    node.values[k].emplace_back(child.values[k][j]);
                }
            } else if (child.values[player][j] == max_value) {
                child.optimal = true;
                for (int k = 0; k < NUM_PLAYERS; ++k) {
                    node.values[k].emplace_back(child.values[k][j]);
                }
            }
        }
    }
}

void markOptimalPath(Node &node, int player) {
    if (node.children.empty()) {
        node.optimal_path = true;
        return;
    }
    node.optimal_path = false;
    for (auto &child: node.children) {
        if (child.optimal) {
            markOptimalPath(child, (player + 1) % NUM_PLAYERS);
            if (child.optimal_path) {
                node.optimal_path = true;
            }
        }
    }
}

bool isOptimalPath(const Node &node, int player) {
    if (node.children.empty()) {
        return true;
    }
    for (const auto &child: node.children) {
        if (child.optimal && isOptimalPath(child, (player + 1) % NUM_PLAYERS)) {
            return true;
        }
    }
    return false;
}

std::string colorLabel(const Node &node, int depth, bool is_root) {
    std::string label = "(";
    for (int i = 0; i < NUM_PLAYERS; ++i) {
        label += "{";
        for (size_t j = 0; j < node.values[i].size(); ++j) {
            if (i == (depth - 1) % NUM_PLAYERS && !is_root) {
                label += "<font color=\"blue\">" + std::to_string(node.values[i][j]) + "</font>";
            } else {
                label += std::to_string(node.values[i][j]);
            }
            if (j < node.values[i].size() - 1) {
                label += ", ";
            }
        }
        label += "}";
        if (i < 2) {
            label += ", ";
        }
    }
    label += ")";
    return label;
}

void graphOutput(const Node &node, const std::string &parent, int depth, int &node_id,
                 std::ofstream &output) {
    std::string node_name = "node" + std::to_string(node_id++);
    if (!parent.empty()) {
        output << parent << " -> " << node_name;
        if (node.optimal) {
            if (node.optimal_path) {
                output << " [color=\"red\"]";
            } else {
                output << " [color=\"green\"]";
            }
        }
        output << ";\n";
    }
    bool is_root = parent.empty();
    std::string label = colorLabel(node, depth, is_root);
    output << node_name << " [label=<" << label << ">];\n";
    output << "{ rank = same; level" << depth << "; " << node_name << "; }\n";

    int next_depth = depth + 1;
    for (const auto &child: node.children) {
        graphOutput(child, node_name, next_depth, node_id, output);
    }
}

int main() {
    std::random_device rd;
    std::mt19937 gen(rd());
    Node root;
    treeGenerator(root, 0, DEPTH, gen);
    backInduction(root, 0);
    markOptimalPath(root, 0);
    std::cout << "Game values for each player: " << std::endl;
    for (int player = 0; player < NUM_PLAYERS; ++player) {
        std::cout << "Player " << player + 1 << ": ";
        for (const auto &value: root.values[player]) {
            std::cout << value << " ";
        }
        std::cout << std::endl;
    }
    std::ofstream graph_output("visualisation.dot");
    graph_output << "digraph tree {\n";
    for (int i = 0; i < DEPTH; ++i) {
        graph_output << "level" << i << " [label=\"Player " << (i % NUM_PLAYERS) + 1 << "\", shape=plaintext];\n";
    }
    int node_id = 0;
    graphOutput(root, "", 0, node_id, graph_output);
    graph_output << "}\n";
    graph_output.close();
    system("dot -Tpdf visualisation.dot -o visualisation.pdf");
    return 0;
}


