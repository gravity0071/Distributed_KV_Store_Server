#pragma once

#include "../util/KVMap.h"
#include "../util/JsonParser.h"
#include <atomic>
#include <string>

class CommandThread {
private:              // Reference to the key-value store
    KVMap& kvMap;
    int port;                          // Command port
    bool& isMigrating;
    std::atomic<bool>& isRunning;      // Shared flag for graceful shutdown
    int commandSocket;                 // Persistent socket for client connection
    JsonParser& jsonParser;

    // Establish a connection with the command client
    bool connectToClient();

    // Process commands received from the client
    void processCommands();

public:
    // Constructor
    CommandThread(KVMap& kvMap, int port, bool& isMigrating, std::atomic<bool>& isRunning, JsonParser& jsonParser);

    // Destructor
    ~CommandThread();

    // Run the thread
    void run();
};