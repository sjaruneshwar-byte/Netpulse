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


void printDataRate(
    const std::string& label,
    double bytesPerSecond)
{
    std::cout << label;

    if (bytesPerSecond >=
        1024.0 * 1024.0)
    {
        std::cout
            << bytesPerSecond /
               (1024.0 * 1024.0)
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


void displayNetworkMetrics(
    const std::vector<NetworkStats>& previousStats,
    const std::vector<NetworkStats>& currentStats,
    double elapsedSeconds)
{
    std::vector<NetworkRates> rates =
        calculateNetworkRates(
            previousStats,
            currentStats,
            elapsedSeconds
        );

    std::cout << "\n";
    std::cout << "NETWORK METRICS\n";
    std::cout << "----------------------------------------\n";

    if (rates.empty())
    {
        std::cout << "No network interfaces found.\n";
        return;
    }

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

        std::cout << std::fixed
                  << std::setprecision(2);

        std::cout
            << "RX Packets: "
            << rate.rxPacketsPerSecond
            << " pkt/s\n";

        std::cout
            << "TX Packets: "
            << rate.txPacketsPerSecond
            << " pkt/s\n";
    }
}


int main()
{
    const std::string agentId =
        "NODE_01";

    char hostname[HOST_NAME_MAX + 1];

    if (gethostname(
            hostname,
            sizeof(hostname)) != 0)
    {
        std::cerr
            << "Error: Could not get hostname.\n";

        return 1;
    }

    hostname[HOST_NAME_MAX] =
        '\0';


    // ------------------------------------------
    // HEADER
    // ------------------------------------------

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


    // ------------------------------------------
    // CONTINUOUS MONITORING
    // ------------------------------------------

    while (true)
    {
        // ======================================
        // SYSTEM METRICS
        // ======================================

        double cpuUsage =
            getCpuUsage();

        double memoryUsage =
            getMemoryUsage();

        std::string uptime =
            getUptime();

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


        // ======================================
        // FIRST NETWORK SAMPLE
        // ======================================

        std::vector<NetworkStats>
            previousStats =
                getNetworkStats();

        auto startTime =
            std::chrono::steady_clock::now();


        // ======================================
        // SAMPLING INTERVAL
        // ======================================

        std::cout
            << "\nSampling network for 5 seconds...\n";

        std::this_thread::sleep_for(
            std::chrono::seconds(5)
        );


        // ======================================
        // SECOND NETWORK SAMPLE
        // ======================================

        std::vector<NetworkStats>
            currentStats =
                getNetworkStats();

        auto endTime =
            std::chrono::steady_clock::now();


        // ======================================
        // ACTUAL ELAPSED TIME
        // ======================================

        double elapsedSeconds =
            std::chrono::duration<double>(
                endTime - startTime
            ).count();


        // ======================================
        // NETWORK OUTPUT
        // ======================================

        displayNetworkMetrics(
            previousStats,
            currentStats,
            elapsedSeconds
        );

        std::cout << "\n";
        std::cout
            << "Next measurement in 5 seconds...\n";
    }

    return 0;
}