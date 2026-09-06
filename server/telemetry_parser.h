#ifndef TELEMETRY_PARSER_H
#define TELEMETRY_PARSER_H

#include <string>

struct ParsedTelemetry
{
    std::string type;
    std::string agentId;
    std::string hostname;
    long long timestamp;

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

bool parseTelemetry(
    const std::string& message,
    ParsedTelemetry& telemetry
);

#endif