#include "network_metrics.h"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>


std::vector<NetworkStats> getNetworkStats()
{
    std::vector<NetworkStats> stats;

    std::ifstream file("/proc/net/dev");

    if (!file.is_open())
    {
        return stats;
    }

    std::string line;

    // Skip headers.
    std::getline(file, line);
    std::getline(file, line);

    while (std::getline(file, line))
    {
        if (line.empty())
        {
            continue;
        }

        std::size_t colon =
            line.find(':');

        if (colon == std::string::npos)
        {
            continue;
        }

        std::string interfaceName =
            line.substr(0, colon);

        // Remove leading/trailing spaces.
        std::size_t first =
            interfaceName.find_first_not_of(" \t");

        std::size_t last =
            interfaceName.find_last_not_of(" \t");

        if (first == std::string::npos)
        {
            continue;
        }

        interfaceName =
            interfaceName.substr(
                first,
                last - first + 1
            );

        std::string data =
            line.substr(colon + 1);

        std::istringstream stream(data);

        NetworkStats network{};

        network.interfaceName =
            interfaceName;

        // RX
        stream >> network.rxBytes;
        stream >> network.rxPackets;
        stream >> network.rxErrors;
        stream >> network.rxDrops;

        unsigned long long rxFifo;
        unsigned long long rxFrame;
        unsigned long long rxCompressed;
        unsigned long long rxMulticast;

        stream >> rxFifo
               >> rxFrame
               >> rxCompressed
               >> rxMulticast;

        // TX
        stream >> network.txBytes;
        stream >> network.txPackets;
        stream >> network.txErrors;
        stream >> network.txDrops;

        unsigned long long txFifo;
        unsigned long long txCollisions;
        unsigned long long txCarrier;
        unsigned long long txCompressed;

        stream >> txFifo
               >> txCollisions
               >> txCarrier
               >> txCompressed;

        if (stream)
        {
            stats.push_back(network);
        }
    }

    return stats;
}


std::vector<NetworkRates> calculateNetworkRates(
    const std::vector<NetworkStats>& previous,
    const std::vector<NetworkStats>& current,
    double elapsedSeconds)
{
    std::vector<NetworkRates> rates;

    if (elapsedSeconds <= 0.0)
    {
        return rates;
    }

    for (const auto& currentStats : current)
    {
        for (const auto& previousStats : previous)
        {
            if (currentStats.interfaceName ==
                previousStats.interfaceName)
            {
                NetworkRates rate{};

                rate.interfaceName =
                    currentStats.interfaceName;

                // Protect against counter reset/wrap.
                if (currentStats.rxBytes >=
                    previousStats.rxBytes)
                {
                    rate.rxBytesPerSecond =
                        static_cast<double>(
                            currentStats.rxBytes -
                            previousStats.rxBytes
                        ) / elapsedSeconds;
                }
                else
                {
                    rate.rxBytesPerSecond = 0.0;
                }

                if (currentStats.txBytes >=
                    previousStats.txBytes)
                {
                    rate.txBytesPerSecond =
                        static_cast<double>(
                            currentStats.txBytes -
                            previousStats.txBytes
                        ) / elapsedSeconds;
                }
                else
                {
                    rate.txBytesPerSecond = 0.0;
                }

                if (currentStats.rxPackets >=
                    previousStats.rxPackets)
                {
                    rate.rxPacketsPerSecond =
                        static_cast<double>(
                            currentStats.rxPackets -
                            previousStats.rxPackets
                        ) / elapsedSeconds;
                }
                else
                {
                    rate.rxPacketsPerSecond = 0.0;
                }

                if (currentStats.txPackets >=
                    previousStats.txPackets)
                {
                    rate.txPacketsPerSecond =
                        static_cast<double>(
                            currentStats.txPackets -
                            previousStats.txPackets
                        ) / elapsedSeconds;
                }
                else
                {
                    rate.txPacketsPerSecond = 0.0;
                }

                rates.push_back(rate);

                break;
            }
        }
    }

    return rates;
}