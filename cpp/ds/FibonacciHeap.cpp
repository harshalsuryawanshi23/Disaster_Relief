#include "FibonacciHeap.h"

#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace {

std::string json_escape(const std::string& input) {
    std::ostringstream out;
    for (char ch : input) {
        switch (ch) {
            case '\\':
                out << "\\\\";
                break;
            case '"':
                out << "\\\"";
                break;
            case '\n':
                out << "\\n";
                break;
            case '\r':
                out << "\\r";
                break;
            case '\t':
                out << "\\t";
                break;
            default:
                out << ch;
                break;
        }
    }
    return out.str();
}

void append_node_json(std::ostringstream& out, FibNode* node) {
    out << "{"
        << "\"zone_id\":" << node->zone_id << ','
        << "\"key\":" << node->key << ','
        << "\"degree\":" << node->degree << ','
        << "\"marked\":" << (node->marked ? "true" : "false") << ','
        << "\"parent\":" << (node->parent ? std::to_string(node->parent->zone_id) : "null")
        << ','
        << "\"children\":[";

    bool first_child = true;
    if (node->child != nullptr) {
        FibNode* child = node->child;
        do {
            if (!first_child) {
                out << ',';
            }
            append_node_json(out, child);
            first_child = false;
            child = child->right;
        } while (child != node->child);
    }

    out << "]}";
}

}  // namespace

FibNode::FibNode(int id, double k)
    : zone_id(id),
      key(k),
      degree(0),
      marked(false),
      parent(nullptr),
      child(nullptr),
      left(this),
      right(this) {}

FibonacciHeap::FibonacciHeap() : min_node(nullptr), total_nodes(0) {}

FibonacciHeap::~FibonacciHeap() {
    std::unordered_set<FibNode*> unique_nodes;
    unique_nodes.reserve(node_map.size());
    for (const auto& entry : node_map) {
        unique_nodes.insert(entry.second);
    }
    for (FibNode* node : unique_nodes) {
        delete node;
    }
}

FibNode* FibonacciHeap::insert(int zone_id, double key) {
    auto existing = node_map.find(zone_id);
    if (existing != node_map.end()) {
        if (key <= existing->second->key) {
            decrease_key(zone_id, key);
            return existing->second;
        }
        delete_node(zone_id);
    }

    FibNode* node = new FibNode(zone_id, key);
    add_to_root_list(node);
    node_map[zone_id] = node;
    ++total_nodes;

    if (min_node == nullptr || node->key < min_node->key) {
        min_node = node;
    }

    return node;
}

FibNode* FibonacciHeap::get_min() {
    return min_node;
}

FibNode* FibonacciHeap::extract_min() {
    FibNode* z = min_node;
    if (z == nullptr) {
        return nullptr;
    }

    if (z->child != nullptr) {
        std::vector<FibNode*> children;
        FibNode* child = z->child;
        do {
            children.push_back(child);
            child = child->right;
        } while (child != z->child);

        for (FibNode* current : children) {
            current->left->right = current->right;
            current->right->left = current->left;
            current->left = current;
            current->right = current;
            current->parent = nullptr;
            current->marked = false;
            add_to_root_list(current);
        }
        z->child = nullptr;
        z->degree = 0;
    }

    if (z->right == z) {
        min_node = nullptr;
    } else {
        z->left->right = z->right;
        z->right->left = z->left;
        min_node = z->right;
        consolidate();
    }

    z->left = z;
    z->right = z;
    z->parent = nullptr;
    z->child = nullptr;
    z->marked = false;

    node_map.erase(z->zone_id);
    --total_nodes;
    return z;
}

void FibonacciHeap::decrease_key(int zone_id, double new_key) {
    auto it = node_map.find(zone_id);
    if (it == node_map.end()) {
        throw std::out_of_range("Zone not present in FibonacciHeap");
    }

    FibNode* x = it->second;
    if (new_key > x->key) {
        throw std::invalid_argument("New key is greater than current key");
    }

    x->key = new_key;
    FibNode* y = x->parent;
    if (y != nullptr && x->key < y->key) {
        cut(x, y);
        cascading_cut(y);
    }

    if (min_node == nullptr || x->key < min_node->key) {
        min_node = x;
    }
}

void FibonacciHeap::delete_node(int zone_id) {
    auto it = node_map.find(zone_id);
    if (it == node_map.end()) {
        return;
    }

    decrease_key(zone_id, -std::numeric_limits<double>::infinity());
    FibNode* removed = extract_min();
    delete removed;
}

bool FibonacciHeap::is_empty() const {
    return total_nodes == 0;
}

int FibonacciHeap::size() const {
    return total_nodes;
}

