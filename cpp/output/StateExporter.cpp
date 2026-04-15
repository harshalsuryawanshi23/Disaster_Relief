#include "StateExporter.h"

#include <fstream>
#include <stdexcept>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

namespace {

std::string normalize_output_path(std::string path) {
#ifdef _WIN32
    std::replace(path.begin(), path.end(), '/', '\\');
#endif
    return path;
}

void create_dir_if_needed(const std::string& path) {
    if (path.empty()) {
        return;
    }
#ifdef _WIN32
    _mkdir(path.c_str());
#else
    mkdir(path.c_str(), 0755);
#endif
}

}  // namespace

StateExporter::StateExporter(const std::string& output_path) : output_path_(output_path) {}

void StateExporter::export_state(int cycle,
                                 const std::string& heap_json,
                                 const std::vector<Zone>& zones,
                                 const std::vector<Depot>& depots,
                                 const std::vector<DispatchEntry>& dispatch_log,
                                 const std::vector<std::vector<int>>& components,
                                 const std::string& skip_list_json,
                                 const std::vector<std::string>& trie_path) const {
    ensure_parent_dirs();

    nlohmann::json state;
    state["cycle"] = cycle;
    state["heap"] = convert_heap(heap_json);
    state["zones"] = convert_zones(zones);
    state["depots"] = convert_depots(depots);
    state["dispatch_log"] = convert_dispatch_log(dispatch_log);
    state["union_find"] = convert_union_find(components);
    state["skip_list"] = convert_skip_list(skip_list_json);
    state["trie_path"] = convert_trie_path(trie_path);

    const std::string normalized_path = normalize_output_path(output_path_);
    std::ofstream output(normalized_path);
    if (!output.is_open()) {
        throw std::runtime_error("Failed to open state output file: " + normalized_path);
    }
    output << state.dump(2);
}

void StateExporter::ensure_parent_dirs() const {
    if (output_path_.empty()) {
        return;
    }

    const std::string normalized = normalize_output_path(output_path_);
    std::size_t pos = normalized.find_first_of("\\/");
    while (pos != std::string::npos) {
        const std::string part = normalized.substr(0, pos);
        if (!part.empty() && part.find(':') == std::string::npos) {
            create_dir_if_needed(part);
        } else if (part.size() > 2 && part[1] == ':') {
            create_dir_if_needed(part);
        }
        pos = normalized.find_first_of("\\/", pos + 1);
    }
}

nlohmann::json StateExporter::convert_heap(const std::string& heap_json) const {
    const nlohmann::json raw = nlohmann::json::parse(heap_json);
    nlohmann::json nodes = nlohmann::json::array();

    if (raw.contains("snapshot") && raw.at("snapshot").is_array()) {
        for (const auto& item : raw.at("snapshot")) {
            nlohmann::json node;
            node["id"] = item.value("zone_id", -1);
            node["key"] = item.value("key", 0.0);
            node["degree"] = item.value("degree", 0);
            node["marked"] = item.value("marked", false);
            nodes.push_back(node);
        }
    }

    nlohmann::json heap;
    heap["nodes"] = nodes;
    if (raw.contains("min_zone_id") && !raw.at("min_zone_id").is_null()) {
        heap["min_id"] = raw.at("min_zone_id").get<int>();
    } else {
        heap["min_id"] = -1;
    }
    return heap;
}

nlohmann::json StateExporter::convert_zones(const std::vector<Zone>& zones) const {
    nlohmann::json result = nlohmann::json::array();
    for (const Zone& zone : zones) {
        nlohmann::json item;
        item["id"] = zone.id;
        item["name"] = zone.name;
        item["lat"] = zone.lat;
        item["lng"] = zone.lng;
        item["severity"] = zone.severity;
        item["is_served"] = zone.is_served;
        item["assigned_depot"] = zone.assigned_depot;
        result.push_back(item);
    }
    return result;
}

nlohmann::json StateExporter::convert_depots(const std::vector<Depot>& depots) const {
    nlohmann::json result = nlohmann::json::array();
    for (const Depot& depot : depots) {
        nlohmann::json item;
        item["id"] = depot.id;
        item["name"] = depot.name;
        item["lat"] = depot.lat;
        item["lng"] = depot.lng;
        item["food_stock"] = depot.food_stock;
        item["medicine_stock"] = depot.medicine_stock;
        item["is_active"] = depot.is_active;
        result.push_back(item);
    }
    return result;
}

nlohmann::json StateExporter::convert_dispatch_log(
    const std::vector<DispatchEntry>& dispatch_log) const {
    nlohmann::json result = nlohmann::json::array();
    for (const DispatchEntry& entry : dispatch_log) {
        nlohmann::json item;
        item["cycle"] = entry.cycle;
        item["zone_id"] = entry.zone_id;
        item["depot_id"] = entry.depot_id;
        item["food"] = entry.food;
        item["medicine"] = entry.medicine;
        item["route"] = entry.route;
        item["distance_km"] = entry.distance_km;
        result.push_back(item);
    }
    return result;
}

nlohmann::json StateExporter::convert_union_find(
    const std::vector<std::vector<int>>& components) const {
    nlohmann::json result;
    result["components"] = components;
    return result;
}

nlohmann::json StateExporter::convert_skip_list(const std::string& skip_list_json) const {
    const nlohmann::json raw = nlohmann::json::parse(skip_list_json);
    nlohmann::json levels = nlohmann::json::array();

    if (raw.contains("levels") && raw.at("levels").is_array()) {
        for (const auto& level_entry : raw.at("levels")) {
            nlohmann::json level_nodes = nlohmann::json::array();
            if (level_entry.contains("nodes") && level_entry.at("nodes").is_array()) {
                for (const auto& node : level_entry.at("nodes")) {
                    nlohmann::json converted;
                    converted["key"] = node.value("key", 0.0);
                    converted["depot_id"] = std::stoi(node.value("value", std::string("-1")));
                    level_nodes.push_back(converted);
                }
            }
            levels.push_back(level_nodes);
        }
    }

    nlohmann::json result;
    result["levels"] = levels;
    return result;
}

nlohmann::json StateExporter::convert_trie_path(const std::vector<std::string>& trie_path) const {
    return trie_path;
}
