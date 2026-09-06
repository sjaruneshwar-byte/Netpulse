#include "protocol.h"

#include <cstring>
#include <iostream>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>


// --------------------------------------------------
// Send all bytes
// --------------------------------------------------

bool sendAll(
    int socketFd,
    const char* data,
    std::size_t totalBytes)
{
    std::size_t bytesSent = 0;

    while (bytesSent < totalBytes)
    {
        ssize_t result =
            send(
                socketFd,
                data + bytesSent,
                totalBytes - bytesSent,
                0
            );

        if (result <= 0)
        {
            return false;
        }

        bytesSent +=
            static_cast<std::size_t>(result);
    }

    return true;
}


// --------------------------------------------------
// Receive all bytes
// --------------------------------------------------

bool receiveAll(
    int socketFd,
    char* data,
    std::size_t totalBytes)
{
    std::size_t bytesReceived = 0;

    while (bytesReceived < totalBytes)
    {
        ssize_t result =
            recv(
                socketFd,
                data + bytesReceived,
                totalBytes - bytesReceived,
                0
            );

        if (result <= 0)
        {
            return false;
        }

        bytesReceived +=
            static_cast<std::size_t>(result);
    }

    return true;
}


// --------------------------------------------------
// Send length-prefixed message
// --------------------------------------------------

bool sendFramedMessage(
    int socketFd,
    const std::string& message)
{
    std::uint32_t messageLength =
        static_cast<std::uint32_t>(
            message.size()
        );

    std::uint32_t networkLength =
        htonl(messageLength);


    // Send message length first
    if (!sendAll(
            socketFd,
            reinterpret_cast<char*>(
                &networkLength
            ),
            sizeof(networkLength)))
    {
        return false;
    }


    // Send message body
    if (!message.empty())
    {
        if (!sendAll(
                socketFd,
                message.data(),
                message.size()))
        {
            return false;
        }
    }

    return true;
}


// --------------------------------------------------
// Receive length-prefixed message
// --------------------------------------------------

bool receiveFramedMessage(
    int socketFd,
    std::string& message)
{
    std::uint32_t networkLength = 0;

    if (!receiveAll(
            socketFd,
            reinterpret_cast<char*>(
                &networkLength
            ),
            sizeof(networkLength)))
    {
        return false;
    }


    std::uint32_t messageLength =
        ntohl(networkLength);


    // Basic protection against unreasonable messages
    const std::uint32_t MAX_MESSAGE_SIZE =
        1024 * 1024;


    if (messageLength > MAX_MESSAGE_SIZE)
    {
        std::cerr
            << "Error: Message too large.\n";

        return false;
    }


    message.resize(messageLength);


    if (messageLength > 0)
    {
        if (!receiveAll(
                socketFd,
                message.data(),
                messageLength))
        {
            return false;
        }
    }

    return true;
}


// --------------------------------------------------
// Create telemetry message
// --------------------------------------------------

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
    unsigned long long txDrops)
{
    std::string message;

    message += "TYPE=TELEMETRY";
    message += " AGENT=" + agentId;
    message += " HOST=" + hostname;

    message +=
        " TIMESTAMP=" +
        std::to_string(timestamp);

    message +=
        " CPU=" +
        std::to_string(cpuUsage);

    message +=
        " MEM=" +
        std::to_string(memoryUsage);

    message +=
        " UPTIME=" +
        uptime;

    message +=
        " IFACE=" +
        interfaceName;

    message +=
        " RX_BPS=" +
        std::to_string(rxBytesPerSecond);

    message +=
        " TX_BPS=" +
        std::to_string(txBytesPerSecond);

    message +=
        " RX_PPS=" +
        std::to_string(rxPacketsPerSecond);

    message +=
        " TX_PPS=" +
        std::to_string(txPacketsPerSecond);

    message +=
        " RX_ERRORS=" +
        std::to_string(rxErrors);

    message +=
        " TX_ERRORS=" +
        std::to_string(txErrors);

    message +=
        " RX_DROPS=" +
        std::to_string(rxDrops);

    message +=
        " TX_DROPS=" +
        std::to_string(txDrops);

    return message;
}