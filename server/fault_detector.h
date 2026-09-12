#ifndef FAULT_DETECTOR_H
#define FAULT_DETECTOR_H

#include <string>
#include <vector>

#include "agent_state.h"


enum class FaultSeverity
{
    INFO,
    WARNING,
    CRITICAL
};


struct FaultEvent
{
    std::string agentId;
    std::string interfaceName;
    std::string faultType;
    std::string description;

    FaultSeverity severity;
};


const char* severityToString(
    FaultSeverity severity
);


std::vector<FaultEvent> evaluateFaults(
    const AgentState& state
);

#endif