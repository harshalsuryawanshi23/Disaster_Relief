#pragma once

#include <string>
#include <vector>

class SegmentTree {
public:
    explicit SegmentTree(std::vector<double>& weights);

    void update(int road_idx, double new_weight);
    double range_min(int l, int r);
    double range_sum(int l, int r);
    std::string export_json() const;

private:
    std::vector<double> tree;
    std::vector<double> sum_tree;
    std::vector<double> data;
    int n;

    void build(int node, int start, int end);
    void update_helper(int node, int start, int end, int idx, double val);
    double query_helper(int node, int start, int end, int l, int r);
    double sum_helper(int node, int start, int end, int l, int r);
    void validate_query_range(int l, int r) const;
    void validate_index(int idx) const;
};
