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
    kvMap.put("1000", "A");
    kvMap.put("2000", "B");
    kvMap.put("3000", "C");
    kvMap.put("4000", "100");
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

void ClientThread::processCommands() {
    char buffer[1024] = {0};

    while (isRunning) {
        int bytesRead = recv(commandSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead <= 0) {
            if (bytesRead == 0) {
                std::cout << "ClientThread: Command client disconnected.\n";
            } else {
                std::cerr << "ClientThread: Error reading from command client.\n";
            }
            close(commandSocket);
            commandSocket = -1;
            break;
        }

        buffer[bytesRead] = '\0';

        auto command = jsonParser.JsonToMap(buffer);

        if (command.find("operation") == command.end() || command.find("key") == command.end()) {
            std::string errorResponse = jsonParser.MapToJson({{"error", "Invalid command format. Missing 'operation' or 'key'."}});
            send(commandSocket, errorResponse.c_str(), errorResponse.size(), 0);
            continue;
        }

        std::string operation = command["operation"];
        std::string key = command["key"];

        if (operation == "read") {
            std::string value;
            if (kvMap.get(key, value)) {
                std::string successResponse = jsonParser.MapToJson({{"key", key}, {"value", value}});
                send(commandSocket, successResponse.c_str(), successResponse.size(), 0);
            } else {
                std::string errorResponse = jsonParser.MapToJson({{"error", "Key not found."}});
                send(commandSocket, errorResponse.c_str(), errorResponse.size(), 0);
            }
        } else if (operation == "write") {
            if (command.find("value") != command.end()) {
                std::string value = command["value"];
                kvMap.write(key, value);
                std::string successResponse = jsonParser.MapToJson({{"message", "Write operation succeeded."}});
                send(commandSocket, successResponse.c_str(), successResponse.size(), 0);
            } else {
                std::string errorResponse = jsonParser.MapToJson({{"error", "Write operation failed. Missing 'value'."}});
                send(commandSocket, errorResponse.c_str(), errorResponse.size(), 0);
            }
        } else if (operation == "delete") {
            if (kvMap.remove(key)) {
                std::string successResponse = jsonParser.MapToJson({{"message", "Delete operation succeeded."}});
                send(commandSocket, successResponse.c_str(), successResponse.size(), 0);
            } else {
                std::string errorResponse = jsonParser.MapToJson({{"error", "Key not found. Delete operation failed."}});
                send(commandSocket, errorResponse.c_str(), errorResponse.size(), 0);
            }
        } else if (operation == "close") {
            std::cout << "ClientThread: Shutting down...\n";
            isRunning = false;
            break;
        } else {
            std::string errorResponse = jsonParser.MapToJson({{"error", "Invalid operation. Supported: 'read', 'write', 'delete', 'close'."}});
            send(commandSocket, errorResponse.c_str(), errorResponse.size(), 0);
        }
        memset(buffer, 0, sizeof(buffer));
    }
}

void ClientThread::run() {
    Server server(port);

    if (!server.initialize()) {
        std::cerr << "ClientThread: Server initialization failed.\n";
        return;
    }

    while (isRunning) {
        if (commandSocket == -1) {
            if (!connectToClient(server)) {
                continue;
            }
        }
        processCommands();
    }

    if (commandSocket != -1) {
        close(commandSocket);
    }

    server.closeServer();
    std::cout << "ClientThread: Stopped.\n";
}
