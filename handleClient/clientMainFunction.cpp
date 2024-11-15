// ClientThread.cpp
// Created by Shawn Wan on 2024/11/14

#include "ClientMainFunction.h"
#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <map>
#include <cstring>
#include <unistd.h> // For close()

// Constructor
ClientThread::ClientThread(KVMap& kvMap, int port, bool& isMigrating, std::atomic<bool>& isRunning, JsonParser& jsonParser)
        : kvMap(kvMap), port(port), isMigrating(isMigrating), isRunning(isRunning), jsonParser(jsonParser) {}

//todo: need to implement corresponding function
void ClientThread::handleClient(int clientSocket) {
    char buffer[1024] = {0};
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);

    if (bytesRead <= 0) {
        if (bytesRead == 0) {
            std::cout << "Client disconnected gracefully.\n";
        } else {
            std::cerr << "Error reading from client.\n";
        }
        close(clientSocket);
        return;
    }

    std::cout << "Received from client: " << buffer << std::endl;

    // Parse the client request
    auto clientLookUpMap = jsonParser.JsonToMap(buffer);

    // Check for required fields
    std::string key = "NULL";
    if (clientLookUpMap.find("operation") != clientLookUpMap.end() &&
        clientLookUpMap.find("key") != clientLookUpMap.end()) {
        key = clientLookUpMap["key"];
        std::string operation = clientLookUpMap["operation"];

        // Handle write operation
        if (operation == "write") {
            if (isMigrating) {
                std::string errorMessage = "Server is migrating; writes are temporarily blocked.";
                send(clientSocket, errorMessage.c_str(), errorMessage.size(), 0);
            } else {
                std::string value = clientLookUpMap["value"];
                kvMap.put(key, value); // Write to KV store
                std::string successMessage = "Write operation succeeded.";
                send(clientSocket, successMessage.c_str(), successMessage.size(), 0);
            }
        }
            // Handle read operation
        else if (operation == "read") {
            std::string value;
            if (kvMap.get(key, value)) {
                send(clientSocket, value.c_str(), value.size(), 0);
            } else {
                std::string errorMessage = "Key not found.";
                send(clientSocket, errorMessage.c_str(), errorMessage.size(), 0);
            }
        } else {
            std::string errorMessage = "Invalid operation.";
            send(clientSocket, errorMessage.c_str(), errorMessage.size(), 0);
        }
    } else {
        std::cerr << "Invalid request: Missing 'operation' or 'key'.\n";
        std::string errorMessage = "Invalid request: Missing 'operation' or 'key'.";
        send(clientSocket, errorMessage.c_str(), errorMessage.size(), 0);
    }

    close(clientSocket); // Close the client socket after communication
    std::cout << "Closed connection with client\n";
}

// Run the thread
void ClientThread::run() {
    Server server(port);

    // Initialize the server
    if (!server.initialize()) {
        std::cerr << "ClientThread: Server initialization failed.\n";
        return;
    }

    std::vector<std::thread> clientThreads;

    while (isRunning) {
        // Accept a new client connection
        int clientSocket = server.acceptConnection();
        if (clientSocket < 0) {
            if (!isRunning) break; // Shutdown signal received
            std::cerr << "ClientThread: Failed to accept client connection.\n";
            continue;
        }

        std::cout << "Accepted new client connection.\n";

        // Spawn a new thread to handle the client
        clientThreads.emplace_back(&ClientThread::handleClient, this, clientSocket);
    }

    // Join all client threads
    for (auto& t : clientThreads) {
        if (t.joinable()) {
            t.join();
        }
    }

    // Close the server when done
    server.closeServer();
    std::cout << "ClientThread: Server stopped.\n";
}