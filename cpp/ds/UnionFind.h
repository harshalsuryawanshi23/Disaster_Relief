#pragma once

#include <string>
#include <utility>
#include <vector>

class UnionFind {
public:
    explicit UnionFind(int n);

    void union_zones(int a, int b);
    bool connected(int a, int b);
    int find(int x);
    int get_component_size(int x);
    int component_count() const;
    void block_road(int a, int b);
    std::vector<std::vector<int>> get_all_components();
    std::string export_json() const;

private:
    std::vector<int> parent;
    std::vector<int> rank_arr;
    std::vector<int> size_arr;
    int num_components;
    std::vector<std::pair<int, int>> history;

    int find_root(int x);
    void validate_index(int x) const;
};
