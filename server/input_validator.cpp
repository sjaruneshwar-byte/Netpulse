#include "input_validator.h"

#include <cctype>
#include <cmath>

namespace
{
bool validText(
    const std::string& value,
    std::size_t maxLength
)
{
    if (value.empty() || value.size() > maxLength)
    {
        return false;
    }

    for (unsigned char ch : value)
    {
        if (!std::isprint(ch))
        {
            return false;
        }
    }

    return true;
}
}

bool validateTelemetry(
    const ParsedTelemetry& telemetry,
    std::string& reason
)
{
    if (telemetry.type != "TELEMETRY")
    {
        reason = "Invalid message type";
        return false;
    }

    if (!validText(telemetry.agentId, 64))
    {
        reason = "Invalid agent ID";
        return false;
    }

    if (!validText(telemetry.hostname, 255))
    {
        reason = "Invalid hostname";
        return false;
    }

    if (!validText(telemetry.interfaceName, 64))
    {
        reason = "Invalid interface name";
        return false;
    }

    if (!std::isfinite(telemetry.cpuUsage) ||
        telemetry.cpuUsage < 0.0 ||
        telemetry.cpuUsage > 100.0)
    {
        reason = "CPU usage outside valid range";
        return false;
    }

    if (!std::isfinite(telemetry.memoryUsage) ||
        telemetry.memoryUsage < 0.0 ||
        telemetry.memoryUsage > 100.0)
    {
        reason = "Memory usage outside valid range";
        return false;
    }

    if (!std::isfinite(telemetry.rxBytesPerSecond) ||
        telemetry.rxBytesPerSecond < 0.0)
    {
        reason = "Invalid RX byte rate";
        return false;
    }

    if (!std::isfinite(telemetry.txBytesPerSecond) ||
        telemetry.txBytesPerSecond < 0.0)
    {
        reason = "Invalid TX byte rate";
        return false;
    }

    if (!std::isfinite(telemetry.rxPacketsPerSecond) ||
        telemetry.rxPacketsPerSecond < 0.0)
    {
        reason = "Invalid RX packet rate";
        return false;
    }

    if (!std::isfinite(telemetry.txPacketsPerSecond) ||
        telemetry.txPacketsPerSecond < 0.0)
    {
        reason = "Invalid TX packet rate";
        return false;
    }

    return true;
}
