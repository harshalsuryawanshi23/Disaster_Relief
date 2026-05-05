#pragma once

#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

template <typename T>
struct SkipNode {
    double key;
    T value;
    int level;
    std::vector<SkipNode<T>*> forward;

    SkipNode(double k, T v, int lvl);
};

template <typename T>
class SkipList {
public:
    static const int MAX_LEVEL = 16;

    explicit SkipList(float p = 0.5f);
    ~SkipList();

    SkipList(const SkipList&) = delete;
    SkipList& operator=(const SkipList&) = delete;

    void insert(double key, T value);
    bool remove(double key);
    SkipNode<T>* search(double key);
    std::vector<T> range_query(double lo, double hi);
    std::vector<T> get_top_k(int k);
    std::string export_json() const;

private:
    float probability;
    int current_level;
    SkipNode<T>* header;
    int node_count;
    std::mt19937 rng;

    int random_level();
    void delete_all();
};

template <typename T>
SkipNode<T>::SkipNode(double k, T v, int lvl)
    : key(k), value(std::move(v)), level(lvl), forward(static_cast<std::size_t>(lvl + 1), nullptr) {}

template <typename T>
SkipList<T>::SkipList(float p)
    : probability(p),
      current_level(0),
      header(new SkipNode<T>(-std::numeric_limits<double>::infinity(), T{}, MAX_LEVEL)),
      node_count(0),
      rng(std::random_device{}()) {
    if (p <= 0.0f || p >= 1.0f) {
        throw std::invalid_argument("SkipList probability must be between 0 and 1");
    }
}

template <typename T>
SkipList<T>::~SkipList() {
    delete_all();
}

template <typename T>
void SkipList<T>::insert(double key, T value) {
    std::vector<SkipNode<T>*> update(static_cast<std::size_t>(MAX_LEVEL + 1), nullptr);
    SkipNode<T>* current = header;

    for (int level = current_level; level >= 0; --level) {
        while (current->forward[static_cast<std::size_t>(level)] != nullptr &&
               current->forward[static_cast<std::size_t>(level)]->key < key) {
            current = current->forward[static_cast<std::size_t>(level)];
        }
        update[static_cast<std::size_t>(level)] = current;
    }

    current = current->forward[0];
    if (current != nullptr && current->key == key) {
        current->value = std::move(value);
        return;
    }

    const int new_level = random_level();
    if (new_level > current_level) {
        for (int level = current_level + 1; level <= new_level; ++level) {
            update[static_cast<std::size_t>(level)] = header;
        }
        current_level = new_level;
    }

    SkipNode<T>* node = new SkipNode<T>(key, std::move(value), new_level);
    for (int level = 0; level <= new_level; ++level) {
        node->forward[static_cast<std::size_t>(level)] =
            update[static_cast<std::size_t>(level)]->forward[static_cast<std::size_t>(level)];
        update[static_cast<std::size_t>(level)]->forward[static_cast<std::size_t>(level)] = node;
    }
    ++node_count;
}

template <typename T>
bool SkipList<T>::remove(double key) {
    std::vector<SkipNode<T>*> update(static_cast<std::size_t>(MAX_LEVEL + 1), nullptr);
    SkipNode<T>* current = header;

    for (int level = current_level; level >= 0; --level) {
        while (current->forward[static_cast<std::size_t>(level)] != nullptr &&
               current->forward[static_cast<std::size_t>(level)]->key < key) {
            current = current->forward[static_cast<std::size_t>(level)];
        }
        update[static_cast<std::size_t>(level)] = current;
    }

    current = current->forward[0];
    if (current == nullptr || current->key != key) {
        return false;
    }

    for (int level = 0; level <= current_level; ++level) {
        if (update[static_cast<std::size_t>(level)]->forward[static_cast<std::size_t>(level)] !=
            current) {
            continue;
        }
        update[static_cast<std::size_t>(level)]->forward[static_cast<std::size_t>(level)] =
            current->forward[static_cast<std::size_t>(level)];
    }

    delete current;
    --node_count;

    while (current_level > 0 &&
           header->forward[static_cast<std::size_t>(current_level)] == nullptr) {
        --current_level;
    }
    return true;
}

template <typename T>
SkipNode<T>* SkipList<T>::search(double key) {
    SkipNode<T>* current = header;
    for (int level = current_level; level >= 0; --level) {
        while (current->forward[static_cast<std::size_t>(level)] != nullptr &&
               current->forward[static_cast<std::size_t>(level)]->key < key) {
            current = current->forward[static_cast<std::size_t>(level)];
        }
    }

    current = current->forward[0];
    if (current != nullptr && current->key == key) {
        return current;
    }
    return nullptr;
}

template <typename T>
std::vector<T> SkipList<T>::range_query(double lo, double hi) {
    if (lo > hi) {
        std::swap(lo, hi);
    }

    std::vector<T> result;
    SkipNode<T>* current = header;
    for (int level = current_level; level >= 0; --level) {
        while (current->forward[static_cast<std::size_t>(level)] != nullptr &&
               current->forward[static_cast<std::size_t>(level)]->key < lo) {
            current = current->forward[static_cast<std::size_t>(level)];
        }
    }

    current = current->forward[0];
    while (current != nullptr && current->key <= hi) {
        result.push_back(current->value);
        current = current->forward[0];
    }
    return result;
}

template <typename T>
std::vector<T> SkipList<T>::get_top_k(int k) {
    std::vector<T> result;
    if (k <= 0 || node_count == 0) {
        return result;
    }

    std::vector<SkipNode<T>*> ordered_nodes;
    ordered_nodes.reserve(static_cast<std::size_t>(node_count));
    SkipNode<T>* current = header->forward[0];
    while (current != nullptr) {
        ordered_nodes.push_back(current);
        current = current->forward[0];
    }

    for (auto it = ordered_nodes.rbegin(); it != ordered_nodes.rend() && k > 0; ++it, --k) {
        result.push_back((*it)->value);
    }
    return result;
}

template <typename T>
std::string SkipList<T>::export_json() const {
    std::ostringstream out;
    out << "{"
        << "\"current_level\":" << current_level << ','
        << "\"node_count\":" << node_count << ','
        << "\"levels\":[";

    for (int level = current_level; level >= 0; --level) {
        if (level != current_level) {
            out << ',';
        }
        out << "{"
            << "\"level\":" << level << ','
            << "\"nodes\":[";

        bool first_node = true;
        SkipNode<T>* current = header->forward[static_cast<std::size_t>(level)];
        while (current != nullptr) {
            if (!first_node) {
                out << ',';
            }
            std::ostringstream value_stream;
            value_stream << current->value;
            out << "{"
                << "\"key\":" << current->key << ','
                << "\"value\":\"" << value_stream.str() << "\","
                << "\"height\":" << current->level
                << "}";
            first_node = false;
            current = current->forward[static_cast<std::size_t>(level)];
        }

        out << "]}";
    }

    out << "]}";
    return out.str();
}

template <typename T>
int SkipList<T>::random_level() {
    std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
    int level = 0;
    while (distribution(rng) < probability && level < MAX_LEVEL) {
        ++level;
    }
    return level;
}

template <typename T>
void SkipList<T>::delete_all() {
    SkipNode<T>* current = header;
    while (current != nullptr) {
        SkipNode<T>* next = current->forward[0];
        delete current;
        current = next;
    }
}
