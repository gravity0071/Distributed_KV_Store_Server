#include "TcpConnectionUtility.h"
#include <iostream>
#include <cstring>
#include <arpa/inet.h> // For socket functions
#include <unistd.h>    // For close()

int TcpConnectionUtility::connectToServer(const std::string& serverIp, int serverPort, int localPort) {
    // Create a new socket
    int socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // Bind to a specific local port, if specified
    if (localPort > 0) {
        sockaddr_in localAddress{};
        localAddress.sin_family = AF_INET;
        localAddress.sin_addr.s_addr = INADDR_ANY; // Bind to all local interfaces
        localAddress.sin_port = htons(localPort);

        if (bind(socketFd, (struct sockaddr*)&localAddress, sizeof(localAddress)) < 0) {
            perror("Binding to local port failed");
            close(socketFd);
            return -1;
        }

        std::cout << "Bound to local port " << localPort << std::endl;
    }

    // Set up the remote server address
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);

    // Convert server IP to binary form
    if (inet_pton(AF_INET, serverIp.c_str(), &serverAddress.sin_addr) <= 0) {
        perror("Invalid server IP address");
        close(socketFd);
        return -1;
    }

    // Connect to the server
    if (connect(socketFd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Connection to server failed");
        close(socketFd);
        return -1;
    }

    std::cout << "Connected to server at " << serverIp << ":" << serverPort << std::endl;
    return socketFd;
}