#include "SegmentTree.h"

#include <algorithm>
#include <limits>
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

SegmentTree::SegmentTree(std::vector<double>& weights)
    : tree(weights.empty() ? 1U : static_cast<std::size_t>(4 * weights.size()),
           std::numeric_limits<double>::infinity()),
      sum_tree(weights.empty() ? 1U : static_cast<std::size_t>(4 * weights.size()), 0.0),
      data(weights),
      n(static_cast<int>(weights.size())) {
    if (n > 0) {
        build(1, 0, n - 1);
    }
}

void SegmentTree::update(int road_idx, double new_weight) {
    validate_index(road_idx);
    update_helper(1, 0, n - 1, road_idx, new_weight);
}

double SegmentTree::range_min(int l, int r) {
    validate_query_range(l, r);
    return query_helper(1, 0, n - 1, l, r);
}

double SegmentTree::range_sum(int l, int r) {
    validate_query_range(l, r);
    return sum_helper(1, 0, n - 1, l, r);
}

std::string SegmentTree::export_json() const {
    std::ostringstream out;
    out << "{"
        << "\"n\":" << n << ','
        << "\"data\":[";

    for (std::size_t i = 0; i < data.size(); ++i) {
        if (i > 0) {
            out << ',';
        }
        out << data[i];
    }

    out << "],\"min_tree\":[";
    for (std::size_t i = 1; i < tree.size(); ++i) {
        if (i > 1) {
            out << ',';
        }
        out << tree[i];
    }

    out << "],\"sum_tree\":[";
    for (std::size_t i = 1; i < sum_tree.size(); ++i) {
        if (i > 1) {
            out << ',';
        }
        out << sum_tree[i];
    }

    out << "],\"meta\":\""
        << json_escape("SegmentTree export with min and sum views for road weights")
        << "\"}";
    return out.str();
}

void SegmentTree::build(int node, int start, int end) {
    if (start == end) {
        tree[static_cast<std::size_t>(node)] = data[static_cast<std::size_t>(start)];
        sum_tree[static_cast<std::size_t>(node)] = data[static_cast<std::size_t>(start)];
        return;
    }

    const int mid = start + (end - start) / 2;
    build(node * 2, start, mid);
    build(node * 2 + 1, mid + 1, end);

    tree[static_cast<std::size_t>(node)] =
        std::min(tree[static_cast<std::size_t>(node * 2)],
                 tree[static_cast<std::size_t>(node * 2 + 1)]);
    sum_tree[static_cast<std::size_t>(node)] =
        sum_tree[static_cast<std::size_t>(node * 2)] +
        sum_tree[static_cast<std::size_t>(node * 2 + 1)];
}

void SegmentTree::update_helper(int node, int start, int end, int idx, double val) {
    if (start == end) {
        data[static_cast<std::size_t>(idx)] = val;
        tree[static_cast<std::size_t>(node)] = val;
        sum_tree[static_cast<std::size_t>(node)] = val;
        return;
    }

    const int mid = start + (end - start) / 2;
    if (idx <= mid) {
        update_helper(node * 2, start, mid, idx, val);
    } else {
        update_helper(node * 2 + 1, mid + 1, end, idx, val);
    }

    tree[static_cast<std::size_t>(node)] =
        std::min(tree[static_cast<std::size_t>(node * 2)],
                 tree[static_cast<std::size_t>(node * 2 + 1)]);
    sum_tree[static_cast<std::size_t>(node)] =
        sum_tree[static_cast<std::size_t>(node * 2)] +
        sum_tree[static_cast<std::size_t>(node * 2 + 1)];
}

double SegmentTree::query_helper(int node, int start, int end, int l, int r) {
    if (r < start || end < l) {
        return std::numeric_limits<double>::infinity();
    }
    if (l <= start && end <= r) {
        return tree[static_cast<std::size_t>(node)];
    }

    const int mid = start + (end - start) / 2;
    return std::min(query_helper(node * 2, start, mid, l, r),
                    query_helper(node * 2 + 1, mid + 1, end, l, r));
}

double SegmentTree::sum_helper(int node, int start, int end, int l, int r) {
    if (r < start || end < l) {
        return 0.0;
    }
    if (l <= start && end <= r) {
        return sum_tree[static_cast<std::size_t>(node)];
    }

    const int mid = start + (end - start) / 2;
    return sum_helper(node * 2, start, mid, l, r) +
           sum_helper(node * 2 + 1, mid + 1, end, l, r);
}

void SegmentTree::validate_query_range(int l, int r) const {
    if (n == 0) {
        throw std::out_of_range("SegmentTree query on empty data");
    }
    if (l < 0 || r < 0 || l > r || r >= n) {
        throw std::out_of_range("SegmentTree query range out of bounds");
    }
}

void SegmentTree::validate_index(int idx) const {
    if (n == 0) {
        throw std::out_of_range("SegmentTree update on empty data");
    }
    if (idx < 0 || idx >= n) {
        throw std::out_of_range("SegmentTree index out of bounds");
    }
}
