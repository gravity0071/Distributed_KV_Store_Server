#include "clientMainFunction.h"
#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <map>
#include <cstring>
#include <unistd.h> // For close()

ClientThread::ClientThread(KVMap& kvMap, int port, bool& isMigrating, std::atomic<bool>& isRunning, JsonParser& jsonParser)
        : kvMap(kvMap), port(port), isMigrating(isMigrating), isRunning(isRunning), jsonParser(jsonParser), commandSocket(-1) {
//    kvMap.put("1000", "A");
//    kvMap.put("2000", "B");
//    kvMap.put("3000", "C");
//    kvMap.put("4000", "100");
}

// Destructor
ClientThread::~ClientThread() {
    if (commandSocket != -1) {
        close(commandSocket);
        std::cout << "ClientThread: Command connection closed.\n";
    }
}

bool ClientThread::connectToClient(Server &server) {
    commandSocket = server.acceptConnection();
    if (commandSocket < 0) {
        std::cerr << "ClientThread: Failed to accept command client connection.\n";
        return false;
    }
    return true;
}

void ClientThread::processCommand(const std::string &commandJson, int clientSocket) {
    auto command = jsonParser.JsonToMap(commandJson);

    if (command.find("operation") == command.end() || command.find("key") == command.end()) {
        std::string errorResponse = jsonParser.MapToJson({{"error", "Invalid command format. Missing 'operation' or 'key'."}});
        send(clientSocket, errorResponse.c_str(), errorResponse.size(), 0);
        return;
    }

    std::string operation = command["operation"];
    std::string key = command["key"];

    if (operation == "read") {
        std::string value;
        if (kvMap.get(key, value)) {
            std::string successResponse = jsonParser.MapToJson({{"key", key}, {"value", value}});
            send(clientSocket, successResponse.c_str(), successResponse.size(), 0);
        } else {
            std::string errorResponse = jsonParser.MapToJson({{"error", "Key not found."}});
            send(clientSocket, errorResponse.c_str(), errorResponse.size(), 0);
        }
    } else if (operation == "write" && !isMigrating) {
        if (command.find("value") != command.end()) {
            std::string value = command["value"];
            kvMap.write(key, value);
            std::string successResponse = jsonParser.MapToJson({{"message", "Write operation succeeded."}});
            send(clientSocket, successResponse.c_str(), successResponse.size(), 0);
        } else {
            std::string errorResponse = jsonParser.MapToJson({{"error", "Write operation failed. Missing 'value'."}});
            send(clientSocket, errorResponse.c_str(), errorResponse.size(), 0);
        }
    } else if (operation == "delete" && !isMigrating) {
        if (kvMap.remove(key)) {
            std::string successResponse = jsonParser.MapToJson({{"message", "Delete operation succeeded."}});
            send(clientSocket, successResponse.c_str(), successResponse.size(), 0);
        } else {
            std::string errorResponse = jsonParser.MapToJson({{"error", "Key not found. Delete operation failed."}});
            send(clientSocket, errorResponse.c_str(), errorResponse.size(), 0);
        }
    } else {
        std::string errorResponse = jsonParser.MapToJson({{"error", "Key not found."}});
        send(clientSocket, errorResponse.c_str(), errorResponse.size(), 0);
    }
}

void ClientThread::run() {
    Server server(port);

    if (!server.initialize()) {
        std::cerr << "ClientThread: Server initialization failed.\n";
        return;
    }

    std::vector<std::thread> client_threads;

    while (isRunning) {
        int clientSocket = server.acceptConnection();

        if (clientSocket < 0) {
            std::cerr << "ClientThread: Failed to accept client connection.\n";
            continue;
        }

        // Spawn a new thread to handle the client
        client_threads.emplace_back([this, clientSocket]() {
            this->processClient(clientSocket);
        });
    }

    // Join all client threads before shutting down
    for (auto &t : client_threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    if (commandSocket != -1) {
        close(commandSocket);
    }

    server.closeServer();
    std::cout << "ClientThread: Stopped.\n";
}

void ClientThread::processClient(int clientSocket) {
    char buffer[1024]; // Buffer to hold received data

    while (true) {
        memset(buffer, 0, sizeof(buffer)); // Clear the buffer for each new message

        int bytes_read = recv(clientSocket, buffer, sizeof(buffer) - 1, 0); // Leave space for null termination

        if (bytes_read < 0) {
            perror("Error reading from socket");
            break;
        } else if (bytes_read == 0) {
            std::cout << "Client disconnected.\n";
            break;
        }

        buffer[bytes_read] = '\0'; // Null-terminate the buffer to safely print as a string

        // Process the command
        processCommand(buffer, clientSocket);
    }

    close(clientSocket);
    std::cout << "Stopped handling client.\n";
}
