#include <iostream>
#include <string>
#include <unistd.h>
#include <limits.h>
#include <iomanip>
#include <chrono>
#include <thread>
#include <vector>

#include "system_metrics.h"
#include "network_metrics.h"
#include "tcp_client.h"
#include "../common/protocol.h"


// --------------------------------------------------
// Convert bytes/sec into a readable format
// --------------------------------------------------
void printDataRate(
    const std::string& label,
    double bytesPerSecond)
{
    std::cout << label;

    if (bytesPerSecond >= 1024.0 * 1024.0)
    {
        std::cout
            << bytesPerSecond / (1024.0 * 1024.0)
            << " MB/s";
    }
    else if (bytesPerSecond >= 1024.0)
    {
        std::cout
            << bytesPerSecond / 1024.0
            << " KB/s";
    }
    else
    {
        std::cout
            << bytesPerSecond
            << " B/s";
    }

    std::cout << "\n";
}


// --------------------------------------------------
// Find matching raw network statistics
// --------------------------------------------------
const NetworkStats* findNetworkStats(
    const std::vector<NetworkStats>& stats,
    const std::string& interfaceName)
{
    for (const auto& stat : stats)
    {
        if (stat.interfaceName == interfaceName)
        {
            return &stat;
        }
    }

    return nullptr;
}


