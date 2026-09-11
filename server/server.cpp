#include <iostream>
#include <string>
#include <cstring>
#include <iomanip>
#include <thread>
#include <chrono>
#include "fault_detector.h"
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../common/protocol.h"
#include "telemetry_parser.h"
#include "agent_registry.h"


constexpr int PORT = 9000;

constexpr int SUSPECT_TIMEOUT_SECONDS = 4;

constexpr int OFFLINE_TIMEOUT_SECONDS = 8;


AgentRegistry agentRegistry;


// --------------------------------------------------
// Convert status to string
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

            if (state.status ==
                AgentStatus::OFFLINE)
            {
                continue;
            }

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

                std::cout
                    << "\n========================================\n";

                std::cout
                    << "⚠ AGENT SUSPECTED\n";

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

                std::cout
                    << "\n========================================\n";

                std::cout
                    << "🔴 AGENT OFFLINE\n";

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


        // ==========================================
        // HELLO
        // ==========================================

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
                break;
            }


            AgentState state{};

            state.agentId =
                connectedAgentId;

            state.clientIP =
                clientIP;

            state.lastSeen =
                std::chrono::system_clock::now();

            state.status =
                AgentStatus::HEALTHY;

            agentRegistry.updateAgent(
                state
            );


            std::cout
                << "\nHELLO received from "
                << connectedAgentId
                << ".\n";
        }


        // ==========================================
        // TELEMETRY
        // ==========================================

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
                    << "Invalid telemetry.\n";

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

            state.lastSeen =
                std::chrono::system_clock::now();

            // Any valid telemetry means the node
            // is alive again.
            state.status =
                AgentStatus::HEALTHY;


            agentRegistry.updateAgent(
                state
            );
            evaluateFaults(state);


            std::cout
                << "\nTelemetry from "
                << telemetry.agentId
                << " | CPU="
                << telemetry.cpuUsage
                << "% | MEM="
                << telemetry.memoryUsage
                << "% | IFACE="
                << telemetry.interfaceName
                << "\n";
        }


        // ==========================================
        // HEARTBEAT
        // ==========================================

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

                    agentRegistry.updateAgent(
                        state
                    );


                    if (previousStatus ==
                        AgentStatus::SUSPECTED)
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


        // ==========================================
        // UNKNOWN MESSAGE
        // ==========================================

        else
        {
            std::cout
                << "Unknown message: "
                << message
                << "\n";
        }
    }


    // ------------------------------------------------
    // TCP connection lost
    // ------------------------------------------------
    //
    // Don't immediately mark OFFLINE.
    // First mark SUSPECTED and let the
    // failure detector decide when to mark
    // the agent OFFLINE.
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


            std::cout
                << "\n⚠ Agent "
                << connectedAgentId
                << " marked SUSPECTED "
                   "after connection loss.\n";
        }
    }


    close(clientSocket);
}


// --------------------------------------------------
// MAIN
// --------------------------------------------------

int main()
{
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


    int option = 1;

    setsockopt(
        serverSocket,
        SOL_SOCKET,
        SO_REUSEADDR,
        &option,
        sizeof(option)
    );


    sockaddr_in serverAddress{};

    serverAddress.sin_family =
        AF_INET;

    serverAddress.sin_addr.s_addr =
        INADDR_ANY;

    serverAddress.sin_port =
        htons(PORT);


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


    if (listen(
            serverSocket,
            20) < 0)
    {
        std::cerr
            << "Error: Listen failed.\n";

        close(serverSocket);

        return 1;
    }


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
        << "Suspect timeout: "
        << SUSPECT_TIMEOUT_SECONDS
        << " seconds\n";

    std::cout
        << "Offline timeout: "
        << OFFLINE_TIMEOUT_SECONDS
        << " seconds\n";

    std::cout
        << "\nWaiting for agents...\n";


    // Start failure detector
    std::thread monitorThread(
        monitorAgents
    );

    monitorThread.detach();


    // Accept multiple agents
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