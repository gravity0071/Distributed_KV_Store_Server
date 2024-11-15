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

// The Server class provides a basic TCP server implementation
// that can initialize, accept connections, and manage client connections.
class Server {
public:
    Server(int port);

    ~Server();

    bool initialize();

    int acceptConnection();

    void closeConnection(int client_socket);

    void closeServer();

private:
    int port_;
    int server_fd_;
    struct sockaddr_in address_;
    int addrlen_;

    bool configureSocket();
};

#endif // SERVER_H