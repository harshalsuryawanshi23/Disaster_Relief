#pragma once

#include <string>
#include <vector>

struct Point {
    double lat = 0.0;
    double lng = 0.0;
    int entity_id = -1;
    std::string label;
};

struct BoundingBox {
    double min_lat = 0.0;
    double max_lat = 0.0;
    double min_lng = 0.0;
    double max_lng = 0.0;

    bool contains(Point p) const;
    bool intersects(BoundingBox other) const;
    double min_distance_to(Point p) const;
};

struct QTNode {
    BoundingBox boundary;
    int capacity;
    std::vector<Point> points;
    QTNode* nw;
    QTNode* ne;
    QTNode* sw;
    QTNode* se;

    explicit QTNode(BoundingBox bb, int cap = 4);
    bool is_leaf() const;
};

class QuadTree {
public:
    explicit QuadTree(BoundingBox world_boundary);
    ~QuadTree();

    QuadTree(const QuadTree&) = delete;
    QuadTree& operator=(const QuadTree&) = delete;

    void insert(Point p);
    Point nearest_neighbor(Point query) const;
    std::vector<Point> range_search(BoundingBox area) const;
    void remove_point(int entity_id);
    std::string export_json() const;

private:
    QTNode* root;

    void subdivide(QTNode* node);
    void insert_helper(QTNode* node, Point p);
    void range_query_helper(QTNode* node, BoundingBox range, std::vector<Point>& result) const;
    void nearest_helper(QTNode* node, Point target, Point& best, double& best_dist) const;
    bool remove_helper(QTNode* node, int entity_id);
    void export_node_json(QTNode* node, std::string& out) const;
    void delete_subtree(QTNode* node);
};
