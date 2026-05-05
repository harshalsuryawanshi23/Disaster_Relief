#pragma once

#include <string>
#include <unordered_map>
#include <vector>

struct FibNode {
    int zone_id;
    double key;
    int degree;
    bool marked;
    FibNode* parent;
    FibNode* child;
    FibNode* left;
    FibNode* right;

    FibNode(int id, double k);
};

class FibonacciHeap {
public:
    FibonacciHeap();
    ~FibonacciHeap();

    FibonacciHeap(const FibonacciHeap&) = delete;
    FibonacciHeap& operator=(const FibonacciHeap&) = delete;

    FibNode* insert(int zone_id, double key);
    FibNode* get_min();
    FibNode* extract_min();
    void decrease_key(int zone_id, double new_key);
    void delete_node(int zone_id);
    bool is_empty() const;
    int size() const;
    std::string export_json() const;

private:
    FibNode* min_node;
    int total_nodes;
    std::unordered_map<int, FibNode*> node_map;

    void link(FibNode* y, FibNode* x);
    void consolidate();
    void cut(FibNode* x, FibNode* y);
    void cascading_cut(FibNode* y);
    void add_to_root_list(FibNode* node);
    std::vector<FibNode*> get_snapshot() const;
};
