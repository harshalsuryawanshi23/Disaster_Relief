#pragma once

#include <utility>
#include <vector>

#include "Graph.h"
#include "../ds/MinHeap.h"

class Router {
public:
    explicit Router(Graph* g);
    ~Router();

    std::pair<double, std::vector<int>> shortest_path(int source, int dest);
    std::vector<double> all_shortest(int source);

private:
    Graph* graph;
    int num_nodes;
    MinHeap<std::pair<double, int>>* pq;

    int infer_num_nodes() const;
    void validate_node(int node_id) const;
};
