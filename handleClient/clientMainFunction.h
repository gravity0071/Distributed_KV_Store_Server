#pragma once

#include "../util/KVMap.h"
#include "../util/JsonParser.h"
#include "../util/Server.h"
#include <atomic>
#include <string>

class ClientThread {
private:
    KVMap& kvMap;                 // Shared key-value store
    int port;                     // Port to listen on
    std::atomic<bool>& isRunning; // Shared shutdown flag
    bool& isMigrating;            // Migration flag (blocks writes)
    JsonParser& jsonParser;       // JSON parser utility
    int commandSocket;            // Active client socket for commands

    // Function to handle communication with a single client
    void handleClient(int clientSocket);

    // Handles incoming commands from clients
    void processCommand(const std::string &commandJson, int clientSocket);

    void processClient(int clientSocket);


        // Establish a connection with the command client
    bool connectToClient(Server &server);

public:
    // Constructor
    ClientThread(KVMap& kvMap, int port, bool& isMigrating, std::atomic<bool>& isRunning, JsonParser& jsonParser);

    // Destructor
    ~ClientThread();

    // Run the thread
    void run();
};
