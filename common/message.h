#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>

struct TelemetryMessage
{
    std::string agentId;
    std::string hostname;

    double cpuUsage;
    double memoryUsage;

    std::string uptime;

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

#endif