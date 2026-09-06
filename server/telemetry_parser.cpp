#include "telemetry_parser.h"

#include <sstream>
#include <string>


bool parseTelemetry(
    const std::string& message,
    ParsedTelemetry& telemetry)
{
    std::istringstream stream(message);

    std::string token;

    while (stream >> token)
    {
        std::size_t separator =
            token.find('=');

        if (separator == std::string::npos)
        {
            continue;
        }

        std::string key =
            token.substr(0, separator);

        std::string value =
            token.substr(separator + 1);


        if (key == "TYPE")
        {
            telemetry.type = value;
        }
        else if (key == "AGENT")
        {
            telemetry.agentId = value;
        }
        else if (key == "HOST")
        {
            telemetry.hostname = value;
        }
        else if (key == "TIMESTAMP")
        {
            telemetry.timestamp =
                std::stoll(value);
        }
        else if (key == "CPU")
        {
            telemetry.cpuUsage =
                std::stod(value);
        }
        else if (key == "MEM")
        {
            telemetry.memoryUsage =
                std::stod(value);
        }
        else if (key == "UPTIME")
        {
            telemetry.uptime = value;
        }
        else if (key == "IFACE")
        {
            telemetry.interfaceName = value;
        }
        else if (key == "RX_BPS")
        {
            telemetry.rxBytesPerSecond =
                std::stod(value);
        }
        else if (key == "TX_BPS")
        {
            telemetry.txBytesPerSecond =
                std::stod(value);
        }
        else if (key == "RX_PPS")
        {
            telemetry.rxPacketsPerSecond =
                std::stod(value);
        }
        else if (key == "TX_PPS")
        {
            telemetry.txPacketsPerSecond =
                std::stod(value);
        }
        else if (key == "RX_ERRORS")
        {
            telemetry.rxErrors =
                std::stoull(value);
        }
        else if (key == "TX_ERRORS")
        {
            telemetry.txErrors =
                std::stoull(value);
        }
        else if (key == "RX_DROPS")
        {
            telemetry.rxDrops =
                std::stoull(value);
        }
        else if (key == "TX_DROPS")
        {
            telemetry.txDrops =
                std::stoull(value);
        }
    }

    return telemetry.type == "TELEMETRY";
}