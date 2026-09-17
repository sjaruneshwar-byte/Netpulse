#ifndef FAULT_DETECTOR_H
#define FAULT_DETECTOR_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

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


struct FaultUpdateResult
{
    std::vector<FaultEvent> newFaults;
    std::vector<FaultEvent> resolvedFaults;
};


const char* severityToString(
    FaultSeverity severity
);


std::vector<FaultEvent> evaluateFaults(
    const AgentState& state
);


class FaultTracker
{
public:

    FaultUpdateResult update(
        const AgentState& state
    );

    void clearAgent(
        const std::string& agentId
    );

private:

    std::string makeKey(
        const FaultEvent& fault
    ) const;

    std::unordered_map<
        std::string,
        FaultEvent
    > activeFaults;

    std::mutex faultMutex;
};

#endif