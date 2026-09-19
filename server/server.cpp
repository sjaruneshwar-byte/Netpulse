#include <iostream>
#include <string>
#include <cstring>
#include <iomanip>
#include <thread>
#include <chrono>
#include <sstream>
#include <cctype>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "../common/protocol.h"
#include "telemetry_parser.h"
#include "agent_registry.h"
#include "fault_detector.h"
#include "database.h"
#include "input_validator.h"

namespace
{
constexpr int PORT = 9000;
constexpr int SUSPECT_TIMEOUT_SECONDS = 4;
constexpr int OFFLINE_TIMEOUT_SECONDS = 8;

std::string statusToString(AgentStatus status)
{
    switch (status)
    {
        case AgentStatus::HEALTHY:
            return "HEALTHY";

        case AgentStatus::SUSPECTED:
            return "SUSPECTED";

        case AgentStatus::OFFLINE:
            return "OFFLINE";
    }

    return "UNKNOWN";
}

std::string trim(const std::string& value)
{
    const std::size_t first = value.find_first_not_of(" \t\r\n");

    if (first == std::string::npos)
    {
        return "";
    }

    const std::size_t last = value.find_last_not_of(" \t\r\n");

    return value.substr(first, last - first + 1);
}

std::string extractField(
    const std::string& message,
    const std::string& key
)
{
    const std::string prefix = key + "=";

    const std::size_t startPos = message.find(prefix);

    if (startPos == std::string::npos)
    {
        return "";
    }

    std::size_t start = startPos + prefix.length();

    std::size_t end = message.find(' ', start);

    if (end == std::string::npos)
    {
        end = message.length();
    }

    return message.substr(start, end - start);
}

std::string extractHelloAgentId(const std::string& message)
{
    // Supports:
    // HELLO NODE_01
    // HELLO AGENT=NODE_01

    std::string rest = trim(message.substr(5));

    if (rest.empty())
    {
        return "";
    }

    const std::string agentField = extractField(rest, "AGENT");

    if (!agentField.empty())
    {
        return agentField;
    }

    // If the message is simply:
    // HELLO NODE_01
    const std::size_t firstSpace = rest.find(' ');

    if (firstSpace == std::string::npos)
    {
        return rest;
    }

    return trim(rest.substr(0, firstSpace));
}
}

AgentRegistry agentRegistry;
Database database;
FaultTracker faultTracker;

void monitorAgents()
{
    while (true)
    {
        std::this_thread::sleep_for(
            std::chrono::seconds(1)
        );

        const auto agents = agentRegistry.getAllAgents();

        const auto now =
            std::chrono::system_clock::now();

        // getAllAgents() returns:
        // std::unordered_map<std::string, AgentState>
        //
        // Therefore each item is a pair:
        // entry.first  -> agent ID
        // entry.second -> AgentState

        for (const auto& entry : agents)
        {
            const AgentState& state = entry.second;

            if (state.status == AgentStatus::OFFLINE)
            {
                continue;
            }

            const auto elapsed =
                std::chrono::duration_cast<std::chrono::seconds>(
                    now - state.lastSeen
                ).count();

            // HEALTHY -> SUSPECTED
            if (
                state.status == AgentStatus::HEALTHY &&
                elapsed >= SUSPECT_TIMEOUT_SECONDS
            )
            {
                agentRegistry.setStatus(
                    state.agentId,
                    AgentStatus::SUSPECTED
                );

                database.updateAgentStatus(
                    state.agentId,
                    AgentStatus::SUSPECTED
                );

                std::cout
                    << "\n----------------------------------------\n"
                    << "AGENT SUSPECTED\n"
                    << "----------------------------------------\n"
                    << "Agent     : "
                    << state.agentId
                    << "\n"
                    << "Last seen : "
                    << elapsed
                    << " seconds ago\n"
                    << "Status    : SUSPECTED\n"
                    << "----------------------------------------\n"
                    << std::endl;
            }

            // SUSPECTED -> OFFLINE
            else if (
                state.status == AgentStatus::SUSPECTED &&
                elapsed >= OFFLINE_TIMEOUT_SECONDS
            )
            {
                agentRegistry.setStatus(
                    state.agentId,
                    AgentStatus::OFFLINE
                );

                database.updateAgentStatus(
                    state.agentId,
                    AgentStatus::OFFLINE
                );

                std::cout
                    << "\n----------------------------------------\n"
                    << "AGENT OFFLINE\n"
                    << "----------------------------------------\n"
                    << "Agent     : "
                    << state.agentId
                    << "\n"
                    << "Last seen : "
                    << elapsed
                    << " seconds ago\n"
                    << "Status    : OFFLINE\n"
                    << "----------------------------------------\n"
                    << std::endl;
            }
        }
    }
}

