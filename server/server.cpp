#include <iostream>
#include <string>
#include <cstring>
#include <iomanip>
#include <thread>
#include <chrono>

#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../common/protocol.h"
#include "telemetry_parser.h"
#include "agent_registry.h"


// --------------------------------------------------
// Configuration
// --------------------------------------------------

constexpr int PORT = 9000;

// Agent is considered offline if nothing is
// received for longer than this duration.
constexpr int HEARTBEAT_TIMEOUT_SECONDS = 6;


// --------------------------------------------------
// Shared registry
// --------------------------------------------------

AgentRegistry agentRegistry;


// --------------------------------------------------
// Check agent health continuously
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

            if (!state.connected)
            {
                continue;
            }

            auto elapsed =
                std::chrono::duration_cast<
                    std::chrono::seconds
                >(
                    now - state.lastSeen
                ).count();

            if (elapsed >
                HEARTBEAT_TIMEOUT_SECONDS)
            {
                agentRegistry.markDisconnected(
                    state.agentId
                );

                std::cout
                    << "\n";
                std::cout
                    << "========================================\n";

                std::cout
                    << "⚠ AGENT OFFLINE\n";

                std::cout
                    << "========================================\n";

                std::cout
                    << "Agent      : "
                    << state.agentId
                    << "\n";

                std::cout
                    << "Hostname   : "
                    << state.hostname
                    << "\n";

                std::cout
                    << "Last seen  : "
                    << elapsed
                    << " seconds ago\n";

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


    std::string connectedAgentId;


    // ------------------------------------------------
    // Receive messages from this agent
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
                    << "Error: Failed to send WELCOME.\n";

                break;
            }


            AgentState state{};

            state.agentId =
                connectedAgentId;

            state.clientIP =
                clientIP;

            state.connected =
                true;

            state.lastSeen =
                std::chrono::system_clock::now();


            agentRegistry.updateAgent(
                state
            );


            std::cout
                << "\nHELLO received from "
                << connectedAgentId
                << ".\n";

            std::cout
                << "Agent registered.\n";
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

            // Every valid telemetry message also
            // proves that the agent is alive.
            state.lastSeen =
                std::chrono::system_clock::now();

            state.connected =
                true;


            agentRegistry.updateAgent(
                state
            );


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
                    state.lastSeen =
                        std::chrono::system_clock::now();

                    state.connected =
                        true;

                    agentRegistry.updateAgent(
                        state
                    );
                }
            }

            std::cout
                << "\nHEARTBEAT received from ";

            if (connectedAgentId.empty())
            {
                std::cout
                    << clientIP;
            }
            else
            {
                std::cout
                    << connectedAgentId;
            }

            std::cout
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
    // Connection ended
    // ------------------------------------------------

    if (!connectedAgentId.empty())
    {
        agentRegistry.markDisconnected(
            connectedAgentId
        );

        std::cout
            << "Agent "
            << connectedAgentId
            << " marked disconnected.\n";
    }


    close(clientSocket);
}


// --------------------------------------------------
// MAIN
// --------------------------------------------------

int main()
{
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
    // Server address
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
    // Startup
    // ----------------------------------------------

    std::cout
        << "\n========================================\n";

    std::cout
        << "       NetPulse Monitoring Server\n";

    std::cout
        << "========================================\n\n";

    std::cout
        << "Server started successfully.\n";

    std::cout
        << "Listening on port: "
        << PORT
        << "\n";

    std::cout
        << "Heartbeat timeout: "
        << HEARTBEAT_TIMEOUT_SECONDS
        << " seconds\n";

    std::cout
        << "\nWaiting for agents...\n";


    // ----------------------------------------------
    // Start background health monitor
    // ----------------------------------------------

    std::thread monitorThread(
        monitorAgents
    );

    monitorThread.detach();


    // ----------------------------------------------
    // Accept agents
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
        // Create a handler thread
        // ------------------------------------------

        std::thread clientThread(
            handleClient,
            clientSocket,
            clientAddress
        );

        clientThread.detach();


        std::cout
            << "\nNew agent connection accepted.\n";
    }


    close(serverSocket);

    return 0;
}