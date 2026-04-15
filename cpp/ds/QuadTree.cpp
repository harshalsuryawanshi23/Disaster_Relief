#include "QuadTree.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {

double euclidean_distance(double lat1, double lng1, double lat2, double lng2) {
    const double d_lat = lat1 - lat2;
    const double d_lng = lng1 - lng2;
    return std::sqrt(d_lat * d_lat + d_lng * d_lng);
}

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

bool BoundingBox::contains(Point p) const {
    return p.lat >= min_lat && p.lat <= max_lat && p.lng >= min_lng && p.lng <= max_lng;
}

bool BoundingBox::intersects(BoundingBox other) const {
    return !(other.min_lat > max_lat || other.max_lat < min_lat || other.min_lng > max_lng ||
             other.max_lng < min_lng);
}

double BoundingBox::min_distance_to(Point p) const {
    const double clamped_lat = std::max(min_lat, std::min(max_lat, p.lat));
    const double clamped_lng = std::max(min_lng, std::min(max_lng, p.lng));
    return euclidean_distance(clamped_lat, clamped_lng, p.lat, p.lng);
}

QTNode::QTNode(BoundingBox bb, int cap)
    : boundary(bb), capacity(cap), points(), nw(nullptr), ne(nullptr), sw(nullptr), se(nullptr) {}

bool QTNode::is_leaf() const {
    return nw == nullptr && ne == nullptr && sw == nullptr && se == nullptr;
}

QuadTree::QuadTree(BoundingBox world_boundary) : root(new QTNode(world_boundary)) {}

QuadTree::~QuadTree() {
    delete_subtree(root);
}

void QuadTree::insert(Point p) {
    if (!root->boundary.contains(p)) {
        throw std::out_of_range("Point lies outside the QuadTree boundary");
    }
    insert_helper(root, p);
}

Point QuadTree::nearest_neighbor(Point query) const {
    if (root == nullptr) {
        throw std::runtime_error("QuadTree root is null");
    }

    Point best;
    double best_dist = std::numeric_limits<double>::infinity();
    nearest_helper(root, query, best, best_dist);

    if (!std::isfinite(best_dist)) {
        throw std::runtime_error("QuadTree is empty");
    }
    return best;
}

std::vector<Point> QuadTree::range_search(BoundingBox area) const {
    std::vector<Point> result;
    range_query_helper(root, area, result);
    return result;
}

void QuadTree::remove_point(int entity_id) {
    remove_helper(root, entity_id);
}

std::string QuadTree::export_json() const {
    std::string out = "{\"root\":";
    if (root == nullptr) {
        out += "null";
    } else {
        export_node_json(root, out);
    }
    out += ",\"meta\":\"";
    out += json_escape("QuadTree export with recursive quadrant structure");
    out += "\"}";
    return out;
}

void QuadTree::subdivide(QTNode* node) {
    const double mid_lat = (node->boundary.min_lat + node->boundary.max_lat) / 2.0;
    const double mid_lng = (node->boundary.min_lng + node->boundary.max_lng) / 2.0;

    node->nw = new QTNode({mid_lat, node->boundary.max_lat, node->boundary.min_lng, mid_lng},
                          node->capacity);
    node->ne = new QTNode({mid_lat, node->boundary.max_lat, mid_lng, node->boundary.max_lng},
                          node->capacity);
    node->sw = new QTNode({node->boundary.min_lat, mid_lat, node->boundary.min_lng, mid_lng},
                          node->capacity);
    node->se = new QTNode({node->boundary.min_lat, mid_lat, mid_lng, node->boundary.max_lng},
                          node->capacity);

    std::vector<Point> existing_points = node->points;
    node->points.clear();
    for (const Point& point : existing_points) {
        insert_helper(node, point);
    }
}

void QuadTree::insert_helper(QTNode* node, Point p) {
    if (!node->boundary.contains(p)) {
        return;
    }

    if (node->is_leaf() && static_cast<int>(node->points.size()) < node->capacity) {
        node->points.push_back(p);
        return;
    }

    if (node->is_leaf()) {
        subdivide(node);
    }

    if (node->nw->boundary.contains(p)) {
        insert_helper(node->nw, p);
    } else if (node->ne->boundary.contains(p)) {
        insert_helper(node->ne, p);
    } else if (node->sw->boundary.contains(p)) {
        insert_helper(node->sw, p);
    } else if (node->se->boundary.contains(p)) {
        insert_helper(node->se, p);
    } else {
        node->points.push_back(p);
    }
}

