#include "agent_registry.h"


void AgentRegistry::updateAgent(
    const AgentState& state)
{
    std::lock_guard<std::mutex> lock(
        registryMutex
    );

    agents[state.agentId] = state;
}


void AgentRegistry::markDisconnected(
    const std::string& agentId)
{
    std::lock_guard<std::mutex> lock(
        registryMutex
    );

    auto it = agents.find(agentId);

    if (it != agents.end())
    {
        it->second.connected = false;
    }
}


bool AgentRegistry::getAgent(
    const std::string& agentId,
    AgentState& state)
{
    std::lock_guard<std::mutex> lock(
        registryMutex
    );

    auto it = agents.find(agentId);

    if (it == agents.end())
    {
        return false;
    }

    state = it->second;

    return true;
}


std::unordered_map<std::string, AgentState>
AgentRegistry::getAllAgents()
{
    std::lock_guard<std::mutex> lock(
        registryMutex
    );

    return agents;
}