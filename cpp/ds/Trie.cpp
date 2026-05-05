#include "Trie.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <utility>

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

}  // namespace

TrieNode::TrieNode() : children(), is_end(false), zone_id(-1), frequency(0), full_name() {}

Trie::Trie() : root(new TrieNode()) {}

Trie::~Trie() {
    delete_tree(root);
}

void Trie::insert(const std::string& name, int zone_id) {
    if (name.empty()) {
        throw std::invalid_argument("Trie cannot insert an empty name");
    }

    TrieNode* current = root;
    for (char ch : name) {
        TrieNode*& next = current->children[ch];
        if (next == nullptr) {
            next = new TrieNode();
        }
        current = next;
    }

    current->is_end = true;
    current->zone_id = zone_id;
    current->full_name = name;
}

bool Trie::search(const std::string& name) {
    const TrieNode* node = traverse(name);
    return node != nullptr && node->is_end;
}

bool Trie::starts_with(const std::string& prefix) {
    return traverse(prefix) != nullptr;
}

std::vector<std::string> Trie::autocomplete(const std::string& prefix, int limit) {
    if (limit <= 0) {
        return {};
    }

    const TrieNode* node = traverse(prefix);
    if (node == nullptr) {
        return {};
    }

    std::vector<std::pair<int, std::string>> ranked_results;
    collect_all(node, ranked_results);

    std::stable_sort(ranked_results.begin(), ranked_results.end(),
                     [](const auto& lhs, const auto& rhs) {
                         return lhs.first > rhs.first;
                     });

    if (static_cast<int>(ranked_results.size()) > limit) {
        ranked_results.resize(static_cast<std::size_t>(limit));
    }

    std::vector<std::string> result;
    result.reserve(ranked_results.size());
    for (const auto& entry : ranked_results) {
        result.push_back(entry.second);
    }
    return result;
}

int Trie::get_zone_id(const std::string& name) {
    const TrieNode* node = traverse(name);
    if (node == nullptr || !node->is_end) {
        return -1;
    }
    return node->zone_id;
}

void Trie::increment_frequency(const std::string& name) {
    TrieNode* node = traverse_mutable(name);
    if (node == nullptr || !node->is_end) {
        return;
    }
    ++node->frequency;
}

std::string Trie::export_json() const {
    std::string out = "{\"root\":";
    export_node_json(root, '\0', out);
    out += "}";
    return out;
}

void Trie::collect_all(const TrieNode* node,
                       std::vector<std::pair<int, std::string>>& result) const {
    if (node == nullptr) {
        return;
    }

    std::vector<const TrieNode*> stack = {node};
    while (!stack.empty()) {
        const TrieNode* current = stack.back();
        stack.pop_back();

        if (current->is_end) {
            result.emplace_back(current->frequency, current->full_name);
        }

        std::vector<std::pair<char, TrieNode*>> ordered_children(current->children.begin(),
                                                                 current->children.end());
        std::sort(ordered_children.begin(), ordered_children.end(),
                  [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });

        for (const auto& child : ordered_children) {
            if (child.second != nullptr) {
                stack.push_back(child.second);
            }
        }
    }
}

void Trie::delete_tree(TrieNode* node) {
    if (node == nullptr) {
        return;
    }

    for (auto& child : node->children) {
        delete_tree(child.second);
    }
    delete node;
}

const TrieNode* Trie::traverse(const std::string& key) const {
    const TrieNode* current = root;
    for (char ch : key) {
        auto it = current->children.find(ch);
        if (it == current->children.end() || it->second == nullptr) {
            return nullptr;
        }
        current = it->second;
    }
    return current;
}

TrieNode* Trie::traverse_mutable(const std::string& key) {
    TrieNode* current = root;
    for (char ch : key) {
        auto it = current->children.find(ch);
        if (it == current->children.end() || it->second == nullptr) {
            return nullptr;
        }
        current = it->second;
    }
    return current;
}

void Trie::export_node_json(const TrieNode* node, char edge_label, std::string& out) const {
    out += "{";
    if (edge_label == '\0') {
        out += "\"label\":\"ROOT\",";
    } else {
        out += "\"label\":\"";
        out += json_escape(std::string(1, edge_label));
        out += "\",";
    }
    out += "\"is_end\":";
    out += node->is_end ? "true" : "false";
    out += ",\"zone_id\":";
    out += node->is_end ? std::to_string(node->zone_id) : "null";
    out += ",\"frequency\":";
    out += std::to_string(node->frequency);
    out += ",\"full_name\":";
    if (node->is_end) {
        out += "\"";
        out += json_escape(node->full_name);
        out += "\"";
    } else {
        out += "null";
    }
    out += ",\"children\":[";

    std::vector<std::pair<char, TrieNode*>> ordered_children(node->children.begin(),
                                                             node->children.end());
    std::sort(ordered_children.begin(), ordered_children.end(),
              [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });

    for (std::size_t i = 0; i < ordered_children.size(); ++i) {
        if (i > 0) {
            out += ",";
        }
        export_node_json(ordered_children[i].second, ordered_children[i].first, out);
    }

    out += "]}";
}
