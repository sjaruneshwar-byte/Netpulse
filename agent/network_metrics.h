#ifndef NETWORK_METRICS_H
#define NETWORK_METRICS_H

#include <string>
#include <vector>


// --------------------------------------------------
// Raw network interface statistics
// --------------------------------------------------

struct NetworkStats
{
    std::string interfaceName;

    unsigned long long rxBytes;
    unsigned long long rxPackets;
    unsigned long long rxErrors;
    unsigned long long rxDrops;

    unsigned long long txBytes;
    unsigned long long txPackets;
    unsigned long long txErrors;
    unsigned long long txDrops;
};


// --------------------------------------------------
// Calculated network rates + error/drop counters
// --------------------------------------------------

struct NetworkRates
{
    std::string interfaceName;

    double rxBytesPerSecond;
    double txBytesPerSecond;

    double rxPacketsPerSecond;
    double txPacketsPerSecond;

    unsigned long long rxErrors;
    unsigned long long txErrors;

    unsigned long long rxDrops;
    unsigned long long txDrops;
};


// --------------------------------------------------
// Function declarations
// --------------------------------------------------

std::vector<NetworkStats> getNetworkStats();


std::vector<NetworkRates> calculateNetworkRates(
    const std::vector<NetworkStats>& previous,
    const std::vector<NetworkStats>& current,
    double elapsedSeconds
);


#endif