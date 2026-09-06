#ifndef NETWORK_METRICS_H
#define NETWORK_METRICS_H

#include <string>
#include <vector>

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

struct NetworkRates
{
    std::string interfaceName;

    double rxBytesPerSecond;
    double txBytesPerSecond;

    double rxPacketsPerSecond;
    double txPacketsPerSecond;
};

std::vector<NetworkStats> getNetworkStats();

std::vector<NetworkRates> calculateNetworkRates(
    const std::vector<NetworkStats>& previous,
    const std::vector<NetworkStats>& current,
    double elapsedSeconds
);

#endif