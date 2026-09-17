#include <iostream>
#include <string>
#include <cstring>
#include <iomanip>
#include <thread>
#include <chrono>
#include <vector>

#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../common/protocol.h"

#include "telemetry_parser.h"
#include "agent_registry.h"
#include "fault_detector.h"
#include "database.h"


// --------------------------------------------------
// Configuration
// --------------------------------------------------

constexpr int PORT = 9000;

constexpr int SUSPECT_TIMEOUT_SECONDS = 4;

constexpr int OFFLINE_TIMEOUT_SECONDS = 8;


// --------------------------------------------------
// Global components
// --------------------------------------------------

AgentRegistry agentRegistry;

Database database;

FaultTracker faultTracker;


// --------------------------------------------------
// Convert agent status to text
// --------------------------------------------------

const char* statusToString(
    AgentStatus status)
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


// --------------------------------------------------
// Background failure detector
// --------------------------------------------------

void monitorAgents()
{
    while (true)
    {
        std::this_thread::sleep_for(
            std::chrono::seconds(1)
        );

        auto agents =
            agentRegistry.getAllAgents();

        auto now =
            std::chrono::system_clock::now();


        for (const auto& entry : agents)
        {
            const AgentState& state =
                entry.second;


            // --------------------------------------
            // Already offline
            // --------------------------------------

            if (state.status ==
                AgentStatus::OFFLINE)
            {
                continue;
            }


            // --------------------------------------
            // Calculate time since last message
            // --------------------------------------

            auto elapsed =
                std::chrono::duration_cast<
                    std::chrono::seconds
                >(
                    now - state.lastSeen
                ).count();


            // --------------------------------------
            // HEALTHY -> SUSPECTED
            // --------------------------------------

            if (state.status ==
                    AgentStatus::HEALTHY &&
                elapsed >=
                    SUSPECT_TIMEOUT_SECONDS)
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
                    << "\n========================================\n";

                std::cout
                    << "AGENT SUSPECTED\n";

                std::cout
                    << "========================================\n";

                std::cout
                    << "Agent     : "
                    << state.agentId
                    << "\n";

                std::cout
                    << "Hostname  : "
                    << state.hostname
                    << "\n";

                std::cout
                    << "Last seen : "
                    << elapsed
                    << " seconds ago\n";

                std::cout
                    << "Status    : "
                    << statusToString(
                           AgentStatus::SUSPECTED)
                    << "\n";

                std::cout
                    << "========================================\n";
            }


            // --------------------------------------
            // SUSPECTED -> OFFLINE
            // --------------------------------------

            else if (
                state.status ==
                    AgentStatus::SUSPECTED &&
                elapsed >=
                    OFFLINE_TIMEOUT_SECONDS)
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
                    << "\n========================================\n";

                std::cout
                    << "AGENT OFFLINE\n";

                std::cout
                    << "========================================\n";

                std::cout
                    << "Agent     : "
                    << state.agentId
                    << "\n";

                std::cout
                    << "Hostname  : "
                    << state.hostname
                    << "\n";

                std::cout
                    << "Last seen : "
                    << elapsed
                    << " seconds ago\n";

                std::cout
                    << "Status    : "
                    << statusToString(
                           AgentStatus::OFFLINE)
                    << "\n";

                std::cout
                    << "========================================\n";
            }
        }
    }
}


// --------------------------------------------------
// Handle one connected agent
// --------------------------------------------------

