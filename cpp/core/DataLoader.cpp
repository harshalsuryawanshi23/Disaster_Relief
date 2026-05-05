#include "DataLoader.h"

#include <fstream>
#include <stdexcept>

namespace {

template <typename T>
T required_value(const nlohmann::json& item, const char* key) {
    if (!item.contains(key)) {
        throw std::runtime_error(std::string("Missing required key: ") + key);
    }
    return item.at(key).get<T>();
}

template <typename T>
T optional_value(const nlohmann::json& item, const char* key, const T& fallback) {
    if (!item.contains(key) || item.at(key).is_null()) {
        return fallback;
    }
    return item.at(key).get<T>();
}

}  // namespace

std::vector<Zone> DataLoader::load_zones(const std::string& path) const {
    const nlohmann::json document = read_json_file(path);
    if (!document.is_array()) {
        throw std::runtime_error("Zones JSON must be an array");
    }

    std::vector<Zone> zones;
    zones.reserve(document.size());

    for (const auto& item : document) {
        Zone zone;
        zone.id = required_value<int>(item, "id");
        zone.name = required_value<std::string>(item, "name");
        zone.lat = required_value<double>(item, "lat");
        zone.lng = required_value<double>(item, "lng");
        zone.severity = required_value<int>(item, "severity");
        zone.population = required_value<int>(item, "population");
        zone.food_needed = required_value<int>(item, "food_needed");
        zone.medicine_needed = required_value<int>(item, "medicine_needed");
        zone.is_served = optional_value<bool>(item, "is_served", false);
        zone.assigned_depot = optional_value<int>(item, "assigned_depot", -1);
        zone.heap_node = nullptr;
        zones.push_back(zone);
    }

    return zones;
}

std::vector<Depot> DataLoader::load_depots(const std::string& path) const {
    const nlohmann::json document = read_json_file(path);
    if (!document.is_array()) {
        throw std::runtime_error("Depots JSON must be an array");
    }

    std::vector<Depot> depots;
    depots.reserve(document.size());

    for (const auto& item : document) {
        Depot depot;
        depot.id = required_value<int>(item, "id");
        depot.name = required_value<std::string>(item, "name");
        depot.lat = required_value<double>(item, "lat");
        depot.lng = required_value<double>(item, "lng");
        depot.food_stock = required_value<int>(item, "food_stock");
        depot.medicine_stock = required_value<int>(item, "medicine_stock");
        depot.vehicles = required_value<int>(item, "vehicles");
        depot.is_active = optional_value<bool>(item, "is_active", true);
        depots.push_back(depot);
    }

    return depots;
}

std::vector<Road> DataLoader::load_roads(const std::string& path) const {
    const nlohmann::json document = read_json_file(path);
    if (!document.is_array()) {
        throw std::runtime_error("Roads JSON must be an array");
    }

    std::vector<Road> roads;
    roads.reserve(document.size());

    for (const auto& item : document) {
        Road road;
        road.id = optional_value<int>(item, "id", static_cast<int>(roads.size()));
        road.from_id = required_value<int>(item, "from");
        road.to_id = required_value<int>(item, "to");
        road.distance_km = required_value<double>(item, "distance_km");
        road.current_weight = optional_value<double>(item, "current_weight", road.distance_km);
        road.is_blocked = optional_value<bool>(item, "is_blocked", false);
        roads.push_back(road);
    }

    return roads;
}

nlohmann::json DataLoader::read_json_file(const std::string& path) const {
    std::ifstream input(path);
    if (!input.is_open()) {
        throw std::runtime_error("Failed to open JSON file: " + path);
    }

    nlohmann::json document;
    input >> document;
    return document;
}
