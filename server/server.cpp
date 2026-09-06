#include <iostream>
#include <string>
#include <cstring>
#include <iomanip>

#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../common/protocol.h"
#include "telemetry_parser.h"


int main()
{
    const int PORT = 9000;

    // --------------------------------------------------
    // 1. Create TCP socket
    // --------------------------------------------------

    int serverSocket = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (serverSocket < 0)
    {
        std::cerr
            << "Error: Could not create TCP socket.\n";

        return 1;
    }


    // --------------------------------------------------
    // 2. Allow address reuse
    // --------------------------------------------------

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


    // --------------------------------------------------
    // 3. Configure server address
    // --------------------------------------------------

    sockaddr_in serverAddress{};

    serverAddress.sin_family =
        AF_INET;

    // Accept connections on all interfaces
    serverAddress.sin_addr.s_addr =
        INADDR_ANY;

    serverAddress.sin_port =
        htons(PORT);


    // --------------------------------------------------
    // 4. Bind socket
    // --------------------------------------------------

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


    // --------------------------------------------------
    // 5. Listen for incoming connections
    // --------------------------------------------------

    if (listen(
            serverSocket,
            10) < 0)
    {
        std::cerr
            << "Error: Listen failed.\n";

        close(serverSocket);

        return 1;
    }


    // --------------------------------------------------
    // Server startup
    // --------------------------------------------------

    std::cout << "\n";
    std::cout
        << "========================================\n";

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
        << "\nWaiting for agent connection...\n";


    // --------------------------------------------------
    // 6. Accept one agent connection
    // --------------------------------------------------

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
            << "Error: Accept failed.\n";

        close(serverSocket);

        return 1;
    }


    // --------------------------------------------------
    // 7. Get client IP address
    // --------------------------------------------------

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


    // --------------------------------------------------
    // Agent connection information
    // --------------------------------------------------

    std::cout << "\n";
    std::cout
        << "========================================\n";

    std::cout
        << "AGENT CONNECTED\n";

    std::cout
        << "========================================\n";

    std::cout
        << "Client IP: "
        << clientIP
        << "\n";


    // --------------------------------------------------
    // 8. Continuously receive framed messages
    // --------------------------------------------------

    while (true)
    {
        std::string message;

        bool received =
            receiveFramedMessage(
                clientSocket,
                message
            );


        // ----------------------------------------------
        // Connection closed / receive failure
        // ----------------------------------------------

        if (!received)
        {
            std::cout
                << "\nAgent disconnected "
                   "or message reception failed.\n";

            break;
        }


        // ----------------------------------------------
        // Display raw message
        // ----------------------------------------------

        std::cout << "\n";
        std::cout
            << "----------------------------------------\n";

        std::cout
            << "Message received\n";

        std::cout
            << "----------------------------------------\n";

        std::cout
            << message
            << "\n";


        // ==============================================
        // HELLO
        // ==============================================

        if (message.rfind(
                "HELLO ",
                0) == 0)
        {
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


            std::cout
                << "WELCOME sent to agent.\n";
        }


        // ==============================================
        // TELEMETRY
        // ==============================================

        else if (
            message.rfind(
                "TYPE=TELEMETRY",
                0) == 0)
        {
            ParsedTelemetry telemetry{};


            bool valid =
                parseTelemetry(
                    message,
                    telemetry
                );


            if (!valid)
            {
                std::cerr
                    << "Error: Invalid telemetry message.\n";

                continue;
            }


            // ------------------------------------------
            // Display parsed telemetry
            // ------------------------------------------

            std::cout << "\n";

            std::cout
                << "========================================\n";

            std::cout
                << "         TELEMETRY DATA\n";

            std::cout
                << "========================================\n";


            std::cout
                << std::fixed
                << std::setprecision(2);


            std::cout
                << "Agent ID       : "
                << telemetry.agentId
                << "\n";


            std::cout
                << "Hostname       : "
                << telemetry.hostname
                << "\n";


            std::cout
                << "Timestamp      : "
                << telemetry.timestamp
                << "\n";


            std::cout
                << "CPU Usage      : "
                << telemetry.cpuUsage
                << " %\n";


            std::cout
                << "Memory Usage   : "
                << telemetry.memoryUsage
                << " %\n";


            std::cout
                << "Uptime         : "
                << telemetry.uptime
                << "\n";


            std::cout
                << "Interface      : "
                << telemetry.interfaceName
                << "\n";


            std::cout
                << "RX Rate        : "
                << telemetry.rxBytesPerSecond
                << " B/s\n";


            std::cout
                << "TX Rate        : "
                << telemetry.txBytesPerSecond
                << " B/s\n";


            std::cout
                << "RX Packet Rate : "
                << telemetry.rxPacketsPerSecond
                << " pkt/s\n";


            std::cout
                << "TX Packet Rate : "
                << telemetry.txPacketsPerSecond
                << " pkt/s\n";


            std::cout
                << "RX Errors      : "
                << telemetry.rxErrors
                << "\n";


            std::cout
                << "TX Errors      : "
                << telemetry.txErrors
                << "\n";


            std::cout
                << "RX Drops       : "
                << telemetry.rxDrops
                << "\n";


            std::cout
                << "TX Drops       : "
                << telemetry.txDrops
                << "\n";


            std::cout
                << "========================================\n";

            std::cout
                << "Telemetry parsed successfully.\n";
        }


        // ==============================================
        // HEARTBEAT
        // ==============================================

        else if (
            message.rfind(
                "HEARTBEAT",
                0) == 0)
        {
            std::cout
                << "Heartbeat received.\n";
        }


        // ==============================================
        // UNKNOWN MESSAGE
        // ==============================================

        else
        {
            std::cout
                << "Unknown message type.\n";
        }
    }


    // --------------------------------------------------
    // 9. Cleanup
    // --------------------------------------------------

    close(clientSocket);

    close(serverSocket);


    std::cout
        << "\nServer shutting down.\n";


    return 0;
}