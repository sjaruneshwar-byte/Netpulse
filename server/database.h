#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <mutex>

#include "telemetry_parser.h"
#include "fault_detector.h"
#include "agent_state.h"


class Database
{
public:

    Database();

    ~Database();

    bool initialize(
        const std::string& databasePath
    );

    bool saveTelemetry(
        const ParsedTelemetry& telemetry
    );

    bool saveFault(
        const FaultEvent& fault
    );

    bool saveAgentState(
        const AgentState& state
    );

    bool updateAgentStatus(
        const std::string& agentId,
        AgentStatus status
    );

private:

    void* db;

    mutable std::mutex databaseMutex;
};

#endif