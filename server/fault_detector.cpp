#include "fault_detector.h"

#include <iostream>


// --------------------------------------------------
// Thresholds
// --------------------------------------------------

constexpr double CPU_WARNING_THRESHOLD = 80.0;
constexpr double CPU_CRITICAL_THRESHOLD = 95.0;

constexpr double MEMORY_WARNING_THRESHOLD = 80.0;
constexpr double MEMORY_CRITICAL_THRESHOLD = 95.0;

constexpr unsigned long long DROP_WARNING_THRESHOLD = 1;
constexpr unsigned long long ERROR_WARNING_THRESHOLD = 1;


// --------------------------------------------------
// Severity -> string
// --------------------------------------------------

const char* severityToString(
    FaultSeverity severity)
{
    switch (severity)
    {
        case FaultSeverity::INFO:
            return "INFO";

        case FaultSeverity::WARNING:
            return "WARNING";

        case FaultSeverity::CRITICAL:
            return "CRITICAL";
    }

    return "UNKNOWN";
}


// --------------------------------------------------
// Detect currently active fault conditions
// --------------------------------------------------

std::vector<FaultEvent> evaluateFaults(
    const AgentState& state)
{
    std::vector<FaultEvent> faults;


    // ----------------------------------------------
    // CPU
    // ----------------------------------------------

    if (state.cpuUsage >=
        CPU_CRITICAL_THRESHOLD)
    {
        faults.push_back({
            state.agentId,
            state.interfaceName,
            "HIGH_CPU",
            "CPU utilization is critically high.",
            FaultSeverity::CRITICAL
        });
    }
    else if (
        state.cpuUsage >=
        CPU_WARNING_THRESHOLD)
    {
        faults.push_back({
            state.agentId,
            state.interfaceName,
            "HIGH_CPU",
            "CPU utilization is high.",
            FaultSeverity::WARNING
        });
    }


    // ----------------------------------------------
    // Memory
    // ----------------------------------------------

    if (state.memoryUsage >=
        MEMORY_CRITICAL_THRESHOLD)
    {
        faults.push_back({
            state.agentId,
            state.interfaceName,
            "HIGH_MEMORY",
            "Memory utilization is critically high.",
            FaultSeverity::CRITICAL
        });
    }
    else if (
        state.memoryUsage >=
        MEMORY_WARNING_THRESHOLD)
    {
        faults.push_back({
            state.agentId,
            state.interfaceName,
            "HIGH_MEMORY",
            "Memory utilization is high.",
            FaultSeverity::WARNING
        });
    }


    // ----------------------------------------------
    // RX errors
    // ----------------------------------------------

    if (state.rxErrors >=
        ERROR_WARNING_THRESHOLD)
    {
        faults.push_back({
            state.agentId,
            state.interfaceName,
            "RX_ERRORS",
            "Receive errors detected on interface.",
            FaultSeverity::WARNING
        });
    }


    // ----------------------------------------------
    // TX errors
    // ----------------------------------------------

    if (state.txErrors >=
        ERROR_WARNING_THRESHOLD)
    {
        faults.push_back({
            state.agentId,
            state.interfaceName,
            "TX_ERRORS",
            "Transmit errors detected on interface.",
            FaultSeverity::WARNING
        });
    }


    // ----------------------------------------------
    // RX drops
    // ----------------------------------------------

    if (state.rxDrops >=
        DROP_WARNING_THRESHOLD)
    {
        faults.push_back({
            state.agentId,
            state.interfaceName,
            "RX_DROPS",
            "Receive packet drops detected.",
            FaultSeverity::WARNING
        });
    }


    // ----------------------------------------------
    // TX drops
    // ----------------------------------------------

    if (state.txDrops >=
        DROP_WARNING_THRESHOLD)
    {
        faults.push_back({
            state.agentId,
            state.interfaceName,
            "TX_DROPS",
            "Transmit packet drops detected.",
            FaultSeverity::WARNING
        });
    }


    return faults;
}


// --------------------------------------------------
// Build unique fault key
// --------------------------------------------------

std::string FaultTracker::makeKey(
    const FaultEvent& fault) const
{
    return
        fault.agentId +
        "|" +
        fault.interfaceName +
        "|" +
        fault.faultType;
}


// --------------------------------------------------
// Update fault state
// --------------------------------------------------

FaultUpdateResult FaultTracker::update(
    const AgentState& state)
{
    std::lock_guard<std::mutex> lock(
        faultMutex
    );


    FaultUpdateResult result;


    std::vector<FaultEvent> currentFaults =
        evaluateFaults(state);


    std::unordered_map<
        std::string,
        FaultEvent
    > currentFaultMap;


    // ----------------------------------------------
    // Find newly active faults
    // ----------------------------------------------

    for (const auto& fault : currentFaults)
    {
        std::string key =
            makeKey(fault);


        currentFaultMap[key] =
            fault;


        auto existing =
            activeFaults.find(key);


        if (existing ==
            activeFaults.end())
        {
            result.newFaults.push_back(
                fault
            );
        }
    }


    // ----------------------------------------------
    // Find resolved faults
    // ----------------------------------------------

    for (const auto& entry : activeFaults)
    {
        const std::string& key =
            entry.first;

        const FaultEvent& oldFault =
            entry.second;


        if (currentFaultMap.find(key) ==
            currentFaultMap.end())
        {
            FaultEvent resolved =
                oldFault;


            resolved.severity =
                FaultSeverity::INFO;


            resolved.description =
                "Fault resolved: " +
                oldFault.description;


            result.resolvedFaults.push_back(
                resolved
            );
        }
    }


    // ----------------------------------------------
    // Replace active-fault state
    // ----------------------------------------------

    activeFaults =
        std::move(currentFaultMap);


    return result;
}


// --------------------------------------------------
// Clear all faults for an agent
// --------------------------------------------------

void FaultTracker::clearAgent(
    const std::string& agentId)
{
    std::lock_guard<std::mutex> lock(
        faultMutex
    );


    for (auto it = activeFaults.begin();
         it != activeFaults.end();)
    {
        if (it->second.agentId ==
            agentId)
        {
            it =
                activeFaults.erase(it);
        }
        else
        {
            ++it;
        }
    }
}