#include "fault_detector.h"

#include <iostream>
#include <iomanip>


// --------------------------------------------------
// Configurable thresholds
// --------------------------------------------------

constexpr double CPU_WARNING_THRESHOLD = 80.0;
constexpr double CPU_CRITICAL_THRESHOLD = 95.0;

constexpr double MEMORY_WARNING_THRESHOLD = 80.0;
constexpr double MEMORY_CRITICAL_THRESHOLD = 95.0;

constexpr unsigned long long DROP_WARNING_THRESHOLD = 1;
constexpr unsigned long long ERROR_WARNING_THRESHOLD = 1;


// --------------------------------------------------
// Convert severity to text
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
// Print fault event
// --------------------------------------------------

void printFault(
    const FaultEvent& event)
{
    std::cout
        << "\n========================================\n";

    std::cout
        << "          NETWORK/SYSTEM ALERT\n";

    std::cout
        << "========================================\n";

    std::cout
        << "Severity   : "
        << severityToString(event.severity)
        << "\n";

    std::cout
        << "Agent      : "
        << event.agentId
        << "\n";

    std::cout
        << "Interface  : "
        << event.interfaceName
        << "\n";

    std::cout
        << "Fault Type : "
        << event.faultType
        << "\n";

    std::cout
        << "Description: "
        << event.description
        << "\n";

    std::cout
        << "========================================\n";
}


// --------------------------------------------------
// Evaluate telemetry
// --------------------------------------------------

void evaluateFaults(
    const AgentState& state)
{
    // ----------------------------------------------
    // CPU
    // ----------------------------------------------

    if (state.cpuUsage >=
        CPU_CRITICAL_THRESHOLD)
    {
        FaultEvent event{
            state.agentId,
            state.interfaceName,
            "HIGH_CPU",
            "CPU utilization is critically high.",
            FaultSeverity::CRITICAL
        };

        printFault(event);
    }
    else if (
        state.cpuUsage >=
        CPU_WARNING_THRESHOLD)
    {
        FaultEvent event{
            state.agentId,
            state.interfaceName,
            "HIGH_CPU",
            "CPU utilization is high.",
            FaultSeverity::WARNING
        };

        printFault(event);
    }


    // ----------------------------------------------
    // Memory
    // ----------------------------------------------

    if (state.memoryUsage >=
        MEMORY_CRITICAL_THRESHOLD)
    {
        FaultEvent event{
            state.agentId,
            state.interfaceName,
            "HIGH_MEMORY",
            "Memory utilization is critically high.",
            FaultSeverity::CRITICAL
        };

        printFault(event);
    }
    else if (
        state.memoryUsage >=
        MEMORY_WARNING_THRESHOLD)
    {
        FaultEvent event{
            state.agentId,
            state.interfaceName,
            "HIGH_MEMORY",
            "Memory utilization is high.",
            FaultSeverity::WARNING
        };

        printFault(event);
    }


    // ----------------------------------------------
    // RX errors
    // ----------------------------------------------

    if (state.rxErrors >=
        ERROR_WARNING_THRESHOLD)
    {
        FaultEvent event{
            state.agentId,
            state.interfaceName,
            "RX_ERRORS",
            "Receive errors detected on interface.",
            FaultSeverity::WARNING
        };

        printFault(event);
    }


    // ----------------------------------------------
    // TX errors
    // ----------------------------------------------

    if (state.txErrors >=
        ERROR_WARNING_THRESHOLD)
    {
        FaultEvent event{
            state.agentId,
            state.interfaceName,
            "TX_ERRORS",
            "Transmit errors detected on interface.",
            FaultSeverity::WARNING
        };

        printFault(event);
    }


    // ----------------------------------------------
    // RX drops
    // ----------------------------------------------

    if (state.rxDrops >=
        DROP_WARNING_THRESHOLD)
    {
        FaultEvent event{
            state.agentId,
            state.interfaceName,
            "RX_DROPS",
            "Receive packet drops detected.",
            FaultSeverity::WARNING
        };

        printFault(event);
    }


    // ----------------------------------------------
    // TX drops
    // ----------------------------------------------

    if (state.txDrops >=
        DROP_WARNING_THRESHOLD)
    {
        FaultEvent event{
            state.agentId,
            state.interfaceName,
            "TX_DROPS",
            "Transmit packet drops detected.",
            FaultSeverity::WARNING
        };

        printFault(event);
    }
}