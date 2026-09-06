#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include <string>

class TcpClient
{
public:

    TcpClient();

    ~TcpClient();

    bool connectToServer(
        const std::string& serverIP,
        int serverPort,
        const std::string& agentId
    );

    bool sendMessage(
        const std::string& message
    );

    void disconnect();

    bool isConnected() const;

private:

    int socketFd;

    bool connected;
};

#endif