// --------------------------------------------------
// MAIN
// --------------------------------------------------
int main()
{
    // ----------------------------------------------
    // Agent configuration
    // ----------------------------------------------

    const std::string agentId = "NODE_01";

    const std::string serverIP = "127.0.0.1";

    const int serverPort = 9000;


    // ----------------------------------------------
    // Get hostname
    // ----------------------------------------------

    char hostname[HOST_NAME_MAX + 1];

    if (gethostname(
            hostname,
            sizeof(hostname)) != 0)
    {
        std::cerr
            << "Error: Could not get hostname.\n";

        return 1;
    }

    hostname[HOST_NAME_MAX] = '\0';


    // ----------------------------------------------
    // Agent header
    // ----------------------------------------------

    std::cout << "\n";

    std::cout
        << "========================================\n";

    std::cout
        << "       NetPulse Monitoring Agent\n";

    std::cout
        << "========================================\n\n";

    std::cout
        << "Agent ID  : "
        << agentId
        << "\n";

    std::cout
        << "Hostname  : "
        << hostname
        << "\n";

    std::cout
        << "Platform  : Linux / WSL2\n";


    // ----------------------------------------------
    // Create persistent TCP client
    // ----------------------------------------------

    TcpClient tcpClient;


    // ----------------------------------------------
    // Connect to monitoring server
    // ----------------------------------------------

    std::cout << "\n";

    std::cout
        << "NETWORK CONNECTION\n";

    std::cout
        << "----------------------------------------\n";

    bool serverConnected =
        tcpClient.connectToServer(
            serverIP,
            serverPort,
            agentId
        );

    if (!serverConnected)
    {
        std::cerr
            << "Warning: Could not connect to "
               "NetPulse server.\n";

        std::cerr
            << "Local monitoring will continue.\n";
    }


    // ----------------------------------------------
    // Continuous monitoring loop
    // ----------------------------------------------

    while (true)
    {
        // ==========================================
        // SYSTEM METRICS
        // ==========================================

        double cpuUsage =
            getCpuUsage();

        double memoryUsage =
            getMemoryUsage();

        std::string uptime =
            getUptime();


        if (cpuUsage < 0 ||
            memoryUsage < 0)
        {
            std::cerr
                << "Error: Could not read system metrics.\n";

            break;
        }


        // ==========================================
        // DISPLAY SYSTEM METRICS
        // ==========================================

        std::cout << "\n";

        std::cout
            << "========================================\n";

        std::cout
            << "SYSTEM STATUS\n";

        std::cout
            << "========================================\n";

        std::cout
            << std::fixed
            << std::setprecision(2);

        std::cout
            << "CPU Usage    : "
            << cpuUsage
            << " %\n";

        std::cout
            << "Memory Usage : "
            << memoryUsage
            << " %\n";

        std::cout
            << "Uptime       : "
            << uptime
            << "\n";


        // ==========================================
        // FIRST NETWORK SAMPLE
        // ==========================================

        std::vector<NetworkStats>
            previousStats =
                getNetworkStats();

        auto startTime =
            std::chrono::steady_clock::now();


        std::cout
            << "\nSampling network for 5 seconds...\n";


        // ==========================================
        // WAIT
        // ==========================================

        std::this_thread::sleep_for(
            std::chrono::seconds(5)
        );


        // ==========================================
        // SECOND NETWORK SAMPLE
        // ==========================================

        std::vector<NetworkStats>
            currentStats =
                getNetworkStats();

        auto endTime =
            std::chrono::steady_clock::now();


        // ==========================================
        // ACTUAL ELAPSED TIME
        // ==========================================

        double elapsedSeconds =
            std::chrono::duration<double>(
                endTime - startTime
            ).count();


        // ==========================================
        // CALCULATE NETWORK RATES
        // ==========================================

        std::vector<NetworkRates>
            rates =
                calculateNetworkRates(
                    previousStats,
                    currentStats,
                    elapsedSeconds
                );


        // ==========================================
        // DISPLAY NETWORK METRICS
        // ==========================================

        std::cout << "\n";

        std::cout
            << "NETWORK METRICS\n";

        std::cout
            << "----------------------------------------\n";


        if (rates.empty())
        {
            std::cout
                << "No network interfaces found.\n";
        }
        else
        {
            for (const auto& rate : rates)
            {
                std::cout << "\n";

                std::cout
                    << "Interface : "
                    << rate.interfaceName
                    << "\n";

                printDataRate(
                    "RX Rate   : ",
                    rate.rxBytesPerSecond
                );

                printDataRate(
                    "TX Rate   : ",
                    rate.txBytesPerSecond
                );

                std::cout
                    << "RX Packets: "
                    << rate.rxPacketsPerSecond
                    << " pkt/s\n";

                std::cout
                    << "TX Packets: "
                    << rate.txPacketsPerSecond
                    << " pkt/s\n";

                std::cout
                    << "RX Errors : "
                    << rate.rxErrors
                    << "\n";

                std::cout
                    << "TX Errors : "
                    << rate.txErrors
                    << "\n";

                std::cout
                    << "RX Drops  : "
                    << rate.rxDrops
                    << "\n";

                std::cout
                    << "TX Drops  : "
                    << rate.txDrops
                    << "\n";


                // ==================================
                // SEND TELEMETRY
                // ==================================

                if (tcpClient.isConnected())
                {
                    const NetworkStats*
                        rawStats =
                            findNetworkStats(
                                currentStats,
                                rate.interfaceName
                            );


                    unsigned long long rxErrors = 0;
                    unsigned long long txErrors = 0;
                    unsigned long long rxDrops = 0;
                    unsigned long long txDrops = 0;


                    if (rawStats != nullptr)
                    {
                        rxErrors =
                            rawStats->rxErrors;

                        txErrors =
                            rawStats->txErrors;

                        rxDrops =
                            rawStats->rxDrops;

                        txDrops =
                            rawStats->txDrops;
                    }


                    // ==================================
                    // Generate Unix timestamp
                    // ==================================

                    auto currentTime =
                        std::chrono::system_clock::now();

                    long long timestamp =
                        std::chrono::duration_cast<
                            std::chrono::seconds
                        >(
                            currentTime.time_since_epoch()
                        ).count();


                    // ==================================
                    // Encode uptime
                    // ==================================

                    std::string encodedUptime =
                        uptime;

                    for (char& c : encodedUptime)
                    {
                        if (c == ' ')
                        {
                            c = '_';
                        }
                    }


                    // ==================================
                    // Create telemetry message
                    // ==================================

                    std::string telemetry =
                        createTelemetryMessage(
                            agentId,
                            hostname,
                            timestamp,
                            cpuUsage,
                            memoryUsage,
                            encodedUptime,
                            rate.interfaceName,
                            rate.rxBytesPerSecond,
                            rate.txBytesPerSecond,
                            rate.rxPacketsPerSecond,
                            rate.txPacketsPerSecond,
                            rxErrors,
                            txErrors,
                            rxDrops,
                            txDrops
                        );


                    // ==================================
                    // Send telemetry
                    // ==================================

                    if (tcpClient.sendMessage(
                            telemetry))
                    {
                        std::cout
                            << "\nTelemetry sent successfully "
                               "for interface "
                            << rate.interfaceName
                            << ".\n";
                    }
                    else
                    {
                        std::cerr
                            << "\nWarning: Failed to send "
                               "telemetry.\n";
                    }
                }
            }
        }


        std::cout
            << "\nNext measurement in 5 seconds...\n";
    }


    // ----------------------------------------------
    // Cleanup
    // ----------------------------------------------

    tcpClient.disconnect();

    return 0;
}