#include "tcp_client.h"
#include "../common/protocol.h"
#include <iostream>
#include <string>
#include <cstring>

#include <unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>


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
    // ------------------------------------------
    // Create TCP socket
    // ------------------------------------------

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


    // ------------------------------------------
    // Configure server address
    // ------------------------------------------

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


    // ------------------------------------------
    // Connect
    // ------------------------------------------

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


    // ------------------------------------------
    // HELLO
    // ------------------------------------------

    std::string hello =
        "HELLO " + agentId;


    if (!sendMessage(hello))
    {
        disconnect();

        return false;
    }


    std::cout
        << "Sent: "
        << hello
        << "\n";


    // ------------------------------------------
    // Receive WELCOME
    // ------------------------------------------

    char buffer[1024];

    std::memset(
        buffer,
        0,
        sizeof(buffer)
    );


    ssize_t bytesReceived =
        recv(
            socketFd,
            buffer,
            sizeof(buffer) - 1,
            0
        );


    if (bytesReceived <= 0)
    {
        std::cerr
            << "Error: Failed to receive "
               "server response.\n";

        disconnect();

        return false;
    }


    buffer[bytesReceived] = '\0';


    std::cout
        << "Received: "
        << buffer
        << "\n";


    return true;
}


bool TcpClient::sendMessage(
    const std::string& message)
{
    if (!connected)
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
    if (socketFd >= 0)
    {
        close(socketFd);

        socketFd = -1;
    }

    connected = false;
}


bool TcpClient::isConnected() const
{
    return connected;
}