void handleClient(
    int clientSocket,
    sockaddr_in clientAddress)
{
    char clientIP[INET_ADDRSTRLEN];


    // ----------------------------------------------
    // Convert client IP to string
    // ----------------------------------------------

    if (inet_ntop(
            AF_INET,
            &clientAddress.sin_addr,
            clientIP,
            sizeof(clientIP)) == nullptr)
    {
        std::strcpy(
            clientIP,
            "Unknown"
        );
    }


    std::cout
        << "\n========================================\n";

    std::cout
        << "AGENT CONNECTED\n";

    std::cout
        << "========================================\n";

    std::cout
        << "Client IP: "
        << clientIP
        << "\n";


    // Agent ID becomes known after HELLO.
    std::string connectedAgentId;


    // ------------------------------------------------
    // Receive messages continuously
    // ------------------------------------------------

    while (true)
    {
        std::string message;


        if (!receiveFramedMessage(
                clientSocket,
                message))
        {
            std::cout
                << "\nConnection lost from "
                << clientIP
                << ".\n";

            break;
        }


        // ============================================
        // HELLO
        // ============================================

        if (message.rfind(
                "HELLO ",
                0) == 0)
        {
            connectedAgentId =
                message.substr(6);


            const std::string response =
                "WELCOME FROM NETPULSE SERVER";


            if (!sendFramedMessage(
                    clientSocket,
                    response))
            {
                std::cerr
                    << "Error: Failed to send WELCOME "
                       "to "
                    << connectedAgentId
                    << ".\n";

                break;
            }


            // ----------------------------------------
            // Create initial agent state
            // ----------------------------------------

            AgentState state{};


            state.agentId =
                connectedAgentId;


            state.clientIP =
                clientIP;


            state.lastSeen =
                std::chrono::system_clock::now();


            state.status =
                AgentStatus::HEALTHY;


            // ----------------------------------------
            // Update memory registry
            // ----------------------------------------

            agentRegistry.updateAgent(
                state
            );


            // ----------------------------------------
            // Persist agent state
            // ----------------------------------------

            if (!database.saveAgentState(
                    state))
            {
                std::cerr
                    << "Warning: Failed to save "
                       "initial agent state.\n";
            }


            std::cout
                << "\nHELLO received from "
                << connectedAgentId
                << ".\n";

            std::cout
                << "Agent registered as HEALTHY.\n";

            std::cout
                << "WELCOME sent.\n";
        }


        // ============================================
        // TELEMETRY
        // ============================================

        else if (
            message.rfind(
                "TYPE=TELEMETRY",
                0) == 0)
        {
            ParsedTelemetry telemetry{};


            // ----------------------------------------
            // Parse telemetry
            // ----------------------------------------

            if (!parseTelemetry(
                    message,
                    telemetry))
            {
                std::cerr
                    << "\nInvalid telemetry from "
                    << clientIP
                    << ".\n";

                continue;
            }


            // ----------------------------------------
            // Build AgentState
            // ----------------------------------------

            AgentState state{};


            state.agentId =
                telemetry.agentId;


            state.hostname =
                telemetry.hostname;


            state.clientIP =
                clientIP;


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


            // ----------------------------------------
            // Update agent registry
            // ----------------------------------------

            agentRegistry.updateAgent(
                state
            );


            // ----------------------------------------
            // Persist agent state
            // ----------------------------------------

            if (!database.saveAgentState(
                    state))
            {
                std::cerr
                    << "Warning: Failed to save "
                       "agent state.\n";
            }


            // ----------------------------------------
            // Persist telemetry
            // ----------------------------------------

            if (database.saveTelemetry(
                    telemetry))
            {
                std::cout
                    << "Telemetry saved to SQLite.\n";
            }
            else
            {
                std::cerr
                    << "Warning: Failed to save "
                       "telemetry.\n";
            }


            // ----------------------------------------
            // Fault detection with deduplication
            // ----------------------------------------

            FaultUpdateResult faultUpdate =
                faultTracker.update(
                    state
                );


            // ----------------------------------------
            // Newly detected faults
            // ----------------------------------------

            for (const auto& fault :
                 faultUpdate.newFaults)
            {
                std::cout
                    << "\n========================================\n";

                std::cout
                    << "FAULT DETECTED\n";

                std::cout
                    << "========================================\n";

                std::cout
                    << "Agent      : "
                    << fault.agentId
                    << "\n";

                std::cout
                    << "Interface  : "
                    << fault.interfaceName
                    << "\n";

                std::cout
                    << "Fault Type : "
                    << fault.faultType
                    << "\n";

                std::cout
                    << "Severity   : "
                    << severityToString(
                           fault.severity)
                    << "\n";

                std::cout
                    << "Description: "
                    << fault.description
                    << "\n";

                std::cout
                    << "========================================\n";


                if (database.saveFault(
                        fault))
                {
                    std::cout
                        << "New fault event saved "
                           "to SQLite.\n";
                }
                else
                {
                    std::cerr
                        << "Warning: Failed to save "
                           "fault event.\n";
                }
            }


            // ----------------------------------------
            // Resolved faults
            // ----------------------------------------

            for (const auto& fault :
                 faultUpdate.resolvedFaults)
            {
                std::cout
                    << "\n========================================\n";

                std::cout
                    << "FAULT RESOLVED\n";

                std::cout
                    << "========================================\n";

                std::cout
                    << "Agent      : "
                    << fault.agentId
                    << "\n";

                std::cout
                    << "Interface  : "
                    << fault.interfaceName
                    << "\n";

                std::cout
                    << "Fault Type : "
                    << fault.faultType
                    << "\n";

                std::cout
                    << "Severity   : INFO\n";

                std::cout
                    << "Description: "
                    << fault.description
                    << "\n";

                std::cout
                    << "========================================\n";


                if (database.saveFault(
                        fault))
                {
                    std::cout
                        << "Fault resolution saved "
                           "to SQLite.\n";
                }
                else
                {
                    std::cerr
                        << "Warning: Failed to save "
                           "fault resolution.\n";
                }
            }


            // ----------------------------------------
            // Display telemetry
            // ----------------------------------------

            std::cout
                << "\n----------------------------------------\n";

            std::cout
                << "TELEMETRY FROM "
                << telemetry.agentId
                << "\n";

            std::cout
                << "----------------------------------------\n";


            std::cout
                << std::fixed
                << std::setprecision(2);


            std::cout
                << "CPU       : "
                << telemetry.cpuUsage
                << " %\n";


            std::cout
                << "Memory    : "
                << telemetry.memoryUsage
                << " %\n";


            std::cout
                << "Interface : "
                << telemetry.interfaceName
                << "\n";


            std::cout
                << "RX Rate   : "
                << telemetry.rxBytesPerSecond
                << " B/s\n";


            std::cout
                << "TX Rate   : "
                << telemetry.txBytesPerSecond
                << " B/s\n";


            std::cout
                << "RX Errors : "
                << telemetry.rxErrors
                << "\n";


            std::cout
                << "TX Errors : "
                << telemetry.txErrors
                << "\n";


            std::cout
                << "RX Drops  : "
                << telemetry.rxDrops
                << "\n";


            std::cout
                << "TX Drops  : "
                << telemetry.txDrops
                << "\n";
        }


        // ============================================
        // HEARTBEAT
        // ============================================

        else if (
            message.rfind(
                "HEARTBEAT",
                0) == 0)
        {
            if (!connectedAgentId.empty())
            {
                AgentState state{};


                if (agentRegistry.getAgent(
                        connectedAgentId,
                        state))
                {
                    AgentStatus previousStatus =
                        state.status;


                    state.lastSeen =
                        std::chrono::system_clock::now();


                    state.status =
                        AgentStatus::HEALTHY;


                    // ------------------------------
                    // Update memory registry
                    // ------------------------------

                    agentRegistry.updateAgent(
                        state
                    );


                    // ------------------------------
                    // Persist state
                    // ------------------------------

                    if (!database.saveAgentState(
                            state))
                    {
                        std::cerr
                            << "Warning: Failed to "
                               "save heartbeat state.\n";
                    }


                    // ------------------------------
                    // Recovery
                    // ------------------------------

                    if (previousStatus ==
                        AgentStatus::SUSPECTED ||
                        previousStatus ==
                        AgentStatus::OFFLINE)
                    {
                        std::cout
                            << "\n[RECOVERY] "
                            << connectedAgentId
                            << " is HEALTHY again.\n";
                    }
                }
            }


            std::cout
                << "HEARTBEAT received from "
                << connectedAgentId
                << ".\n";
        }


        // ============================================
        // UNKNOWN MESSAGE
        // ============================================

        else
        {
            std::cout
                << "\nUnknown message from "
                << clientIP
                << ":\n"
                << message
                << "\n";
        }
    }


    // ------------------------------------------------
    // Connection lost
    // ------------------------------------------------
    //
    // Do not immediately mark OFFLINE.
    // Mark SUSPECTED first and let the background
    // failure detector move it to OFFLINE.
    // ------------------------------------------------

    if (!connectedAgentId.empty())
    {
        AgentState state{};


        if (agentRegistry.getAgent(
                connectedAgentId,
                state))
        {
            state.status =
                AgentStatus::SUSPECTED;


            state.lastSeen =
                std::chrono::system_clock::now();


            agentRegistry.updateAgent(
                state
            );


            if (!database.saveAgentState(
                    state))
            {
                std::cerr
                    << "Warning: Failed to save "
                       "SUSPECTED agent state.\n";
            }


            std::cout
                << "\nAgent "
                << connectedAgentId
                << " marked SUSPECTED after "
                   "connection loss.\n";
        }
    }


    close(clientSocket);
}


