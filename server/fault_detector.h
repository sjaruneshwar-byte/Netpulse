#ifndef FAULT_DETECTOR_H
#define FAULT_DETECTOR_H

#include <string>

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

void evaluateFaults(
    const AgentState& state
);

const char* severityToString(
    FaultSeverity severity
);

#endif