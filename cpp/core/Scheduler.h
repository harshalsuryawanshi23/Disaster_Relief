#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../ds/FibonacciHeap.h"

#if __has_include("Zone.h")
#include "Zone.h"
#else
struct Zone {
    int id = -1;
    std::string name;
    double lat = 0.0;
    double lng = 0.0;
    int severity = 1;
    int population = 0;
    int food_needed = 0;
    int medicine_needed = 0;
    bool is_served = false;
    int assigned_depot = -1;
    FibNode* heap_node = nullptr;
};
#endif

#if __has_include("Depot.h")
#include "Depot.h"
#else
struct Depot {
    int id = -1;
    std::string name;
    double lat = 0.0;
    double lng = 0.0;
    int food_stock = 0;
    int medicine_stock = 0;
    int vehicles = 0;
    bool is_active = true;
};
#endif

#if __has_include("Graph.h")
#include "Graph.h"
#else
struct Road {
    int id = -1;
    int from_id = -1;
    int to_id = -1;
    double distance_km = 0.0;
    double current_weight = 0.0;
    bool is_blocked = false;
};
#endif

class QuadTree;
class UnionFind;
template <typename T>
class SkipList;
class Trie;
class Graph;
class Router;
class StateExporter;

class Scheduler {
public:
    struct SchedulerData;

    Scheduler(std::vector<Zone>& zones,
              std::vector<Depot>& depots,
              std::vector<Road>& roads);
    ~Scheduler();

    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    void initialize();
    void run_cycle();
    void run_all();
    void simulate_road_block(int road_idx);
    void simulate_severity_update(int zone_id, int new_severity);
    std::vector<std::string> get_autocomplete(const std::string& prefix);
    std::string get_full_state_json();

private:
    std::unique_ptr<SchedulerData> data_;

    double compute_priority_key(const Zone& z, const Depot& nearest);
    void dispatch_resources(Zone& z, Depot& d, int food, int med);
    void update_zone_severity(int zone_id, int new_severity);
    void rebalance_heap_after_update(int zone_id);
};
