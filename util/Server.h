//
// Created by Shawn Wan on 2024/11/5.
//
// Server.h
#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>
#include <string>

#define PORT 8080
#define BUFFER_SIZE 1024

class Server {
public:
    Server(int port = PORT);      // Constructor to initialize the server with a specific port
    ~Server();                    // Destructor to clean up resources
    void start();                 // Start the server to listen for connections

private:
    int server_fd;                // File descriptor for the server socket
    struct sockaddr_in address;   // Address structure
    int port;                     // Port to listen on

    void handleClient(int client_socket, const std::string& client_ip, int client_port); //will close the connection when type in "close"
};

#endif // SERVER_H