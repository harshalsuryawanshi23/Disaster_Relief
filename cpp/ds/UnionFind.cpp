#include "UnionFind.h"

#include <map>
#include <sstream>
#include <stdexcept>

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

UnionFind::UnionFind(int n)
    : parent(static_cast<std::size_t>(n)),
      rank_arr(static_cast<std::size_t>(n), 0),
      size_arr(static_cast<std::size_t>(n), 1),
      num_components(n),
      history() {
    if (n < 0) {
        throw std::invalid_argument("UnionFind size cannot be negative");
    }

    for (int i = 0; i < n; ++i) {
        parent[static_cast<std::size_t>(i)] = i;
    }
}

void UnionFind::union_zones(int a, int b) {
    validate_index(a);
    validate_index(b);

    int root_a = find_root(a);
    int root_b = find_root(b);
    if (root_a == root_b) {
        history.emplace_back(a, b);
        return;
    }

    if (rank_arr[static_cast<std::size_t>(root_a)] <
        rank_arr[static_cast<std::size_t>(root_b)]) {
        std::swap(root_a, root_b);
    }

    parent[static_cast<std::size_t>(root_b)] = root_a;
    size_arr[static_cast<std::size_t>(root_a)] += size_arr[static_cast<std::size_t>(root_b)];

    if (rank_arr[static_cast<std::size_t>(root_a)] ==
        rank_arr[static_cast<std::size_t>(root_b)]) {
        ++rank_arr[static_cast<std::size_t>(root_a)];
    }

    --num_components;
    history.emplace_back(a, b);
}

bool UnionFind::connected(int a, int b) {
    validate_index(a);
    validate_index(b);
    return find_root(a) == find_root(b);
}

int UnionFind::find(int x) {
    validate_index(x);
    return find_root(x);
}

int UnionFind::get_component_size(int x) {
    validate_index(x);
    return size_arr[static_cast<std::size_t>(find_root(x))];
}

int UnionFind::component_count() const {
    return num_components;
}

void UnionFind::block_road(int a, int b) {
    validate_index(a);
    validate_index(b);
    history.emplace_back(a, b);
}

std::vector<std::vector<int>> UnionFind::get_all_components() {
    std::map<int, std::vector<int>> grouped;
    for (std::size_t i = 0; i < parent.size(); ++i) {
        grouped[find_root(static_cast<int>(i))].push_back(static_cast<int>(i));
    }

    std::vector<std::vector<int>> components;
    components.reserve(grouped.size());
    for (auto& entry : grouped) {
        components.push_back(entry.second);
    }
    return components;
}

std::string UnionFind::export_json() const {
    std::ostringstream out;
    out << "{"
        << "\"num_components\":" << num_components << ','
        << "\"parent\":[";

    for (std::size_t i = 0; i < parent.size(); ++i) {
        if (i > 0) {
            out << ',';
        }
        out << parent[i];
    }

    out << "],\"rank\":[";
    for (std::size_t i = 0; i < rank_arr.size(); ++i) {
        if (i > 0) {
            out << ',';
        }
        out << rank_arr[i];
    }

    out << "],\"size\":[";
    for (std::size_t i = 0; i < size_arr.size(); ++i) {
        if (i > 0) {
            out << ',';
        }
        out << size_arr[i];
    }

    out << "],\"history\":[";
    for (std::size_t i = 0; i < history.size(); ++i) {
        if (i > 0) {
            out << ',';
        }
        out << "{\"a\":" << history[i].first << ",\"b\":" << history[i].second << "}";
    }

    out << "],\"meta\":\""
        << json_escape("UnionFind export with parent, rank, size, and operation history")
        << "\"}";
    return out.str();
}

int UnionFind::find_root(int x) {
    if (parent[static_cast<std::size_t>(x)] != x) {
        parent[static_cast<std::size_t>(x)] = find_root(parent[static_cast<std::size_t>(x)]);
    }
    return parent[static_cast<std::size_t>(x)];
}

void UnionFind::validate_index(int x) const {
    if (x < 0 || static_cast<std::size_t>(x) >= parent.size()) {
        throw std::out_of_range("UnionFind index out of range");
    }
}
