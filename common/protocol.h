#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>
#include <cstdint>


bool sendFramedMessage(
    int socketFd,
    const std::string& message
);


bool receiveFramedMessage(
    int socketFd,
    std::string& message
);


std::string createTelemetryMessage(
    const std::string& agentId,
    const std::string& hostname,
    long long timestamp,
    double cpuUsage,
    double memoryUsage,
    const std::string& uptime,
    const std::string& interfaceName,
    double rxBytesPerSecond,
    double txBytesPerSecond,
    double rxPacketsPerSecond,
    double txPacketsPerSecond,
    unsigned long long rxErrors,
    unsigned long long txErrors,
    unsigned long long rxDrops,
    unsigned long long txDrops
);

#endif