void QuadTree::range_query_helper(QTNode* node,
                                  BoundingBox range,
                                  std::vector<Point>& result) const {
    if (node == nullptr || !node->boundary.intersects(range)) {
        return;
    }

    for (const Point& point : node->points) {
        if (range.contains(point)) {
            result.push_back(point);
        }
    }

    if (node->is_leaf()) {
        return;
    }

    range_query_helper(node->nw, range, result);
    range_query_helper(node->ne, range, result);
    range_query_helper(node->sw, range, result);
    range_query_helper(node->se, range, result);
}

void QuadTree::nearest_helper(QTNode* node,
                              Point target,
                              Point& best,
                              double& best_dist) const {
    if (node == nullptr) {
        return;
    }

    if (node->boundary.min_distance_to(target) > best_dist) {
        return;
    }

    for (const Point& point : node->points) {
        const double distance = euclidean_distance(point.lat, point.lng, target.lat, target.lng);
        if (distance < best_dist) {
            best_dist = distance;
            best = point;
        }
    }

    if (node->is_leaf()) {
        return;
    }

    std::vector<QTNode*> children = {node->nw, node->ne, node->sw, node->se};
    std::sort(children.begin(), children.end(), [&target](QTNode* lhs, QTNode* rhs) {
        const double left_dist =
            lhs != nullptr ? lhs->boundary.min_distance_to(target)
                           : std::numeric_limits<double>::infinity();
        const double right_dist =
            rhs != nullptr ? rhs->boundary.min_distance_to(target)
                           : std::numeric_limits<double>::infinity();
        return left_dist < right_dist;
    });

    for (QTNode* child : children) {
        nearest_helper(child, target, best, best_dist);
    }
}

bool QuadTree::remove_helper(QTNode* node, int entity_id) {
    if (node == nullptr) {
        return false;
    }

    auto it = std::remove_if(node->points.begin(), node->points.end(),
                             [entity_id](const Point& point) {
                                 return point.entity_id == entity_id;
                             });
    if (it != node->points.end()) {
        node->points.erase(it, node->points.end());
        return true;
    }

    if (node->is_leaf()) {
        return false;
    }

    return remove_helper(node->nw, entity_id) || remove_helper(node->ne, entity_id) ||
           remove_helper(node->sw, entity_id) || remove_helper(node->se, entity_id);
}

void QuadTree::export_node_json(QTNode* node, std::string& out) const {
    out += "{";
    out += "\"boundary\":{";
    out += "\"min_lat\":" + std::to_string(node->boundary.min_lat) + ",";
    out += "\"max_lat\":" + std::to_string(node->boundary.max_lat) + ",";
    out += "\"min_lng\":" + std::to_string(node->boundary.min_lng) + ",";
    out += "\"max_lng\":" + std::to_string(node->boundary.max_lng) + "},";
    out += "\"capacity\":" + std::to_string(node->capacity) + ",";
    out += "\"points\":[";

    for (std::size_t i = 0; i < node->points.size(); ++i) {
        if (i > 0) {
            out += ",";
        }
        out += "{";
        out += "\"entity_id\":" + std::to_string(node->points[i].entity_id) + ",";
        out += "\"label\":\"" + json_escape(node->points[i].label) + "\",";
        out += "\"lat\":" + std::to_string(node->points[i].lat) + ",";
        out += "\"lng\":" + std::to_string(node->points[i].lng);
        out += "}";
    }

    out += "],";
    out += "\"children\":{";
    out += "\"nw\":";
    if (node->nw != nullptr) {
        export_node_json(node->nw, out);
    } else {
        out += "null";
    }
    out += ",\"ne\":";
    if (node->ne != nullptr) {
        export_node_json(node->ne, out);
    } else {
        out += "null";
    }
    out += ",\"sw\":";
    if (node->sw != nullptr) {
        export_node_json(node->sw, out);
    } else {
        out += "null";
    }
    out += ",\"se\":";
    if (node->se != nullptr) {
        export_node_json(node->se, out);
    } else {
        out += "null";
    }
    out += "}}";
}

void QuadTree::delete_subtree(QTNode* node) {
    if (node == nullptr) {
        return;
    }

    delete_subtree(node->nw);
    delete_subtree(node->ne);
    delete_subtree(node->sw);
    delete_subtree(node->se);
    delete node;
}
