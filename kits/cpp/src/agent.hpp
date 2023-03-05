#pragma once

#include <cstdint>
#include <vector>

#include "lux/config.hpp"
#include "lux/exception.hpp"
#include "lux/json.hpp"
#include "lux/observation.hpp"



struct DijsktraNode {
    int dijsktraMetric=lux::INF;
    int distance = 0;
    int powerLight=0;
    int powerHeavy=0;
    int next = -1;
};

typedef std::pair<int, DijsktraNode> pii;
typedef std::vector<std::vector<pii>> Graph;

struct Agent {
    int64_t          step;
    std::string      player;
    int64_t          remainingOverageTime;
    lux::Observation obs;

    Agent()                             = default;
    Agent(const Agent &other)           = delete;
    Agent(Agent &&other)                = delete;
    Agent operator=(const Agent &other) = delete;
    Agent operator=(Agent &&other)      = delete;
    ~Agent()                            = default;

    /// Helper to check if the player can place a factory in the current turn.
    bool isTurnToPlaceFactory() const {
        LUX_ASSERT(obs.teams.find(player) != obs.teams.end(), "teams in observation must contain entry for the player");
        return step % 2 == (obs.teams.find(player)->second.place_first ? 1 : 0);
    }

    json setup();
    json act();

    double getPosValue(int x, int y, std::vector<lux::Position>& materialVect, int maxDistance);
    void dijkstra(const Graph& graph, int source, std::vector<DijsktraNode>& nodes);

    Graph createGraphWeights(int base, int rubbleDivider);
};
