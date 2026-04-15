#include "Graph.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
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

Graph::Graph(int n, UnionFind* uf, SegmentTree* st)
    : num_nodes(n),
      adj(n > 0 ? static_cast<std::size_t>(n) : 0U),
      roads(),
      connectivity(uf),
      road_weights(st) {
    if (n < 0) {
        throw std::invalid_argument("Graph node count cannot be negative");
    }
}

void Graph::add_road(int u, int v, double dist) {
    validate_node(u);
    validate_node(v);
    if (dist < 0.0) {
        throw std::invalid_argument("Road distance cannot be negative");
    }

    Road road;
    road.id = static_cast<int>(roads.size());
    road.from_id = u;
    road.to_id = v;
    road.distance_km = dist;
    road.current_weight = dist;
    road.is_blocked = false;
    roads.push_back(road);

    adj[static_cast<std::size_t>(u)].push_back({v, dist});
    adj[static_cast<std::size_t>(v)].push_back({u, dist});

    if (connectivity != nullptr) {
        connectivity->union_zones(u, v);
    }
}

void Graph::block_road(int road_idx) {
    if (road_idx < 0 || static_cast<std::size_t>(road_idx) >= roads.size()) {
        throw std::out_of_range("Road index out of range");
    }

    Road& road = roads[static_cast<std::size_t>(road_idx)];
    road.is_blocked = true;
    road.current_weight = std::numeric_limits<double>::infinity();

    for (auto& edge : adj[static_cast<std::size_t>(road.from_id)]) {
        if (edge.first == road.to_id) {
            edge.second = road.current_weight;
        }
    }
    for (auto& edge : adj[static_cast<std::size_t>(road.to_id)]) {
        if (edge.first == road.from_id) {
            edge.second = road.current_weight;
        }
    }

    if (connectivity != nullptr) {
        connectivity->block_road(road.from_id, road.to_id);
    }

    if (road_weights != nullptr) {
        try {
            road_weights->update(road_idx, road.current_weight);
        } catch (const std::exception&) {
        }
    }
}

bool Graph::zones_connected(int a, int b) {
    validate_node(a);
    validate_node(b);

    bool has_blocked_roads =
        std::any_of(roads.begin(), roads.end(), [](const Road& road) { return road.is_blocked; });
    if (!has_blocked_roads && connectivity != nullptr) {
        return connectivity->connected(a, b);
    }

    std::vector<bool> visited(static_cast<std::size_t>(num_nodes), false);
    std::queue<int> q;
    visited[static_cast<std::size_t>(a)] = true;
    q.push(a);

    while (!q.empty()) {
        const int current = q.front();
        q.pop();
        if (current == b) {
            return true;
        }

        for (const auto& edge : adj[static_cast<std::size_t>(current)]) {
            if (!std::isfinite(edge.second)) {
                continue;
            }
            if (!visited[static_cast<std::size_t>(edge.first)]) {
                visited[static_cast<std::size_t>(edge.first)] = true;
                q.push(edge.first);
            }
        }
    }

    return false;
}

std::vector<int> Graph::get_neighbors(int zone_id) const {
    validate_node(zone_id);

    std::vector<int> neighbors;
    for (const auto& edge : adj[static_cast<std::size_t>(zone_id)]) {
        if (std::isfinite(edge.second)) {
            neighbors.push_back(edge.first);
        }
    }
    return neighbors;
}

double Graph::road_weight(int u, int v) const {
    validate_node(u);
    validate_node(v);

    const int road_idx = find_road_index(u, v);
    if (road_idx < 0) {
        return std::numeric_limits<double>::infinity();
    }
    return roads[static_cast<std::size_t>(road_idx)].current_weight;
}

std::string Graph::export_json() const {
    std::ostringstream out;
    out << "{"
        << "\"num_nodes\":" << num_nodes << ','
        << "\"roads\":[";

    for (std::size_t i = 0; i < roads.size(); ++i) {
        if (i > 0) {
            out << ',';
        }
        const Road& road = roads[i];
        out << "{"
            << "\"id\":" << road.id << ','
            << "\"from\":" << road.from_id << ','
            << "\"to\":" << road.to_id << ','
            << "\"distance_km\":" << road.distance_km << ','
            << "\"current_weight\":" << road.current_weight << ','
            << "\"is_blocked\":" << (road.is_blocked ? "true" : "false")
            << "}";
    }

    out << "],\"adjacency\":[";
    for (std::size_t i = 0; i < adj.size(); ++i) {
        if (i > 0) {
            out << ',';
        }
        out << "{"
            << "\"node\":" << i << ','
            << "\"neighbors\":[";

        for (std::size_t j = 0; j < adj[i].size(); ++j) {
            if (j > 0) {
                out << ',';
            }
            out << "{"
                << "\"to\":" << adj[i][j].first << ','
                << "\"weight\":" << adj[i][j].second
                << "}";
        }

        out << "]}";
    }

    out << "],\"meta\":\""
        << json_escape("Graph export with roads and adjacency list")
        << "\"}";
    return out.str();
}

void Graph::validate_node(int node_id) const {
    if (node_id < 0 || node_id >= num_nodes) {
        throw std::out_of_range("Graph node id out of range");
    }
}

int Graph::find_road_index(int u, int v) const {
    for (std::size_t i = 0; i < roads.size(); ++i) {
        const Road& road = roads[i];
        const bool same_direction = road.from_id == u && road.to_id == v;
        const bool opposite_direction = road.from_id == v && road.to_id == u;
        if (same_direction || opposite_direction) {
            return static_cast<int>(i);
        }
    }
    return -1;
}
