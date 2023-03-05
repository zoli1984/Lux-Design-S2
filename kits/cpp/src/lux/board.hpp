#pragma once

#include <map>
#include <string>
#include <vector>
#include <utility>

#include "lux/common.hpp"
#include "lux/json.hpp"

namespace lux {

    const int BOARD_SIZE = 48;
    const int BOARD_SIZE2 = BOARD_SIZE * BOARD_SIZE;
    const int INF = 9999999;

    struct Board {
        std::vector<std::vector<int64_t>> ice;
        std::vector<std::vector<int64_t>> lichen;
        std::vector<std::vector<int64_t>> lichen_strains;
        std::vector<std::vector<int64_t>> ore;
        std::vector<std::vector<int64_t>> rubble;
        std::vector<std::vector<bool>>    valid_spawns_mask;
        std::vector<std::vector<int64_t>> factory_occupancy;  // populated in Observation deserialization
        int64_t                           factories_per_team;

        std::vector<lux::Position> ice_vect;
        std::vector<lux::Position> ore_vect;

       private:
        bool                           initialized = false;
        std::map<std::string, int64_t> lichen_delta;
        std::map<std::string, int64_t> lichen_strains_delta;
        std::map<std::string, int64_t> rubble_delta;
        friend void                    from_json(const json &j, Board &b);
    };

    void to_json(json &j, const Board b);
    void from_json(const json &j, Board &b);
}  // namespace lux
