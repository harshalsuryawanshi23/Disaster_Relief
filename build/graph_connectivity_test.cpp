#include <iostream>
#include <vector>
#include "D:/SY/sem2/DS/cpp/core/DataLoader.h"
#include "D:/SY/sem2/DS/cpp/core/Graph.h"
#include "D:/SY/sem2/DS/cpp/ds/UnionFind.h"
#include "D:/SY/sem2/DS/cpp/ds/SegmentTree.h"

int main() {
    DataLoader loader;
    auto zones = loader.load_zones("D:/SY/sem2/DS/data/india_zones.json");
    auto roads = loader.load_roads("D:/SY/sem2/DS/data/india_roads.json");
    std::vector<double> weights;
    for (const auto& r : roads) weights.push_back(r.distance_km);
    UnionFind uf(11);
    SegmentTree st(weights);
    Graph g(11, &uf, &st);
    for (const auto& r : roads) g.add_road(r.from_id, r.to_id, r.distance_km);
    std::cout << "before 2-1=" << g.zones_connected(2,1) << " 2-8=" << g.zones_connected(2,8) << " 4-8=" << g.zones_connected(4,8) << "\n";
    g.block_road(8);
    std::cout << "after 2-1=" << g.zones_connected(2,1) << " 2-8=" << g.zones_connected(2,8) << " 4-8=" << g.zones_connected(4,8) << " 3-1=" << g.zones_connected(3,1) << "\n";
    return 0;
}