void handleClient(
    int clientSocket,
    const sockaddr_in& clientAddress
)
{
    char clientIp[INET_ADDRSTRLEN] = {};

    if (
        inet_ntop(
            AF_INET,
            &clientAddress.sin_addr,
            clientIp,
            sizeof(clientIp)
        ) == nullptr
    )
    {
        std::strncpy(
            clientIp,
            "UNKNOWN",
            sizeof(clientIp) - 1
        );

        clientIp[sizeof(clientIp) - 1] = '\0';
    }

    std::string agentId;
    bool identified = false;

    while (true)
    {
        std::string message;

        if (!receiveFramedMessage(clientSocket, message))
        {
            break;
        }

        message = trim(message);

        if (message.empty())
        {
            continue;
        }

        // ============================================================
        // HELLO
        // ============================================================
        if (message.rfind("HELLO", 0) == 0)
        {
            agentId = extractHelloAgentId(message);

            if (agentId.empty())
            {
                std::cerr
                    << "Rejected HELLO from "
                    << clientIp
                    << ": missing agent ID."
                    << std::endl;

                break;
            }

            if (agentId.size() > 64)
            {
                std::cerr
                    << "Rejected HELLO from "
                    << clientIp
                    << ": agent ID too long."
                    << std::endl;

                break;
            }

            identified = true;

            const std::string welcomeMessage =
                "WELCOME AGENT=" + agentId;

            if (
                !sendFramedMessage(
                    clientSocket,
                    welcomeMessage
                )
            )
            {
                std::cerr
                    << "Failed to send WELCOME to "
                    << agentId
                    << "."
                    << std::endl;

                break;
            }

            AgentState state;

            state.agentId = agentId;
            state.hostname = "UNKNOWN";
            state.clientIP = clientIp;
            state.lastSeen =
                std::chrono::system_clock::now();
            state.status =
                AgentStatus::HEALTHY;

            agentRegistry.updateAgent(state);

            if (!database.saveAgentState(state))
            {
                std::cerr
                    << "Failed to save initial state for "
                    << agentId
                    << "."
                    << std::endl;
            }

            std::cout
                << "\n========================================\n"
                << "AGENT CONNECTED\n"
                << "========================================\n"
                << "Agent     : "
                << agentId
                << "\n"
                << "Client IP : "
                << clientIp
                << "\n"
                << "Status    : HEALTHY\n"
                << "========================================\n"
                << std::endl;

            continue;
        }

        // ============================================================
        // HEARTBEAT
        // ============================================================
        if (message.rfind("HEARTBEAT", 0) == 0)
        {
            std::string heartbeatAgentId =
                extractField(
                    message,
                    "AGENT"
                );

            // Existing agent connection may send simply:
            // HEARTBEAT
            if (heartbeatAgentId.empty())
            {
                heartbeatAgentId = agentId;
            }

            if (heartbeatAgentId.empty())
            {
                std::cerr
                    << "Rejected HEARTBEAT from "
                    << clientIp
                    << ": missing agent ID."
                    << std::endl;

                continue;
            }

            if (heartbeatAgentId.size() > 64)
            {
                std::cerr
                    << "Rejected HEARTBEAT from "
                    << clientIp
                    << ": agent ID too long."
                    << std::endl;

                continue;
            }

            AgentState existingState;

            if (
                agentRegistry.getAgent(
                    heartbeatAgentId,
                    existingState
                )
            )
            {
                const AgentStatus previousStatus =
                    existingState.status;

                existingState.lastSeen =
                    std::chrono::system_clock::now();

                existingState.clientIP =
                    clientIp;

                existingState.status =
                    AgentStatus::HEALTHY;

                agentRegistry.updateAgent(
                    existingState
                );

                database.saveAgentState(
                    existingState
                );

                if (
                    previousStatus == AgentStatus::SUSPECTED ||
                    previousStatus == AgentStatus::OFFLINE
                )
                {
                    std::cout
                        << "\n========================================\n"
                        << "AGENT RECOVERED\n"
                        << "========================================\n"
                        << "Agent     : "
                        << heartbeatAgentId
                        << "\n"
                        << "Previous  : "
                        << statusToString(previousStatus)
                        << "\n"
                        << "Current   : HEALTHY\n"
                        << "========================================\n"
                        << std::endl;
                }
            }
            else
            {
                AgentState newState;

                newState.agentId =
                    heartbeatAgentId;

                newState.hostname =
                    "UNKNOWN";

                newState.clientIP =
                    clientIp;

                newState.lastSeen =
                    std::chrono::system_clock::now();

                newState.status =
                    AgentStatus::HEALTHY;

                agentRegistry.updateAgent(
                    newState
                );

                database.saveAgentState(
                    newState
                );
            }

            std::cout
                << "HEARTBEAT received from "
                << heartbeatAgentId
                << "."
                << std::endl;

            continue;
        }

        // ============================================================
        // TELEMETRY
        // ============================================================
        if (
            message.rfind("TYPE=TELEMETRY", 0) == 0
        )
        {
            ParsedTelemetry telemetry;

            // --------------------------------------------------------
            // Parse telemetry
            // --------------------------------------------------------
            if (!parseTelemetry(message, telemetry))
            {
                std::cerr
                    << "Rejected malformed telemetry from "
                    << agentId
                    << "."
                    << std::endl;

                continue;
            }

            // --------------------------------------------------------
            // Validate telemetry
            // --------------------------------------------------------
            std::string validationError;

            if (
                !validateTelemetry(
                    telemetry,
                    validationError
                )
            )
            {
                std::cerr
                    << "Rejected invalid telemetry from "
                    << telemetry.agentId
                    << ": "
                    << validationError
                    << std::endl;

                continue;
            }

            // --------------------------------------------------------
            // Verify agent ID
            // --------------------------------------------------------
            if (telemetry.agentId.empty())
            {
                std::cerr
                    << "Rejected telemetry: missing agent ID."
                    << std::endl;

                continue;
            }

            if (telemetry.agentId.size() > 64)
            {
                std::cerr
                    << "Rejected telemetry: agent ID too long."
                    << std::endl;

                continue;
            }

            // --------------------------------------------------------
            // Prevent one client from impersonating another agent
            // --------------------------------------------------------
            if (
                identified &&
                !agentId.empty() &&
                telemetry.agentId != agentId
            )
            {
                std::cerr
                    << "Rejected telemetry: agent identity mismatch."
                    << std::endl;

                continue;
            }

            // --------------------------------------------------------
            // Build AgentState
            // --------------------------------------------------------
            AgentState state;

            state.agentId =
                telemetry.agentId;

            state.hostname =
                telemetry.hostname;

            state.clientIP =
                clientIp;

            state.cpuUsage =
                telemetry.cpuUsage;

            state.memoryUsage =
                telemetry.memoryUsage;

            state.uptime =
                telemetry.uptime;

            state.interfaceName =
                telemetry.interfaceName;

            state.rxBytesPerSecond =
                telemetry.rxBytesPerSecond;

            state.txBytesPerSecond =
                telemetry.txBytesPerSecond;

            state.rxPacketsPerSecond =
                telemetry.rxPacketsPerSecond;

            state.txPacketsPerSecond =
                telemetry.txPacketsPerSecond;

            state.rxErrors =
                telemetry.rxErrors;

            state.txErrors =
                telemetry.txErrors;

            state.rxDrops =
                telemetry.rxDrops;

            state.txDrops =
                telemetry.txDrops;

            state.lastSeen =
                std::chrono::system_clock::now();

            state.status =
                AgentStatus::HEALTHY;

            // --------------------------------------------------------
            // Update registry
            // --------------------------------------------------------
            agentRegistry.updateAgent(state);

            // --------------------------------------------------------
            // Save persistent agent state
            // --------------------------------------------------------
            if (
                !database.saveAgentState(state)
            )
            {
                std::cerr
                    << "Failed to save agent state for "
                    << telemetry.agentId
                    << "."
                    << std::endl;
            }

            // --------------------------------------------------------
            // Save telemetry
            // --------------------------------------------------------
            if (
                database.saveTelemetry(telemetry)
            )
            {
                std::cout
                    << "Telemetry saved to SQLite."
                    << std::endl;
            }
            else
            {
                std::cerr
                    << "Failed to save telemetry to SQLite."
                    << std::endl;
            }

            // --------------------------------------------------------
            // Fault detection
            // --------------------------------------------------------
            FaultUpdateResult faultUpdate =
                faultTracker.update(state);

            // --------------------------------------------------------
            // New faults
            // --------------------------------------------------------
            for (
                const FaultEvent& fault :
                faultUpdate.newFaults
            )
            {
                if (
                    database.saveFault(fault)
                )
                {
                    std::cout
                        << "\n========================================\n"
                        << "NEW FAULT\n"
                        << "========================================\n"
                        << "Agent     : "
                        << fault.agentId
                        << "\n"
                        << "Interface : "
                        << fault.interfaceName
                        << "\n"
                        << "Type      : "
                        << fault.faultType
                        << "\n"
                        << "Severity  : "
                        << severityToString(
                               fault.severity
                           )
                        << "\n"
                        << "Description: "
                        << fault.description
                        << "\n"
                        << "========================================\n"
                        << std::endl;
                }
            }

            // --------------------------------------------------------
            // Resolved faults
            // --------------------------------------------------------
            for (
                const FaultEvent& fault :
                faultUpdate.resolvedFaults
            )
            {
                if (
                    database.saveFault(fault)
                )
                {
                    std::cout
                        << "\n========================================\n"
                        << "FAULT RESOLVED\n"
                        << "========================================\n"
                        << "Agent     : "
                        << fault.agentId
                        << "\n"
                        << "Interface : "
                        << fault.interfaceName
                        << "\n"
                        << "Type      : "
                        << fault.faultType
                        << "\n"
                        << "Severity  : "
                        << severityToString(
                               fault.severity
                           )
                        << "\n"
                        << "Description: "
                        << fault.description
                        << "\n"
                        << "========================================\n"
                        << std::endl;
                }
            }

            // --------------------------------------------------------
            // Display telemetry
            // --------------------------------------------------------
            std::cout
                << "\n----------------------------------------\n"
                << "TELEMETRY FROM "
                << telemetry.agentId
                << "\n"
                << "----------------------------------------\n"
                << std::fixed
                << std::setprecision(2)
                << "CPU       : "
                << telemetry.cpuUsage
                << " %\n"
                << "Memory    : "
                << telemetry.memoryUsage
                << " %\n"
                << "Interface : "
                << telemetry.interfaceName
                << "\n"
                << "RX Rate   : "
                << telemetry.rxBytesPerSecond
                << " B/s\n"
                << "TX Rate   : "
                << telemetry.txBytesPerSecond
                << " B/s\n"
                << "RX Errors : "
                << telemetry.rxErrors
                << "\n"
                << "TX Errors : "
                << telemetry.txErrors
                << "\n"
                << "RX Drops  : "
                << telemetry.rxDrops
                << "\n"
                << "TX Drops  : "
                << telemetry.txDrops
                << "\n"
                << "----------------------------------------\n"
                << std::endl;

            continue;
        }

        // ============================================================
        // UNKNOWN MESSAGE
        // ============================================================
        std::cerr
            << "Unknown message received from "
            << clientIp
            << ": "
            << message
            << std::endl;
    }

    // ================================================================
    // CONNECTION LOST
    // ================================================================
    if (!agentId.empty())
    {
        AgentState state;

        if (
            agentRegistry.getAgent(
                agentId,
                state
            )
        )
        {
            if (
                state.status != AgentStatus::OFFLINE
            )
            {
                state.status =
                    AgentStatus::SUSPECTED;

                state.lastSeen =
                    std::chrono::system_clock::now();

                state.clientIP =
                    clientIp;

                agentRegistry.updateAgent(
                    state
                );

                database.saveAgentState(
                    state
                );

                database.updateAgentStatus(
                    agentId,
                    AgentStatus::SUSPECTED
                );

                std::cout
                    << "Agent "
                    << agentId
                    << " marked SUSPECTED "
                    << "after connection loss."
                    << std::endl;
            }
        }
    }

    std::cout
        << "Connection lost from "
        << clientIp
        << "."
        << std::endl;

    close(clientSocket);
}

