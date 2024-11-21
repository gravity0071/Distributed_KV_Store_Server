//
// Created by Shawn Wan on 2024/11/5.
//
#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

// The Server class provides a basic TCP server implementation
// that can initialize, accept connections, and manage client connections.
class Server {
public:
    explicit Server(int port);
    Server();

    ~Server();

    bool initialize();
    int acceptConnection();
    void closeConnection(int client_socket);
    void closeServer();
    int getSocket() const;
    std::string getServerIP() const;
    int getServerPort() const;

private:
    int port_;                 // Port number the server listens on.
    int server_fd_;            // Server socket file descriptor.
    struct sockaddr_in address_; // Structure holding server address information.
    int addrlen_;              // Length of the server address structure.

    // Configures the server socket with necessary options.
    // Returns true on success, false on failure.
    bool configureSocket();
};

#endif // SERVER_H