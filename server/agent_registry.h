#ifndef AGENT_REGISTRY_H
#define AGENT_REGISTRY_H

#include <string>
#include <unordered_map>
#include <mutex>

#include "agent_state.h"


class AgentRegistry
{
public:

    void updateAgent(
        const AgentState& state
    );


    bool getAgent(
        const std::string& agentId,
        AgentState& state
    );


    std::unordered_map<
        std::string,
        AgentState
    > getAllAgents();


    bool setStatus(
        const std::string& agentId,
        AgentStatus status
    );


private:

    std::unordered_map<
        std::string,
        AgentState
    > agents;

    mutable std::mutex registryMutex;
};


#endif