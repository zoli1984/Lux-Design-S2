#include "agent.hpp"

#include "lux/action.hpp"
#include "lux/common.hpp"
#include "lux/log.hpp"

#include <vector>
#include <queue>

using namespace std;


Graph Agent::createGraphWeights(int base, int rubbleDivider) {
    Graph graph;
    graph.resize(lux::BOARD_SIZE2);
    for(int x = 0; x < lux::BOARD_SIZE; x++) {
        for (int y = 0; y < lux::BOARD_SIZE; y++) {
            int fromIndex = y * lux::BOARD_SIZE + x;
            graph[fromIndex].reserve(4);
            for (int vx = -1; vx <= 1; vx++)
            {
                for (int vy = -1; vy <= 1; vy++)
                {
                    if (abs(vx) + abs(vy) > 1) continue;
                    int newX = x + vx;
                    int newY = y + vy;
                    if (newX < 0 || newY < 0 || newX >= lux::BOARD_SIZE || newY >= lux::BOARD_SIZE) continue;

                    int toIndex = newY * lux::BOARD_SIZE + newX;

                    int cost = base + obs.board.rubble[newX][newY] / rubbleDivider;
                    DijsktraNode node;
                    node.distance = 1;
                    node.dijsktraMetric = cost;
                    node.powerLight= 1 + obs.board.rubble[newX][newY] / 20;
                    node.powerHeavy = 20 + obs.board.rubble[newX][newY];
                    graph[fromIndex].push_back({toIndex, node });
                }
            }
        }
    }
    return graph; 
}


void Agent::dijkstra(const Graph& graph, int source, vector<DijsktraNode> &nodes) {
    priority_queue<int, vector<int>, greater<int>> pq;
    nodes[source].dijsktraMetric = 0;
    nodes[source].powerLight = 0;
    nodes[source].powerHeavy = 0;
    pq.push(source);

    while (!pq.empty()) {
        auto current = pq.top();
        pq.pop();

        for (auto [v, w] : graph[current]) {
            if (nodes[v].dijsktraMetric > nodes[current].dijsktraMetric + w.dijsktraMetric) {
                if (nodes[current].next != -1) {
                    nodes[v].next = nodes[current].next; 
                }
                else {
                    nodes[v].next = v;
                }
                nodes[v].distance = nodes[current].distance + w.distance;
                nodes[v].powerHeavy= nodes[current].powerHeavy + w.powerHeavy;
                nodes[v].powerLight = nodes[current].powerLight + w.powerLight;
                nodes[v].dijsktraMetric = nodes[current].dijsktraMetric + w.dijsktraMetric;
                pq.push(v);
            }
        }
    }  
}
//vector<vector<int>> Agent::dijkstraAll(const Graph& graph) {
//    int INF = 1000000;
//    vector<vector<int>> dist;
//    dist.resize(lux::BOARD_SIZE2, vector<int>(lux::BOARD_SIZE2, INF));
//    vector<vector<int>> prev;
//    prev.resize(lux::BOARD_SIZE2, vector<int>(lux::BOARD_SIZE2, INF));
//    for (int i = 0; i < lux::BOARD_SIZE * lux::BOARD_SIZE; i++) {
//        dijkstra(graph, i, dist[i], prev[i]);
//    }
//    return dist;
//}

double Agent::getPosValue(int x, int y, std::vector<lux::Position>& materialVect, int maxDistance){
    double materialBaseFactor = 0.5;
    double posValue = 0;
    std::vector<double> values;
    values.reserve(50);
    for (auto& pos : materialVect) {
        auto dist = std::max(0,(int)std::abs(pos.x - x)-1) + std::max(0,(int)std::abs(pos.y - y)-1);
        if (dist > maxDistance) continue;
        bool hasCloserFactory = false;
        for (const auto& [unitId, factory] : obs.factories[player]) {
            auto factoryDist = std::max(0, (int)std::abs(factory.pos.x - pos.x) - 1) + std::max(0, (int)std::abs(factory.pos.y - pos.y) - 1);
            if (factoryDist <= dist || factoryDist <= 3) {
                hasCloserFactory = true;
                break;
            }
        }
        if (hasCloserFactory) continue;
        values.push_back(pow(materialBaseFactor, dist));
        //posValue += pow(materialBaseFactor, dist);
    }
    std::sort(values.begin(), values.end(), std::greater<>());
    for (size_t i = 0; i < values.size(); i++) {
        posValue += values[i] * std::pow(0.5, i);
    }
    return posValue;
}


json Agent::setup() {
    if (step == 0) {
        return lux::BidAction(player == "player_1" ? "AlphaStrike" : "MotherMars", 0);
    }
    if (obs.teams[player].factories_to_place && isTurnToPlaceFactory()) {
        // transform spawn_mask to positions
        const auto &spawns_mask = obs.board.valid_spawns_mask;
        lux::Position bestPos(-1,-1);
        double bestValue = -1;
        for (size_t x = 0; x < spawns_mask.size(); ++x) {
            for (size_t y = 0; y < spawns_mask[x].size(); ++y) {
                if (spawns_mask[x][y]) {
                    double icePosValue = getPosValue(x, y, obs.board.ice_vect,5);
                    double orePosValue = getPosValue(x, y, obs.board.ore_vect,100);

                    double value = icePosValue * orePosValue*orePosValue;
                    if (value > bestValue || bestPos.x==-1) {
                        bestValue = value;
                        bestPos = lux::Position(x, y);
                    }
                }
            }
        }
        return lux::SpawnAction(bestPos,
                                obs.teams[player].metal / 2,
                                obs.teams[player].water / 2);
    }
    return json::object();
}

json Agent::act() {
    auto lightRobotPowerSaveGraph = createGraphWeights(1, 20);
    auto lightRobotFastestGraph = createGraphWeights(1, 100);
    vector<DijsktraNode> dist(lux::BOARD_SIZE2);
    dijkstra(lightRobotPowerSaveGraph,1328,dist);

    json actions = json::object();
    for (const auto &[unitId, factory] : obs.factories[player]) {
        if (step % 4 < 3 && factory.canBuildLight(obs)) {
            actions[unitId] = factory.buildLight(obs);
        } else if (factory.canBuildHeavy(obs)) {
            actions[unitId] = factory.buildHeavy(obs);
        } else if (factory.canWater(obs)) {
            actions[unitId] = factory.water(obs);  // Alternatively set it to lux::FactoryAction::Water()
        }
    }
    for (const auto &[unitId, unit] : obs.units[player]) {
        for (int64_t i = 4; i < 5; ++i) {
            auto direction = lux::directionFromInt(i);
            auto moveCost = unit.moveCost(obs, direction);
            if (moveCost >= 0 && unit.power >= moveCost + unit.actionQueueCost(obs)) {
                LUX_LOG("ordering unit " << unit.unit_id << " to move in direction " << i);
                // Alternatively, push lux::UnitAction::Move(direction, 0)
                actions[unitId].push_back(unit.move(direction, 2));
                break;
            }
        }
    }
    // dump your created actions in a file by uncommenting this line
    // lux::dumpJsonToFile("last_actions.json", actions);
    // or log them by uncommenting this line
    // LUX_LOG(actions);
    return actions;
}
