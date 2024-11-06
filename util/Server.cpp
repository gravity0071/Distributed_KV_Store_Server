//
// Created by Shawn Wan on 2024/11/5.
//
// Server.cpp
#include "Server.h"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

Server::Server(int port) : port(port) {
    int opt = 1;

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Set up the address structure
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Bind the socket to the port
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
}

Server::~Server() {
    close(server_fd); // Close the server socket when done
}

void Server::start() {
    // Listen for connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << port << std::endl;

    while (true) {
        int addrlen = sizeof(address);
        int client_socket;

        // Accept a client connection
        if ((client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }

        // Get client information
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &address.sin_addr, client_ip, INET_ADDRSTRLEN); // Convert IP to string
        int client_port = ntohs(address.sin_port);

        std::cout << "New connection from IP: " << client_ip << ", Port: " << client_port << std::endl;

        // Fork a new process for each client
        if (fork() == 0) {
            // In the child process, handle the client and then exit
            close(server_fd); // Child doesn't need the listening socket
            handleClient(client_socket, client_ip, client_port);
            close(client_socket);
            exit(0); // End child process after handling the client
        }

        close(client_socket); // Parent closes the client socket
    }
}

void Server::handleClient(int client_socket, const std::string& client_ip, int client_port) {
    char buffer[BUFFER_SIZE] = {0};
    const char *hello = "Hello from server";

    // Send an initial message to the client
    send(client_socket, hello, strlen(hello), 0);
    std::cout << "Initial message sent to client [" << client_ip << ":" << client_port << "]" << std::endl;

    // Keep the connection open to receive and send messages
    while (true) {
        // Clear the buffer
        memset(buffer, 0, BUFFER_SIZE);

        // Read a message from the client
        int valread = read(client_socket, buffer, BUFFER_SIZE);
        if (valread <= 0) {
            std::cout << "Client [" << client_ip << ":" << client_port << "] disconnected or error occurred" << std::endl;
            break;
        }

        // Convert `buffer` to a std::string and trim whitespace/newline characters
        std::string clientMessage(buffer);
        clientMessage.erase(clientMessage.find_last_not_of(" \n\r\t") + 1);

        // Display the client's message
        std::cout << "Message from client [" << client_ip << ":" << client_port << "]: " << clientMessage << std::endl;

        // Check if the client sent "close" to terminate the connection
        if (clientMessage == "close") {
            std::cout << "Client [" << client_ip << ":" << client_port << "] requested to close the connection." << std::endl;
            break;
        }

        // Prepare a response
        std::string response = "Server received: ";
        response += clientMessage;

        // Send the response back to the client
        send(client_socket, response.c_str(), response.length(), 0);
        std::cout << "Response sent to client [" << client_ip << ":" << client_port << "]" << std::endl;
    }

    // Close the client socket once the connection is no longer needed
    close(client_socket);
    std::cout << "Connection closed with client [" << client_ip << ":" << client_port << "]" << std::endl;
}