// --------------------------------------------------
// MAIN
// --------------------------------------------------

int main()
{
    // ----------------------------------------------
    // Initialize SQLite
    // ----------------------------------------------

    if (!database.initialize(
            "netpulse.db"))
    {
        std::cerr
            << "Error: Database initialization failed.\n";

        return 1;
    }


    // ----------------------------------------------
    // Create TCP socket
    // ----------------------------------------------

    int serverSocket =
        socket(
            AF_INET,
            SOCK_STREAM,
            0
        );


    if (serverSocket < 0)
    {
        std::cerr
            << "Error: Could not create socket.\n";

        return 1;
    }


    // ----------------------------------------------
    // Allow address reuse
    // ----------------------------------------------

    int option = 1;


    if (setsockopt(
            serverSocket,
            SOL_SOCKET,
            SO_REUSEADDR,
            &option,
            sizeof(option)) < 0)
    {
        std::cerr
            << "Error: setsockopt failed.\n";

        close(serverSocket);

        return 1;
    }


    // ----------------------------------------------
    // Configure server address
    // ----------------------------------------------

    sockaddr_in serverAddress{};


    serverAddress.sin_family =
        AF_INET;


    serverAddress.sin_addr.s_addr =
        INADDR_ANY;


    serverAddress.sin_port =
        htons(PORT);


    // ----------------------------------------------
    // Bind
    // ----------------------------------------------

    if (bind(
            serverSocket,
            reinterpret_cast<sockaddr*>(
                &serverAddress),
            sizeof(serverAddress)) < 0)
    {
        std::cerr
            << "Error: Bind failed.\n";

        close(serverSocket);

        return 1;
    }


    // ----------------------------------------------
    // Listen
    // ----------------------------------------------

    if (listen(
            serverSocket,
            20) < 0)
    {
        std::cerr
            << "Error: Listen failed.\n";

        close(serverSocket);

        return 1;
    }


    // ----------------------------------------------
    // Startup information
    // ----------------------------------------------

    std::cout
        << "\n========================================\n";

    std::cout
        << "       NetPulse Monitoring Server\n";

    std::cout
        << "========================================\n\n";


    std::cout
        << "SQLite database : netpulse.db\n";


    std::cout
        << "Listening on port: "
        << PORT
        << "\n";


    std::cout
        << "Suspect timeout : "
        << SUSPECT_TIMEOUT_SECONDS
        << " seconds\n";


    std::cout
        << "Offline timeout : "
        << OFFLINE_TIMEOUT_SECONDS
        << " seconds\n";


    std::cout
        << "\nWaiting for agents...\n";


    // ----------------------------------------------
    // Start background failure detector
    // ----------------------------------------------

    std::thread monitorThread(
        monitorAgents
    );


    monitorThread.detach();


    // ----------------------------------------------
    // Accept multiple agents
    // ----------------------------------------------

    while (true)
    {
        sockaddr_in clientAddress{};


        socklen_t clientLength =
            sizeof(clientAddress);


        int clientSocket =
            accept(
                serverSocket,
                reinterpret_cast<sockaddr*>(
                    &clientAddress),
                &clientLength
            );


        if (clientSocket < 0)
        {
            std::cerr
                << "Error: accept() failed.\n";

            continue;
        }


        // ------------------------------------------
        // Dedicated handler thread
        // ------------------------------------------

        std::thread clientThread(
            handleClient,
            clientSocket,
            clientAddress
        );


        clientThread.detach();


        std::cout
            << "\nNew agent connection accepted.\n";

        std::cout
            << "Agent handler thread started.\n";
    }


    close(serverSocket);

    return 0;
}