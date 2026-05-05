#include "Router.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

#include "../ds/json.hpp"

namespace {

constexpr double kInfinity = std::numeric_limits<double>::infinity();

}  // namespace

Router::Router(Graph* g) : graph(g), num_nodes(0), pq(new MinHeap<std::pair<double, int>>()) {
    if (graph == nullptr) {
        throw std::invalid_argument("Router requires a valid Graph instance");
    }
    num_nodes = infer_num_nodes();
}

Router::~Router() {
    delete pq;
}

std::pair<double, std::vector<int>> Router::shortest_path(int source, int dest) {
    validate_node(source);
    validate_node(dest);

    std::vector<double> dist(static_cast<std::size_t>(num_nodes), kInfinity);
    std::vector<int> parent(static_cast<std::size_t>(num_nodes), -1);
    std::vector<bool> visited(static_cast<std::size_t>(num_nodes), false);

    pq->clear();
    dist[static_cast<std::size_t>(source)] = 0.0;
    pq->push({0.0, source});

    while (!pq->empty()) {
        const std::pair<double, int> current = pq->pop();
        const double current_dist = current.first;
        const int node = current.second;

        if (visited[static_cast<std::size_t>(node)]) {
            continue;
        }
        visited[static_cast<std::size_t>(node)] = true;

        if (node == dest) {
            break;
        }

        const std::vector<int> neighbors = graph->get_neighbors(node);
        for (int neighbor : neighbors) {
            const double weight = graph->road_weight(node, neighbor);
            if (!std::isfinite(weight)) {
                continue;
            }
            const double candidate = current_dist + weight;
            if (candidate < dist[static_cast<std::size_t>(neighbor)]) {
                dist[static_cast<std::size_t>(neighbor)] = candidate;
                parent[static_cast<std::size_t>(neighbor)] = node;
                pq->push({candidate, neighbor});
            }
        }
    }

    if (!std::isfinite(dist[static_cast<std::size_t>(dest)])) {
        return {kInfinity, {}};
    }

    std::vector<int> path;
    for (int at = dest; at != -1; at = parent[static_cast<std::size_t>(at)]) {
        path.push_back(at);
    }
    std::reverse(path.begin(), path.end());
    return {dist[static_cast<std::size_t>(dest)], path};
}

std::vector<double> Router::all_shortest(int source) {
    validate_node(source);

    std::vector<double> dist(static_cast<std::size_t>(num_nodes), kInfinity);
    std::vector<bool> visited(static_cast<std::size_t>(num_nodes), false);

    pq->clear();
    dist[static_cast<std::size_t>(source)] = 0.0;
    pq->push({0.0, source});

    while (!pq->empty()) {
        const std::pair<double, int> current = pq->pop();
        const double current_dist = current.first;
        const int node = current.second;

        if (visited[static_cast<std::size_t>(node)]) {
            continue;
        }
        visited[static_cast<std::size_t>(node)] = true;

        const std::vector<int> neighbors = graph->get_neighbors(node);
        for (int neighbor : neighbors) {
            const double weight = graph->road_weight(node, neighbor);
            if (!std::isfinite(weight)) {
                continue;
            }
            const double candidate = current_dist + weight;
            if (candidate < dist[static_cast<std::size_t>(neighbor)]) {
                dist[static_cast<std::size_t>(neighbor)] = candidate;
                pq->push({candidate, neighbor});
            }
        }
    }

    return dist;
}

int Router::infer_num_nodes() const {
    const nlohmann::json graph_json = nlohmann::json::parse(graph->export_json());
    if (!graph_json.contains("num_nodes")) {
        throw std::runtime_error("Graph export does not include num_nodes");
    }
    return graph_json.at("num_nodes").get<int>();
}

void Router::validate_node(int node_id) const {
    if (node_id < 0 || node_id >= num_nodes) {
        throw std::out_of_range("Router node id out of range");
    }
}