int main()
{
    std::cout
        << "========================================\n"
        << "          NetPulse Server\n"
        << "========================================\n";

    // ================================================================
    // DATABASE
    // ================================================================
    if (
        !database.initialize(
            "netpulse.db"
        )
    )
    {
        std::cerr
            << "Failed to initialize database."
            << std::endl;

        return 1;
    }

    std::cout
        << "Database initialized successfully."
        << std::endl;

    // ================================================================
    // SERVER SOCKET
    // ================================================================
    const int serverSocket =
        socket(
            AF_INET,
            SOCK_STREAM,
            0
        );

    if (serverSocket < 0)
    {
        std::cerr
            << "Failed to create socket."
            << std::endl;

        return 1;
    }

    int reuse = 1;

    if (
        setsockopt(
            serverSocket,
            SOL_SOCKET,
            SO_REUSEADDR,
            &reuse,
            sizeof(reuse)
        ) < 0
    )
    {
        std::cerr
            << "Warning: failed to set SO_REUSEADDR."
            << std::endl;
    }

    sockaddr_in serverAddress{};

    serverAddress.sin_family =
        AF_INET;

    serverAddress.sin_addr.s_addr =
        INADDR_ANY;

    serverAddress.sin_port =
        htons(PORT);

    if (
        bind(
            serverSocket,
            reinterpret_cast<sockaddr*>(
                &serverAddress
            ),
            sizeof(serverAddress)
        ) < 0
    )
    {
        std::cerr
            << "Failed to bind server socket to port "
            << PORT
            << "."
            << std::endl;

        close(serverSocket);

        return 1;
    }

    if (
        listen(
            serverSocket,
            SOMAXCONN
        ) < 0
    )
    {
        std::cerr
            << "Failed to listen on port "
            << PORT
            << "."
            << std::endl;

        close(serverSocket);

        return 1;
    }

    std::cout
        << "Server listening on port "
        << PORT
        << "..."
        << std::endl;

    // ================================================================
    // BACKGROUND AGENT MONITOR
    // ================================================================
    std::thread monitorThread(
        monitorAgents
    );

    monitorThread.detach();

    // ================================================================
    // ACCEPT CLIENT CONNECTIONS
    // ================================================================
    while (true)
    {
        sockaddr_in clientAddress{};

        socklen_t clientAddressLength =
            sizeof(clientAddress);

        const int clientSocket =
            accept(
                serverSocket,
                reinterpret_cast<sockaddr*>(
                    &clientAddress
                ),
                &clientAddressLength
            );

        if (clientSocket < 0)
        {
            std::cerr
                << "Failed to accept client connection."
                << std::endl;

            continue;
        }

        std::cout
            << "New client connection accepted."
            << std::endl;

        std::thread clientThread(
            handleClient,
            clientSocket,
            clientAddress
        );

        clientThread.detach();
    }

    close(serverSocket);

    return 0;
}