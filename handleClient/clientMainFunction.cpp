#include "clientMainFunction.h"
#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <map>
#include <cstring>
#include <unistd.h> // For close()

// Constructor
ClientThread::ClientThread(KVMap& kvMap, int port, bool& isMigrating, std::atomic<bool>& isRunning, JsonParser& jsonParser)
        : kvMap(kvMap), port(port), isMigrating(isMigrating), isRunning(isRunning), jsonParser(jsonParser) {
    kvMap.put("1000", "A");
    kvMap.put("2000", "B");
    kvMap.put("3000", "C");
    kvMap.put("4000", "100");
}

// 处理单个客户端请求
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

    buffer[bytesRead] = '\0';
    std::cout << "Received request from client: " << buffer << std::endl;

    // Parse the client request
    auto clientRequest = jsonParser.JsonToMap(buffer);

    // Check for required fields
    if (clientRequest.find("operation") == clientRequest.end() || clientRequest.find("key") == clientRequest.end()) {
        std::string errorMessage = jsonParser.MapToJson({{"error", "Invalid request format. Missing 'operation' or 'key'."}});
        send(clientSocket, errorMessage.c_str(), errorMessage.size(), 0);
        close(clientSocket);
        return;
    }

    std::string key = clientRequest["key"];
    std::string operation = clientRequest["operation"];

    // Restrict write and delete operations during migration
    if (isMigrating && (operation == "write" || operation == "delete")) {
        std::string errorMessage = jsonParser.MapToJson({{"error", "Server is migrating; only 'read' operations are allowed."}});
        send(clientSocket, errorMessage.c_str(), errorMessage.size(), 0);
        close(clientSocket);
        return;
    }

    // Handle operations
    if (operation == "write") {
        if (clientRequest.find("value") != clientRequest.end()) {
            std::string value = clientRequest["value"];
            kvMap.put(key, value);
            std::string successMessage = jsonParser.MapToJson({{"message", "Write operation succeeded."}});
            send(clientSocket, successMessage.c_str(), successMessage.size(), 0);
        } else {
            std::string errorMessage = jsonParser.MapToJson({{"error", "Write operation failed. Missing 'value'."}});
            send(clientSocket, errorMessage.c_str(), errorMessage.size(), 0);
        }
    } else if (operation == "read") {
        std::string value;
        if (kvMap.get(key, value)) {
            std::string successMessage = jsonParser.MapToJson({{"key", key}, {"value", value}});
            send(clientSocket, successMessage.c_str(), successMessage.size(), 0);
        } else {
            std::string errorMessage = jsonParser.MapToJson({{"error", "Key not found."}});
            send(clientSocket, errorMessage.c_str(), errorMessage.size(), 0);
        }
    } else if (operation == "delete") {
        if (kvMap.remove(key)) {
            std::string successMessage = jsonParser.MapToJson({{"message", "Delete operation succeeded."}});
            send(clientSocket, successMessage.c_str(), successMessage.size(), 0);
        } else {
            std::string errorMessage = jsonParser.MapToJson({{"error", "Key not found. Delete operation failed."}});
            send(clientSocket, errorMessage.c_str(), errorMessage.size(), 0);
        }
    } else {
        std::string errorMessage = jsonParser.MapToJson({{"error", "Invalid operation. Supported operations: 'read', 'write', 'delete'."}});
        send(clientSocket, errorMessage.c_str(), errorMessage.size(), 0);
    }

    close(clientSocket);
    std::cout << "Closed connection with client.\n";
}


// Run the thread
void ClientThread::run() {
    Server server(port);

    if (!server.initialize()) {
        std::cerr << "ClientThread: Server initialization failed.\n";
        return;
    }

    while (isRunning) {
        int clientSocket = server.acceptConnection();
        if (clientSocket < 0) {
            std::cerr << "Failed to accept client connection.\n";
            continue;
        }

        std::cout << "Accepted new client connection.\n";
        std::thread(&ClientThread::handleClient, this, clientSocket).detach();
    }

    server.closeServer();
    std::cout << "ClientThread: Server stopped.\n";
}
