#include "fault_detector.h"

#include <iostream>


constexpr double CPU_WARNING_THRESHOLD = 80.0;
constexpr double CPU_CRITICAL_THRESHOLD = 95.0;

constexpr double MEMORY_WARNING_THRESHOLD = 80.0;
constexpr double MEMORY_CRITICAL_THRESHOLD = 95.0;

constexpr unsigned long long DROP_WARNING_THRESHOLD = 1;
constexpr unsigned long long ERROR_WARNING_THRESHOLD = 1;


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