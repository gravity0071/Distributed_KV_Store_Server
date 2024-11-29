#include "localCommandMainFunction.h"
#include "../util/Server.h"
#include <iostream>
#include <cstring>
#include <unistd.h> // For close()

// Constructor
CommandThread::CommandThread(KVMap &kvMap, int port, bool &isMigrating, std::atomic<bool> &isRunning,
                             JsonParser &jsonParser)
        : kvMap(kvMap), port(port), isMigrating(isMigrating), isRunning(isRunning), jsonParser(jsonParser),
          commandSocket(-1) {}

// Destructor
CommandThread::~CommandThread() {
    if (commandSocket != -1) {
        close(commandSocket);
        std::cout << "CommandThread: Connection closed.\n";
    }
}

// Establish a connection with the command client
bool CommandThread::connectToClient(Server &server) {
    // Accept a client connection
    commandSocket = server.acceptConnection();
    if (commandSocket < 0) {
        std::cerr << "CommandThread: Failed to accept client connection.\n";
        return false;
    }

    std::cout << "CommandThread: Accepted connection on port " << port << "\n";
    return true;
}

// Process commands received from the client
void CommandThread::processCommands() {
    char buffer[1024] = {0};

    while (isRunning) {
        // Receive a command from the client
        int bytesRead = recv(commandSocket, buffer, sizeof(buffer) - 1, 0); // Leave space for null termination
        if (bytesRead <= 0) {
            if (bytesRead == 0) {
                std::cout << "CommandThread: Client disconnected.\n";
            } else {
                std::cerr << "CommandThread: Error reading from client.\n";
            }
            close(commandSocket); // Clean up socket
            commandSocket = -1;  // Reset the socket
            break; // Exit this client's loop
        }

        buffer[bytesRead] = '\0'; // Null-terminate the received data
        std::cout << "CommandThread: Received command: " << buffer << "\n";

        // Parse the command
        auto command = jsonParser.JsonToMap(buffer);

        // Validate command structure
        if (command.find("operation") == command.end() || command.find("key") == command.end()) {
            std::string errorResponse = jsonParser.MapToJson({{"error", "Invalid command format. Missing 'operation' or 'key'."}});
            send(commandSocket, errorResponse.c_str(), errorResponse.size(), 0);
            continue; // Wait for the next command
        }

        std::string operation = command["operation"];
        std::string key = command["key"];

        if (operation == "read") {
            // Handle read operation
            std::string value;
            if (kvMap.get(key, value)) {
                std::string successResponse = jsonParser.MapToJson({{"key", key}, {"value", value}});
                send(commandSocket, successResponse.c_str(), successResponse.size(), 0);
            } else {
                std::string errorResponse = jsonParser.MapToJson({{"error", "Key not found."}});
                send(commandSocket, errorResponse.c_str(), errorResponse.size(), 0);
            }
        } else if (operation == "write") {
            // Handle write operation
            if (command.find("value") != command.end()) {
                std::string value = command["value"];
                kvMap.put(key, value);
                std::string successResponse = jsonParser.MapToJson({{"message", "Write operation succeeded."}});
                send(commandSocket, successResponse.c_str(), successResponse.size(), 0);
            } else {
                std::string errorResponse = jsonParser.MapToJson({{"error", "Write operation failed. Missing 'value'."}});
                send(commandSocket, errorResponse.c_str(), errorResponse.size(), 0);
            }
        } else if (operation == "delete") {
            // Handle delete operation
            if (kvMap.remove(key)) {
                std::string successResponse = jsonParser.MapToJson({{"message", "Delete operation succeeded."}});
                send(commandSocket, successResponse.c_str(), successResponse.size(), 0);
            } else {
                std::string errorResponse = jsonParser.MapToJson({{"error", "Key not found. Delete operation failed."}});
                send(commandSocket, errorResponse.c_str(), errorResponse.size(), 0);
            }
        } else if (operation == "increment") {
            // Handle increment operation
            if (kvMap.increment(key)) {
                std::string newValue;
                kvMap.get(key, newValue);
                std::string successResponse = jsonParser.MapToJson({{"key", key}, {"value", newValue}});
                send(commandSocket, successResponse.c_str(), successResponse.size(), 0);
            } else {
                std::string errorResponse = jsonParser.MapToJson({{"error", "Increment operation failed. Key not found or value is not an integer."}});
                send(commandSocket, errorResponse.c_str(), errorResponse.size(), 0);
            }
        } else if (operation == "close") {
            // Handle close operation
            std::cout << "CommandThread: Shutting down...\n";
            isRunning = false; // Signal to stop the server
            break;
        } else {
            // Invalid operation
            std::string errorResponse = jsonParser.MapToJson({{"error", "Invalid operation. Supported: 'read', 'write', 'delete', 'increment', 'close'."}});
            send(commandSocket, errorResponse.c_str(), errorResponse.size(), 0);
        }

        memset(buffer, 0, sizeof(buffer)); // Clear the buffer for the next command
    }
}

// Run the thread
void CommandThread::run() {
    Server server(port);

    if (!server.initialize()) {
        std::cerr << "CommandThread: Server initialization failed.\n";
        return;
    }

    while (isRunning) {
        if (commandSocket == -1) { // Accept a new connection if no active connection
            if (!connectToClient(server)) {
                continue; // Retry accepting connections
            }
        }

        processCommands(); // Process commands for the connected client
    }

    // Clean up the socket
    if (commandSocket != -1) {
        close(commandSocket);
        commandSocket = -1;
    }

    server.closeServer();
    std::cout << "CommandThread: Stopped.\n";
}
