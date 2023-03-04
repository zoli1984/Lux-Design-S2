#include "agent.hpp"

#include "lux/action.hpp"
#include "lux/common.hpp"
#include "lux/log.hpp"

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
    for (int i = 0; i < values.size(); i++) {
        posValue += values[i] * std::pow(0.5, i);
    }
    return posValue;
}


json Agent::setup() {
    if (step == 0) {
        return lux::BidAction(player == "player_1" ? "AlphaStrike" : "MotherMars", 0);
    }
    if (obs.teams[player].factories_to_place && isTurnToPlaceFactory()) {
        double materialBaseFactor = 0.5;
        // transform spawn_mask to positions
        const auto &spawns_mask = obs.board.valid_spawns_mask;
        lux::Position bestPos(-1,-1);
        double bestValue = -1;
        for (int x = 0; x < spawns_mask.size(); ++x) {
            for (int y = 0; y < spawns_mask[x].size(); ++y) {
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
        static size_t index = 0;
        return lux::SpawnAction(bestPos,
                                obs.teams[player].metal / 2,
                                obs.teams[player].water / 2);
    }
    return json::object();
}

json Agent::act() {
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
