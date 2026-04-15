#pragma once

#include <string>
#include <utility>
#include <vector>

#include "../ds/SegmentTree.h"
#include "../ds/UnionFind.h"

struct Road {
    int id = -1;
    int from_id = -1;
    int to_id = -1;
    double distance_km = 0.0;
    double current_weight = 0.0;
    bool is_blocked = false;
};

class Graph {
public:
    Graph(int n, UnionFind* uf, SegmentTree* st);

    void add_road(int u, int v, double dist);
    void block_road(int road_idx);
    bool zones_connected(int a, int b);
    std::vector<int> get_neighbors(int zone_id) const;
    double road_weight(int u, int v) const;
    std::string export_json() const;

private:
    int num_nodes;
    std::vector<std::vector<std::pair<int, double>>> adj;
    std::vector<Road> roads;
    UnionFind* connectivity;
    SegmentTree* road_weights;

    void validate_node(int node_id) const;
    int find_road_index(int u, int v) const;
};
