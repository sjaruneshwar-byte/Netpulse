#ifndef AGENT_STATE_H
#define AGENT_STATE_H

#include <string>
#include <chrono>


enum class AgentStatus
{
    HEALTHY,
    SUSPECTED,
    OFFLINE
};


struct AgentState
{
    std::string agentId;
    std::string hostname;
    std::string clientIP;

    double cpuUsage = 0.0;
    double memoryUsage = 0.0;

    std::string uptime;
    std::string interfaceName;

    double rxBytesPerSecond = 0.0;
    double txBytesPerSecond = 0.0;

    double rxPacketsPerSecond = 0.0;
    double txPacketsPerSecond = 0.0;

    unsigned long long rxErrors = 0;
    unsigned long long txErrors = 0;

    unsigned long long rxDrops = 0;
    unsigned long long txDrops = 0;

    std::chrono::system_clock::time_point lastSeen;

    AgentStatus status = AgentStatus::OFFLINE;
};

#endif