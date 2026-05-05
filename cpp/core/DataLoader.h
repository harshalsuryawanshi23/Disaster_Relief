#pragma once

#include <string>
#include <vector>

#include "Scheduler.h"
#include "../ds/json.hpp"

class DataLoader {
public:
    std::vector<Zone> load_zones(const std::string& path) const;
    std::vector<Depot> load_depots(const std::string& path) const;
    std::vector<Road> load_roads(const std::string& path) const;

private:
    nlohmann::json read_json_file(const std::string& path) const;
};
