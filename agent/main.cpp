#include <iostream>
#include <string>
#include <unistd.h>
#include <limits.h>
#include <iomanip>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>

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
int main(int argc, char* argv[])
{
    // ----------------------------------------------
    // Agent configuration
    // ----------------------------------------------

    std::string agentId = "NODE_01";

    bool agentIdProvided = false;

    bool simulateNetworkFault = false;


    // ----------------------------------------------
    // Parse command-line arguments
    //
    // Examples:
    //
    // ./agent
    // ./agent NODE_01
    // ./agent NODE_01 --simulate-network-fault
    // ./agent --simulate-network-fault
    // ----------------------------------------------

    for (int i = 1; i < argc; ++i)
    {
        std::string argument = argv[i];

        if (argument ==
            "--simulate-network-fault")
        {
            simulateNetworkFault = true;
        }
        else if (!agentIdProvided)
        {
            agentId = argument;

            agentIdProvided = true;
        }
    }


    const std::string serverIP =
        "127.0.0.1";

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


    if (simulateNetworkFault)
    {
        std::cout
            << "Fault Simulation : NETWORK FAULT ENABLED\n";
    }
    else
    {
        std::cout
            << "Fault Simulation : OFF\n";
    }


    // ----------------------------------------------
    // Create TCP client
    // ----------------------------------------------

    TcpClient tcpClient;


    // ----------------------------------------------
    // Connect to server
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
    // Heartbeat thread
    // ----------------------------------------------

    std::atomic<bool> heartbeatRunning(true);


    std::thread heartbeatThread(
        [&tcpClient,
         &heartbeatRunning]()
        {
            while (heartbeatRunning)
            {
                std::this_thread::sleep_for(
                    std::chrono::seconds(2)
                );


                if (!heartbeatRunning)
                {
                    break;
                }


                if (tcpClient.isConnected())
                {
                    if (tcpClient.sendMessage(
                            "HEARTBEAT"))
                    {
                        std::cout
                            << "\nHeartbeat sent.\n";
                    }
                    else
                    {
                        std::cerr
                            << "\nWarning: Failed to "
                               "send heartbeat.\n";
                    }
                }
            }
        }
    );


    // ----------------------------------------------
    // Continuous monitoring
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
                << "Error: Could not read "
                   "system metrics.\n";

            break;
        }


        // ==========================================
        // DISPLAY SYSTEM STATUS
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
        // NETWORK SAMPLING INTERVAL
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
        // ELAPSED TIME
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


                    // ----------------------------------
                    // Real Linux counters
                    // ----------------------------------

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


                    // ----------------------------------
                    // Fault simulation
                    //
                    // This changes ONLY the telemetry
                    // values sent to NetPulse.
                    //
                    // It does NOT change the actual
                    // Linux interface counters.
                    // ----------------------------------

                    if (simulateNetworkFault)
                    {
                        rxErrors = 10;

                        txErrors = 5;

                        rxDrops = 25;

                        txDrops = 10;
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
                    // Create telemetry
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


                        if (simulateNetworkFault)
                        {
                            std::cout
                                << "SIMULATED NETWORK FAULT "
                                   "TELEMETRY SENT\n";

                            std::cout
                                << "RX_ERRORS=10 "
                                   "TX_ERRORS=5 "
                                   "RX_DROPS=25 "
                                   "TX_DROPS=10\n";
                        }
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

    heartbeatRunning = false;


    if (heartbeatThread.joinable())
    {
        heartbeatThread.join();
    }


    tcpClient.disconnect();


    return 0;
}