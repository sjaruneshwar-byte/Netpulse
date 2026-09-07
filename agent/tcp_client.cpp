#include "tcp_client.h"

#include <iostream>
#include <string>
#include <cstring>

#include <unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include "../common/protocol.h"


TcpClient::TcpClient()
    : socketFd(-1),
      connected(false)
{
}


TcpClient::~TcpClient()
{
    disconnect();
}


bool TcpClient::connectToServer(
    const std::string& serverIP,
    int serverPort,
    const std::string& agentId)
{
    std::lock_guard<std::mutex> lock(socketMutex);

    if (connected)
    {
        return true;
    }

    socketFd = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (socketFd < 0)
    {
        std::cerr
            << "Error: Could not create TCP socket.\n";

        return false;
    }

    sockaddr_in serverAddress{};

    serverAddress.sin_family = AF_INET;

    serverAddress.sin_port =
        htons(serverPort);

    if (inet_pton(
            AF_INET,
            serverIP.c_str(),
            &serverAddress.sin_addr) <= 0)
    {
        std::cerr
            << "Error: Invalid server IP address.\n";

        close(socketFd);
        socketFd = -1;

        return false;
    }

    std::cout
        << "\nConnecting to server "
        << serverIP
        << ":"
        << serverPort
        << "...\n";

    if (connect(
            socketFd,
            reinterpret_cast<sockaddr*>(
                &serverAddress),
            sizeof(serverAddress)) < 0)
    {
        std::cerr
            << "Error: Could not connect to server.\n";

        close(socketFd);
        socketFd = -1;

        return false;
    }

    connected = true;

    std::cout
        << "TCP connection established!\n";

    std::string hello =
        "HELLO " + agentId;

    if (!sendFramedMessage(
            socketFd,
            hello))
    {
        close(socketFd);
        socketFd = -1;
        connected = false;

        return false;
    }

    std::cout
        << "Sent: "
        << hello
        << "\n";

    std::string response;

    if (!receiveFramedMessage(
            socketFd,
            response))
    {
        std::cerr
            << "Error: Failed to receive server response.\n";

        close(socketFd);
        socketFd = -1;
        connected = false;

        return false;
    }

    std::cout
        << "Received: "
        << response
        << "\n";

    return true;
}


bool TcpClient::sendMessage(
    const std::string& message)
{
    std::lock_guard<std::mutex> lock(socketMutex);

    if (!connected || socketFd < 0)
    {
        return false;
    }

    return sendFramedMessage(
        socketFd,
        message
    );
}


void TcpClient::disconnect()
{
    std::lock_guard<std::mutex> lock(socketMutex);

    if (socketFd >= 0)
    {
        close(socketFd);
        socketFd = -1;
    }

    connected = false;
}


bool TcpClient::isConnected() const
{
    std::lock_guard<std::mutex> lock(socketMutex);

    return connected && socketFd >= 0;
}