#include <iostream>
#include <string>
#include <fstream>

#include "agent.hpp"
#include "lux/json.hpp"
#include "lux/log.hpp"

//#define SAVE_DEBUG
//#define LOAD_DEBUG

void logInput(std::string fileName, json s) {
    // declaring character array (+1 for null terminator)
    char* char_array = new char[fileName.length() + 1];

    // copying the contents of the
    // string to char array
    strcpy(char_array, fileName.c_str());
    lux::dumpJsonToFile(char_array, s);
}

int main() {
    Agent agent;
#if defined(SAVE_DEBUG) || defined(LOAD_DEBUG)
    int turn = 0;
#endif
    while (std::cin && !std::cin.eof()) {
#if defined(SAVE_DEBUG) || defined(LOAD_DEBUG)
        turn++;
        std::string turnString = std::format("turns/{}_input.json", std::to_string(turn));
#endif
        json input;
        
#ifdef LOAD_DEBUG
        std::ifstream f(std::format("Debug/{}", turnString));
        //std::ifstream f("d:/kaggle/Lux2Make/Debug/turns/1_input.json");
        
        //std::string a;
        //    f >> a;
        f >> input;
#else
        std::cin >> input;
#endif
#ifdef SAVE_DEBUG
        logInput((turnString), input);
#endif
        lux::dumpJsonToFile("input.json", input);

        input.at("step").get_to(agent.step);
        input.at("player").get_to(agent.player);
        input.at("remainingOverageTime").get_to(agent.remainingOverageTime);

        // parsing logic performs delta calculation
        input.at("obs").get_to(agent.obs);

        if (agent.step == 0) {
            input.at("info").at("env_cfg").get_to(agent.obs.config);
        }

        json output;
        if (agent.obs.real_env_steps < 0) {
            output = agent.setup();
        } else {
            agent.step = agent.obs.real_env_steps;
            output     = agent.act();
        }
        std::cout << output << std::endl;
    }
    return 0;
}
