#include <iostream>
#include <string>
#include <vector>

#include "D:/SY/sem2/DS/cpp/core/Scheduler.h"
#include "D:/SY/sem2/DS/cpp/ds/json.hpp"

int main() {
    std::vector<Zone> zones = {
        {1, "Nashik", 19.9975, 73.7898, 8, 150000, 500, 200, false, -1, nullptr},
        {2, "Pune", 18.5204, 73.8567, 6, 220000, 400, 150, false, -1, nullptr},
        {3, "Mumbai", 19.0760, 72.8777, 9, 500000, 900, 350, false, -1, nullptr}
    };

    std::vector<Depot> depots = {
        {1, "Mumbai Depot", 19.0760, 72.8777, 5000, 2000, 3, true}
    };

    std::vector<Road> roads = {
        {0, 1, 2, 100.0, 100.0, false},
        {1, 2, 3, 100.0, 100.0, false}
    };

    Scheduler scheduler(zones, depots, roads);
    scheduler.initialize();
    scheduler.run_cycle();

    auto first = nlohmann::json::parse(scheduler.get_full_state_json());
    std::cout << "after_cycle served=" << first["zones"][2]["is_served"].get<bool>()
              << " heap=" << first["heap"]["total_nodes"].get<int>() << "\n";

    scheduler.simulate_severity_update(3, 9);
    auto second = nlohmann::json::parse(scheduler.get_full_state_json());
    std::cout << "after_update served=" << second["zones"][2]["is_served"].get<bool>()
              << " assigned=" << second["zones"][2]["assigned_depot"].get<int>()
              << " heap=" << second["heap"]["total_nodes"].get<int>() << "\n";

    scheduler.run_cycle();
    auto third = nlohmann::json::parse(scheduler.get_full_state_json());
    std::cout << "after_second_cycle served=" << third["zones"][2]["is_served"].get<bool>()
              << " heap=" << third["heap"]["total_nodes"].get<int>() << "\n";
    return 0;
}
