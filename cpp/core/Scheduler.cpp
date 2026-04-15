#include "Scheduler.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

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

std::string to_lower_copy(const std::string& input) {
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return result;
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

double compute_priority_key_for_zone(const Zone& zone, const Depot& nearest) {
    const int clamped_severity = std::max(1, std::min(10, zone.severity));
    const double distance = haversine_km(zone.lat, zone.lng, nearest.lat, nearest.lng);
    return (10 - clamped_severity) * 1000.0 + distance;
}

}  // namespace

struct Scheduler::SchedulerData {
    std::unique_ptr<FibonacciHeap> priority_queue;
    std::vector<Zone> zones;
    std::vector<Depot> depots;
    std::vector<Road> roads;
    std::vector<std::string> dispatch_log;
    std::unordered_map<int, std::size_t> zone_index;
    std::unordered_map<int, std::size_t> depot_index;
    bool initialized = false;
};

namespace {

const Depot* find_nearest_active_depot(const Scheduler::SchedulerData& data,
                                       const Zone& zone) {
    const Depot* best = nullptr;
    double best_distance = std::numeric_limits<double>::infinity();

    for (const Depot& depot : data.depots) {
        if (!depot.is_active || depot.vehicles <= 0) {
            continue;
        }

        const bool has_supply = depot.food_stock > 0 || depot.medicine_stock > 0;
        if (!has_supply) {
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

void rebuild_heap(Scheduler::SchedulerData& data, Scheduler& scheduler) {
    data.priority_queue = std::make_unique<FibonacciHeap>();

    for (Zone& zone : data.zones) {
        zone.heap_node = nullptr;
        if (zone.is_served) {
            continue;
        }

        const Depot* nearest = find_nearest_active_depot(data, zone);
        if (nearest != nullptr) {
            zone.heap_node = data.priority_queue->insert(
                zone.id, compute_priority_key_for_zone(zone, *nearest));
        } else {
            const Depot fallback = {-1, "unassigned", zone.lat, zone.lng, 0, 0, 0, false};
            zone.heap_node = data.priority_queue->insert(
                zone.id, compute_priority_key_for_zone(zone, fallback));
        }
    }
}

}  // namespace

Scheduler::Scheduler(std::vector<Zone>& zones,
                     std::vector<Depot>& depots,
                     std::vector<Road>& roads)
    : data_(std::make_unique<SchedulerData>()) {
    data_->zones = zones;
    data_->depots = depots;
    data_->roads = roads;
}

Scheduler::~Scheduler() = default;

void Scheduler::initialize() {
    data_->zone_index.clear();
    data_->depot_index.clear();

    for (std::size_t i = 0; i < data_->zones.size(); ++i) {
        data_->zone_index[data_->zones[i].id] = i;
    }
    for (std::size_t i = 0; i < data_->depots.size(); ++i) {
        data_->depot_index[data_->depots[i].id] = i;
    }

    rebuild_heap(*data_, *this);
    data_->initialized = true;
}

void Scheduler::run_cycle() {
    if (!data_->initialized) {
        initialize();
    }

    if (!data_->priority_queue || data_->priority_queue->is_empty()) {
        data_->dispatch_log.push_back("No pending zones remain in the scheduling queue.");
        return;
    }

    std::unique_ptr<FibNode> extracted(data_->priority_queue->extract_min());
    if (!extracted) {
        data_->dispatch_log.push_back("Priority queue returned no zone.");
        return;
    }

    auto zone_it = data_->zone_index.find(extracted->zone_id);
    if (zone_it == data_->zone_index.end()) {
        data_->dispatch_log.push_back("Extracted zone was not found in scheduler state.");
        return;
    }

    Zone& zone = data_->zones[zone_it->second];
    zone.heap_node = nullptr;
    if (zone.is_served) {
        data_->dispatch_log.push_back("Skipped a zone that was already served.");
        return;
    }

    const Depot* nearest_ptr = find_nearest_active_depot(*data_, zone);
    if (nearest_ptr == nullptr) {
        data_->dispatch_log.push_back("No active depot can currently serve zone " +
                                      std::to_string(zone.id) + ".");
        const Depot fallback = {-1, "unassigned", zone.lat, zone.lng, 0, 0, 0, false};
        zone.heap_node =
            data_->priority_queue->insert(zone.id, compute_priority_key(zone, fallback));
        return;
    }

    Depot& depot = data_->depots[data_->depot_index.at(nearest_ptr->id)];
    const int dispatched_food = std::min(zone.food_needed, depot.food_stock);
    const int dispatched_medicine = std::min(zone.medicine_needed, depot.medicine_stock);

    if (dispatched_food == 0 && dispatched_medicine == 0) {
        data_->dispatch_log.push_back("Nearest depot lacks usable stock for zone " +
                                      std::to_string(zone.id) + ".");
        zone.heap_node =
            data_->priority_queue->insert(zone.id, compute_priority_key(zone, depot));
        return;
    }

    dispatch_resources(zone, depot, dispatched_food, dispatched_medicine);

    if (!zone.is_served) {
        zone.heap_node =
            data_->priority_queue->insert(zone.id, compute_priority_key(zone, depot));
    }
}

void Scheduler::run_all() {
    if (!data_->initialized) {
        initialize();
    }

    int stagnant_rounds = 0;
    while (data_->priority_queue && !data_->priority_queue->is_empty()) {
        const std::size_t served_before = static_cast<std::size_t>(std::count_if(
            data_->zones.begin(), data_->zones.end(),
            [](const Zone& zone) { return zone.is_served; }));

        run_cycle();

        const std::size_t served_after = static_cast<std::size_t>(std::count_if(
            data_->zones.begin(), data_->zones.end(),
            [](const Zone& zone) { return zone.is_served; }));

        if (served_after == served_before) {
            ++stagnant_rounds;
        } else {
            stagnant_rounds = 0;
        }

        if (stagnant_rounds >= 2) {
            data_->dispatch_log.push_back(
                "Scheduler stopped because no further progress was possible.");
            break;
        }
    }
}

void Scheduler::simulate_road_block(int road_idx) {
    if (road_idx < 0 || static_cast<std::size_t>(road_idx) >= data_->roads.size()) {
        throw std::out_of_range("Road index out of range");
    }

    Road& road = data_->roads[static_cast<std::size_t>(road_idx)];
    road.is_blocked = true;
    road.current_weight = std::numeric_limits<double>::infinity();

    const int log_id = road.id >= 0 ? road.id : road_idx;
    data_->dispatch_log.push_back("Road " + std::to_string(log_id) + " marked as blocked.");
}

void Scheduler::simulate_severity_update(int zone_id, int new_severity) {
    update_zone_severity(zone_id, new_severity);
    rebalance_heap_after_update(zone_id);
}

std::vector<std::string> Scheduler::get_autocomplete(const std::string& prefix) {
    std::vector<std::string> matches;
    const std::string lowered_prefix = to_lower_copy(prefix);

    for (const Zone& zone : data_->zones) {
        const std::string lowered_name = to_lower_copy(zone.name);
        if (lowered_name.rfind(lowered_prefix, 0) == 0) {
            matches.push_back(zone.name);
        }
    }

    std::sort(matches.begin(), matches.end());
    if (matches.size() > 10) {
        matches.resize(10);
    }
    return matches;
}

std::string Scheduler::get_full_state_json() {
    if (!data_->initialized) {
        initialize();
    }

    std::ostringstream out;
    out << "{"
        << "\"zones\":[";

    for (std::size_t i = 0; i < data_->zones.size(); ++i) {
        if (i > 0) {
            out << ',';
        }
        const Zone& zone = data_->zones[i];
        out << "{"
            << "\"id\":" << zone.id << ','
            << "\"name\":\"" << json_escape(zone.name) << "\","
            << "\"lat\":" << zone.lat << ','
            << "\"lng\":" << zone.lng << ','
            << "\"severity\":" << zone.severity << ','
            << "\"food_needed\":" << zone.food_needed << ','
            << "\"medicine_needed\":" << zone.medicine_needed << ','
            << "\"is_served\":" << (zone.is_served ? "true" : "false") << ','
            << "\"assigned_depot\":" << zone.assigned_depot
            << "}";
    }

    out << "],\"depots\":[";
    for (std::size_t i = 0; i < data_->depots.size(); ++i) {
        if (i > 0) {
            out << ',';
        }
        const Depot& depot = data_->depots[i];
        out << "{"
            << "\"id\":" << depot.id << ','
            << "\"name\":\"" << json_escape(depot.name) << "\","
            << "\"lat\":" << depot.lat << ','
            << "\"lng\":" << depot.lng << ','
            << "\"food_stock\":" << depot.food_stock << ','
            << "\"medicine_stock\":" << depot.medicine_stock << ','
            << "\"vehicles\":" << depot.vehicles << ','
            << "\"is_active\":" << (depot.is_active ? "true" : "false")
            << "}";
    }

    out << "],\"roads\":[";
    for (std::size_t i = 0; i < data_->roads.size(); ++i) {
        if (i > 0) {
            out << ',';
        }
        const Road& road = data_->roads[i];
        out << "{"
            << "\"id\":" << road.id << ','
            << "\"from\":" << road.from_id << ','
            << "\"to\":" << road.to_id << ','
            << "\"distance_km\":" << road.distance_km << ','
            << "\"current_weight\":" << road.current_weight << ','
            << "\"is_blocked\":" << (road.is_blocked ? "true" : "false")
            << "}";
    }

    out << "],\"dispatch_log\":[";
    for (std::size_t i = 0; i < data_->dispatch_log.size(); ++i) {
        if (i > 0) {
            out << ',';
        }
        out << "\"" << json_escape(data_->dispatch_log[i]) << "\"";
    }

    out << "],\"heap\":";
    if (data_->priority_queue) {
        out << data_->priority_queue->export_json();
    } else {
        out << "null";
    }

    out << ",\"summary\":{"
        << "\"total_zones\":" << data_->zones.size() << ','
        << "\"served_zones\":"
        << std::count_if(data_->zones.begin(), data_->zones.end(),
                         [](const Zone& zone) { return zone.is_served; })
        << ",\"pending_zones\":"
        << std::count_if(data_->zones.begin(), data_->zones.end(),
                         [](const Zone& zone) { return !zone.is_served; })
        << "}}";
    return out.str();
}

double Scheduler::compute_priority_key(const Zone& z, const Depot& nearest) {
    return compute_priority_key_for_zone(z, nearest);
}

void Scheduler::dispatch_resources(Zone& z, Depot& d, int food, int med) {
    d.food_stock = std::max(0, d.food_stock - food);
    d.medicine_stock = std::max(0, d.medicine_stock - med);

    z.food_needed = std::max(0, z.food_needed - food);
    z.medicine_needed = std::max(0, z.medicine_needed - med);
    z.assigned_depot = d.id;
    z.is_served = (z.food_needed == 0 && z.medicine_needed == 0);

    std::ostringstream log;
    log << "Zone " << z.id << " served from depot " << d.id
        << " | food=" << food << " medicine=" << med;
    if (!z.is_served) {
        log << " | remaining(food=" << z.food_needed
            << ", medicine=" << z.medicine_needed << ")";
    }
    data_->dispatch_log.push_back(log.str());
}

void Scheduler::update_zone_severity(int zone_id, int new_severity) {
    auto it = data_->zone_index.find(zone_id);
    if (it == data_->zone_index.end()) {
        throw std::out_of_range("Zone id not found");
    }

    Zone& zone = data_->zones[it->second];
    zone.severity = std::max(1, std::min(10, new_severity));
    data_->dispatch_log.push_back("Severity updated for zone " + std::to_string(zone_id) +
                                  " to " + std::to_string(zone.severity) + ".");
}

void Scheduler::rebalance_heap_after_update(int zone_id) {
    auto it = data_->zone_index.find(zone_id);
    if (it == data_->zone_index.end()) {
        throw std::out_of_range("Zone id not found");
    }

    Zone& zone = data_->zones[it->second];
    if (zone.is_served) {
        return;
    }

    const Depot* nearest = find_nearest_active_depot(*data_, zone);
    if (nearest == nullptr) {
        rebuild_heap(*data_, *this);
        return;
    }

    const double new_key = compute_priority_key(zone, *nearest);
    if (zone.heap_node != nullptr && new_key <= zone.heap_node->key) {
        data_->priority_queue->decrease_key(zone.id, new_key);
        return;
    }

    rebuild_heap(*data_, *this);
}