std::string FibonacciHeap::export_json() const {
    std::ostringstream out;
    out << "{"
        << "\"total_nodes\":" << total_nodes << ','
        << "\"min_zone_id\":"
        << (min_node != nullptr ? std::to_string(min_node->zone_id) : "null") << ','
        << "\"min_key\":"
        << (min_node != nullptr ? std::to_string(min_node->key) : "null") << ','
        << "\"roots\":[";

    bool first_root = true;
    if (min_node != nullptr) {
        FibNode* current = min_node;
        do {
            if (!first_root) {
                out << ',';
            }
            append_node_json(out, current);
            first_root = false;
            current = current->right;
        } while (current != min_node);
    }

    out << "],\"snapshot\":[";

    bool first_snapshot = true;
    for (FibNode* node : get_snapshot()) {
        if (!first_snapshot) {
            out << ',';
        }
        out << "{"
            << "\"zone_id\":" << node->zone_id << ','
            << "\"key\":" << node->key << ','
            << "\"degree\":" << node->degree << ','
            << "\"parent\":"
            << (node->parent ? std::to_string(node->parent->zone_id) : "null")
            << ",\"left\":" << node->left->zone_id
            << ",\"right\":" << node->right->zone_id
            << ",\"marked\":" << (node->marked ? "true" : "false")
            << "}";
        first_snapshot = false;
    }

    out << "],\"meta\":\""
        << json_escape("Fibonacci Heap export for frontend visualization")
        << "\"}";
    return out.str();
}

void FibonacciHeap::link(FibNode* y, FibNode* x) {
    y->left->right = y->right;
    y->right->left = y->left;

    y->parent = x;
    y->marked = false;

    if (x->child == nullptr) {
        x->child = y;
        y->left = y;
        y->right = y;
    } else {
        FibNode* child = x->child;
        y->right = child;
        y->left = child->left;
        child->left->right = y;
        child->left = y;
    }

    ++x->degree;
}

void FibonacciHeap::consolidate() {
    if (min_node == nullptr) {
        return;
    }

    std::vector<FibNode*> roots;
    FibNode* current = min_node;
    do {
        roots.push_back(current);
        current = current->right;
    } while (current != min_node);

    std::size_t array_size = static_cast<std::size_t>(
        std::max(8, static_cast<int>(std::log2(std::max(2, total_nodes))) + 8));
    std::vector<FibNode*> degree_table(array_size, nullptr);

    for (FibNode* root : roots) {
        FibNode* x = root;
        int degree = x->degree;

        while (static_cast<std::size_t>(degree) >= degree_table.size()) {
            degree_table.resize(degree_table.size() * 2, nullptr);
        }

        while (degree_table[degree] != nullptr) {
            FibNode* y = degree_table[degree];
            if (y == x) {
                break;
            }
            if (y->key < x->key) {
                FibNode* temp = x;
                x = y;
                y = temp;
            }
            link(y, x);
            degree_table[degree] = nullptr;
            degree = x->degree;
            while (static_cast<std::size_t>(degree) >= degree_table.size()) {
                degree_table.resize(degree_table.size() * 2, nullptr);
            }
        }

        degree_table[degree] = x;
    }

    min_node = nullptr;
    for (FibNode* node : degree_table) {
        if (node == nullptr) {
            continue;
        }

        node->left = node;
        node->right = node;
        if (min_node == nullptr) {
            min_node = node;
        } else {
            add_to_root_list(node);
            if (node->key < min_node->key) {
                min_node = node;
            }
        }
    }
}

void FibonacciHeap::cut(FibNode* x, FibNode* y) {
    if (x->right == x) {
        y->child = nullptr;
    } else {
        x->left->right = x->right;
        x->right->left = x->left;
        if (y->child == x) {
            y->child = x->right;
        }
    }

    --y->degree;
    x->left = x;
    x->right = x;
    x->parent = nullptr;
    x->marked = false;
    add_to_root_list(x);
}

void FibonacciHeap::cascading_cut(FibNode* y) {
    FibNode* z = y->parent;
    if (z == nullptr) {
        return;
    }

    if (!y->marked) {
        y->marked = true;
        return;
    }

    cut(y, z);
    cascading_cut(z);
}

void FibonacciHeap::add_to_root_list(FibNode* node) {
    if (min_node == nullptr) {
        node->left = node;
        node->right = node;
        min_node = node;
        return;
    }

    node->right = min_node;
    node->left = min_node->left;
    min_node->left->right = node;
    min_node->left = node;
}

std::vector<FibNode*> FibonacciHeap::get_snapshot() const {
    std::vector<FibNode*> snapshot;
    if (min_node == nullptr) {
        return snapshot;
    }

    std::vector<FibNode*> stack;
    std::unordered_set<FibNode*> visited;

    FibNode* root = min_node;
    do {
        stack.push_back(root);
        root = root->right;
    } while (root != min_node);

    while (!stack.empty()) {
        FibNode* node = stack.back();
        stack.pop_back();
        if (!visited.insert(node).second) {
            continue;
        }

        snapshot.push_back(node);
        if (node->child != nullptr) {
            FibNode* child = node->child;
            do {
                stack.push_back(child);
                child = child->right;
            } while (child != node->child);
        }
    }

    return snapshot;
}
