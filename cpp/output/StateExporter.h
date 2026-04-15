#pragma once

#include <string>
#include <vector>

#include "../core/Scheduler.h"
#include "../ds/json.hpp"

struct DispatchEntry {
    int cycle = 0;
    int zone_id = -1;
    int depot_id = -1;
    int food = 0;
    int medicine = 0;
    std::vector<int> route;
    double distance_km = 0.0;
};

class StateExporter {
public:
    explicit StateExporter(const std::string& output_path = "frontend/data/state.json");

    void export_state(int cycle,
                      const std::string& heap_json,
                      const std::vector<Zone>& zones,
                      const std::vector<Depot>& depots,
                      const std::vector<DispatchEntry>& dispatch_log,
                      const std::vector<std::vector<int>>& components,
                      const std::string& skip_list_json,
                      const std::vector<std::string>& trie_path) const;

private:
    std::string output_path_;

    void ensure_parent_dirs() const;
    nlohmann::json convert_heap(const std::string& heap_json) const;
    nlohmann::json convert_zones(const std::vector<Zone>& zones) const;
    nlohmann::json convert_depots(const std::vector<Depot>& depots) const;
    nlohmann::json convert_dispatch_log(const std::vector<DispatchEntry>& dispatch_log) const;
    nlohmann::json convert_union_find(const std::vector<std::vector<int>>& components) const;
    nlohmann::json convert_skip_list(const std::string& skip_list_json) const;
    nlohmann::json convert_trie_path(const std::vector<std::string>& trie_path) const;
};
