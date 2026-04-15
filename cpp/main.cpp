#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/DataLoader.h"
#include "core/Graph.h"
#include "core/Router.h"
#include "ds/FibonacciHeap.h"
#include "ds/QuadTree.h"
#include "ds/SegmentTree.h"
#include "ds/SkipList.h"
#include "ds/Trie.h"
#include "ds/UnionFind.h"
#include "ds/json.hpp"
#include "output/StateExporter.h"

namespace {

constexpr double kEarthRadiusKm = 6371.0;

double to_radians(double degrees) {
    return degrees * 3.14159265358979323846 / 180.0;
}

double haversine_km(double lat1, double lng1, double lat2, double lng2) {
    const double d_lat = to_radians(lat2 - lat1);
    const double d_lng = to_radians(lng2 - lng1);
    const double a = std::pow(std::sin(d_lat / 2.0), 2.0) +
                     std::cos(to_radians(lat1)) * std::cos(to_radians(lat2)) *
                         std::pow(std::sin(d_lng / 2.0), 2.0);
    const double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return kEarthRadiusKm * c;
}

BoundingBox compute_world_boundary(const std::vector<Zone>& zones,
                                   const std::vector<Depot>& depots) {
    BoundingBox box;
    box.min_lat = std::numeric_limits<double>::infinity();
    box.max_lat = -std::numeric_limits<double>::infinity();
    box.min_lng = std::numeric_limits<double>::infinity();
    box.max_lng = -std::numeric_limits<double>::infinity();

    for (const Zone& zone : zones) {
        box.min_lat = std::min(box.min_lat, zone.lat);
        box.max_lat = std::max(box.max_lat, zone.lat);
        box.min_lng = std::min(box.min_lng, zone.lng);
        box.max_lng = std::max(box.max_lng, zone.lng);
    }

    for (const Depot& depot : depots) {
        box.min_lat = std::min(box.min_lat, depot.lat);
        box.max_lat = std::max(box.max_lat, depot.lat);
        box.min_lng = std::min(box.min_lng, depot.lng);
        box.max_lng = std::max(box.max_lng, depot.lng);
    }

    if (!std::isfinite(box.min_lat)) {
        box.min_lat = 0.0;
        box.max_lat = 1.0;
        box.min_lng = 0.0;
        box.max_lng = 1.0;
    }

    const double padding = 0.5;
    box.min_lat -= padding;
    box.max_lat += padding;
    box.min_lng -= padding;
    box.max_lng += padding;
    return box;
}

std::vector<std::string> to_trie_path(const std::string& word) {
    std::vector<std::string> path;
    path.reserve(word.size());
    for (char ch : word) {
        path.push_back(std::string(1, ch));
    }
    return path;
}

int max_zone_id(const std::vector<Zone>& zones) {
    int max_id = 0;
    for (const Zone& zone : zones) {
        max_id = std::max(max_id, zone.id);
    }
    return max_id;
}

int nearest_zone_to_depot(const Depot& depot, const std::vector<Zone>& zones) {
    int best_id = zones.empty() ? -1 : zones.front().id;
    double best_distance = std::numeric_limits<double>::infinity();

    for (const Zone& zone : zones) {
        const double distance = haversine_km(depot.lat, depot.lng, zone.lat, zone.lng);
        if (distance < best_distance) {
            best_distance = distance;
            best_id = zone.id;
        }
    }

    return best_id;
}

double compute_priority_key(const Zone& zone, const Depot& depot) {
    const int clamped_severity = std::max(1, std::min(10, zone.severity));
    return (10 - clamped_severity) * 1000.0 +
           haversine_km(zone.lat, zone.lng, depot.lat, depot.lng);
}

const Depot* select_nearest_usable_depot(const Zone& zone,
                                         const QuadTree& depot_tree,
                                         const std::vector<Depot>& depots,
                                         const std::unordered_map<int, std::size_t>& depot_index) {
    const Depot* best = nullptr;
    double best_distance = std::numeric_limits<double>::infinity();

    try {
        const Point nearest_point = depot_tree.nearest_neighbor({zone.lat, zone.lng, -1, zone.name});
        auto nearest_it = depot_index.find(nearest_point.entity_id);
        if (nearest_it != depot_index.end()) {
            const Depot& candidate = depots[nearest_it->second];
            if (candidate.is_active &&
                (candidate.food_stock > 0 || candidate.medicine_stock > 0)) {
                best = &candidate;
                best_distance = haversine_km(zone.lat, zone.lng, candidate.lat, candidate.lng);
            }
        }
    } catch (const std::exception&) {
    }

    for (const Depot& depot : depots) {
        if (!depot.is_active || (depot.food_stock <= 0 && depot.medicine_stock <= 0)) {
            continue;
        }
        const double distance = haversine_km(zone.lat, zone.lng, depot.lat, depot.lng);
        if (distance < best_distance) {
            best_distance = distance;
            best = &depot;
        }
    }

    return best;
}

std::unique_ptr<FibonacciHeap> rebuild_priority_queue(
    std::vector<Zone>& zones,
    const QuadTree& depot_tree,
    const std::vector<Depot>& depots,
    const std::unordered_map<int, std::size_t>& depot_index) {
    auto heap = std::make_unique<FibonacciHeap>();
    for (Zone& zone : zones) {
        zone.heap_node = nullptr;
        if (zone.is_served) {
            continue;
        }
        const Depot* depot = select_nearest_usable_depot(zone, depot_tree, depots, depot_index);
        if (depot == nullptr) {
            continue;
        }
        zone.heap_node = heap->insert(zone.id, compute_priority_key(zone, *depot));
    }
    return heap;
}

struct SimulationContext {
    std::vector<Zone> zones;
    std::vector<Depot> depots;
    std::vector<Road> roads;
    std::unordered_map<int, std::size_t> zone_index;
    std::unordered_map<int, std::size_t> depot_index;
    std::unordered_map<int, double> depot_food_key;
    std::unique_ptr<QuadTree> depot_tree;
    std::unique_ptr<FibonacciHeap> heap;
    std::unique_ptr<UnionFind> union_find;
    std::unique_ptr<SegmentTree> segment_tree;
    std::unique_ptr<Graph> graph;
    std::unique_ptr<Router> router;
    std::unique_ptr<SkipList<int>> food_index;
    std::unique_ptr<Trie> trie;
    std::unique_ptr<StateExporter> exporter;
    std::vector<DispatchEntry> dispatch_log;
    std::vector<std::string> trie_path;
    int cycle = 0;
};

nlohmann::json build_state_json(const SimulationContext& context) {
    nlohmann::json state;
    state["cycle"] = context.cycle;

    const nlohmann::json raw_heap = nlohmann::json::parse(context.heap->export_json());
    nlohmann::json heap_nodes = nlohmann::json::array();
    if (raw_heap.contains("snapshot") && raw_heap.at("snapshot").is_array()) {
        for (const auto& item : raw_heap.at("snapshot")) {
            nlohmann::json node;
            node["id"] = item.value("zone_id", -1);
            node["key"] = item.value("key", 0.0);
            node["degree"] = item.value("degree", 0);
            node["marked"] = item.value("marked", false);
            heap_nodes.push_back(node);
        }
    }
    state["heap"] = {
        {"nodes", heap_nodes},
        {"min_id", raw_heap.contains("min_zone_id") && !raw_heap.at("min_zone_id").is_null()
                       ? raw_heap.at("min_zone_id").get<int>()
                       : -1}
    };

    state["zones"] = nlohmann::json::array();
    for (const Zone& zone : context.zones) {
        state["zones"].push_back({
            {"id", zone.id},
            {"name", zone.name},
            {"lat", zone.lat},
            {"lng", zone.lng},
            {"severity", zone.severity},
            {"is_served", zone.is_served},
            {"assigned_depot", zone.assigned_depot}
        });
    }

    state["depots"] = nlohmann::json::array();
    for (const Depot& depot : context.depots) {
        state["depots"].push_back({
            {"id", depot.id},
            {"name", depot.name},
            {"lat", depot.lat},
            {"lng", depot.lng},
            {"food_stock", depot.food_stock},
            {"medicine_stock", depot.medicine_stock},
            {"is_active", depot.is_active}
        });
    }

    state["dispatch_log"] = nlohmann::json::array();
    for (const DispatchEntry& entry : context.dispatch_log) {
        state["dispatch_log"].push_back({
            {"cycle", entry.cycle},
            {"zone_id", entry.zone_id},
            {"depot_id", entry.depot_id},
            {"food", entry.food},
            {"medicine", entry.medicine},
            {"route", entry.route},
            {"distance_km", entry.distance_km}
        });
    }

    state["union_find"] = {{"components", context.union_find->get_all_components()}};

    const nlohmann::json raw_skip = nlohmann::json::parse(context.food_index->export_json());
    nlohmann::json levels = nlohmann::json::array();
    if (raw_skip.contains("levels") && raw_skip.at("levels").is_array()) {
        for (const auto& level_entry : raw_skip.at("levels")) {
            nlohmann::json level_nodes = nlohmann::json::array();
            if (level_entry.contains("nodes") && level_entry.at("nodes").is_array()) {
                for (const auto& node : level_entry.at("nodes")) {
                    level_nodes.push_back({
                        {"key", node.value("key", 0.0)},
                        {"depot_id", std::stoi(node.value("value", std::string("-1")))}
                    });
                }
            }
            levels.push_back(level_nodes);
        }
    }
    state["skip_list"] = {{"levels", levels}};
    state["trie_path"] = context.trie_path;

    return state;
}

std::string resolve_state_output_path(const char* argv0) {
    std::string executable_path = argv0 != nullptr ? std::string(argv0) : std::string();
    std::replace(executable_path.begin(), executable_path.end(), '/', '\\');

    const std::size_t last_separator = executable_path.find_last_of("\\");
    const std::string executable_dir =
        last_separator == std::string::npos ? std::string(".")
                                            : executable_path.substr(0, last_separator);

    return executable_dir + "\\..\\frontend\\data\\state.json";
}

SimulationContext create_context(const std::string& interactive_output_path) {
    DataLoader loader;
    SimulationContext context;
    context.zones = loader.load_zones("data/india_zones.json");
    context.depots = loader.load_depots("data/india_depots.json");
    context.roads = loader.load_roads("data/india_roads.json");

    for (std::size_t i = 0; i < context.zones.size(); ++i) {
        context.zone_index[context.zones[i].id] = i;
    }
    for (std::size_t i = 0; i < context.depots.size(); ++i) {
        context.depot_index[context.depots[i].id] = i;
    }

    std::vector<double> road_weights;
    road_weights.reserve(context.roads.size());
    for (const Road& road : context.roads) {
        road_weights.push_back(road.current_weight > 0.0 ? road.current_weight : road.distance_km);
    }

    const int graph_nodes = max_zone_id(context.zones) + 1;
    context.union_find = std::make_unique<UnionFind>(graph_nodes);
    context.segment_tree = std::make_unique<SegmentTree>(road_weights);
    context.graph = std::make_unique<Graph>(graph_nodes, context.union_find.get(),
                                            context.segment_tree.get());
    for (const Road& road : context.roads) {
        context.graph->add_road(road.from_id, road.to_id, road.distance_km);
    }

    context.router = std::make_unique<Router>(context.graph.get());
    context.food_index = std::make_unique<SkipList<int>>();
    context.trie = std::make_unique<Trie>();
    if (!interactive_output_path.empty()) {
        context.exporter = std::make_unique<StateExporter>(interactive_output_path);
    }

    const BoundingBox boundary = compute_world_boundary(context.zones, context.depots);
    context.depot_tree = std::make_unique<QuadTree>(boundary);
    for (const Depot& depot : context.depots) {
        context.depot_tree->insert({depot.lat, depot.lng, depot.id, depot.name});
        context.food_index->insert(static_cast<double>(depot.food_stock), depot.id);
        context.depot_food_key[depot.id] = static_cast<double>(depot.food_stock);
    }

    for (const Zone& zone : context.zones) {
        context.trie->insert(zone.name, zone.id);
    }
    if (!context.zones.empty()) {
        context.trie->increment_frequency(context.zones.front().name);
        context.trie->increment_frequency(context.zones.front().name);
        context.trie_path = to_trie_path(context.zones.front().name);
    }

    context.heap = rebuild_priority_queue(context.zones, *context.depot_tree, context.depots,
                                          context.depot_index);
    return context;
}

bool run_cycle(SimulationContext& context) {
    if (!context.heap || context.heap->is_empty()) {
        return false;
    }

    std::unique_ptr<FibNode> extracted(context.heap->extract_min());
    if (!extracted) {
        return false;
    }

    auto zone_it = context.zone_index.find(extracted->zone_id);
    if (zone_it == context.zone_index.end()) {
        return false;
    }

    Zone& zone = context.zones[zone_it->second];
    if (zone.is_served) {
        return false;
    }

    const Depot* chosen = select_nearest_usable_depot(zone, *context.depot_tree, context.depots,
                                                      context.depot_index);
    if (chosen == nullptr) {
        return false;
    }

    Depot& depot = context.depots[context.depot_index.at(chosen->id)];
    const int food = std::min(zone.food_needed, depot.food_stock);
    const int medicine = std::min(zone.medicine_needed, depot.medicine_stock);
    if (food == 0 && medicine == 0) {
        depot.is_active = false;
        context.heap = rebuild_priority_queue(context.zones, *context.depot_tree, context.depots,
                                              context.depot_index);
        return false;
    }

    const int source_zone_id = nearest_zone_to_depot(depot, context.zones);
    std::pair<double, std::vector<int>> route_info =
        context.router->shortest_path(source_zone_id, zone.id);
    if (route_info.second.empty()) {
        route_info.first = haversine_km(depot.lat, depot.lng, zone.lat, zone.lng);
        route_info.second = {source_zone_id, zone.id};
    }

    zone.food_needed = std::max(0, zone.food_needed - food);
    zone.medicine_needed = std::max(0, zone.medicine_needed - medicine);
    zone.assigned_depot = depot.id;
    zone.is_served = (zone.food_needed == 0 && zone.medicine_needed == 0);

    context.food_index->remove(context.depot_food_key[depot.id]);
    depot.food_stock = std::max(0, depot.food_stock - food);
    depot.medicine_stock = std::max(0, depot.medicine_stock - medicine);
    context.depot_food_key[depot.id] = static_cast<double>(depot.food_stock);
    context.food_index->insert(context.depot_food_key[depot.id], depot.id);
    if (depot.food_stock == 0 && depot.medicine_stock == 0) {
        depot.is_active = false;
    }

    ++context.cycle;
    DispatchEntry entry;
    entry.cycle = context.cycle;
    entry.zone_id = zone.id;
    entry.depot_id = depot.id;
    entry.food = food;
    entry.medicine = medicine;
    entry.route = route_info.second;
    entry.distance_km = route_info.first;
    context.dispatch_log.push_back(entry);

    context.heap = rebuild_priority_queue(context.zones, *context.depot_tree, context.depots,
                                          context.depot_index);
    if (context.exporter) {
        context.exporter->export_state(context.cycle, context.heap->export_json(), context.zones,
                                       context.depots, context.dispatch_log,
                                       context.union_find->get_all_components(),
                                       context.food_index->export_json(), context.trie_path);
    }
    return true;
}

void export_current_state(SimulationContext& context) {
    if (!context.exporter) {
        return;
    }
    if (!context.heap) {
        context.heap = rebuild_priority_queue(context.zones, *context.depot_tree, context.depots,
                                              context.depot_index);
    }
    context.exporter->export_state(context.cycle, context.heap->export_json(), context.zones,
                                   context.depots, context.dispatch_log,
                                   context.union_find->get_all_components(),
                                   context.food_index->export_json(), context.trie_path);
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        const bool batch_mode = argc > 1 && std::string(argv[1]) == "--batch";
        const std::string output_path = batch_mode ? std::string() : resolve_state_output_path(argv[0]);
        SimulationContext context = create_context(output_path);
        if (!batch_mode) {
            export_current_state(context);
        }

        if (batch_mode) {
            run_cycle(context);
            std::cout << build_state_json(context).dump(2) << std::endl;
            return 0;
        }

        std::string command;
        while (std::cin >> command) {
            if (command == "cycle") {
                run_cycle(context);
                export_current_state(context);
            } else if (command == "block") {
                int road_idx = -1;
                std::cin >> road_idx;
                context.graph->block_road(road_idx);
                export_current_state(context);
            } else if (command == "update") {
                int zone_id = -1;
                int new_severity = 0;
                std::cin >> zone_id >> new_severity;
                auto zone_it = context.zone_index.find(zone_id);
                if (zone_it != context.zone_index.end()) {
                    context.zones[zone_it->second].severity = new_severity;
                    context.heap = rebuild_priority_queue(context.zones, *context.depot_tree,
                                                          context.depots, context.depot_index);
                }
                export_current_state(context);
            } else if (command == "search") {
                std::string prefix;
                std::cin >> prefix;
                const std::vector<std::string> suggestions =
                    context.trie->autocomplete(prefix, 10);
                if (!suggestions.empty()) {
                    context.trie_path = to_trie_path(suggestions.front());
                    context.trie->increment_frequency(suggestions.front());
                } else {
                    context.trie_path = to_trie_path(prefix);
                }
                export_current_state(context);
                for (const std::string& suggestion : suggestions) {
                    std::cout << suggestion << '\n';
                }
            } else if (command == "state") {
                export_current_state(context);
                std::ifstream state_file("frontend/data/state.json");
                if (state_file.is_open()) {
                    std::cout << state_file.rdbuf();
                    std::cout << std::endl;
                }
            } else if (command == "quit") {
                break;
            }
        }
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
