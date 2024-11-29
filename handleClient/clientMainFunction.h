// ClientThread.h
// Created by Shawn Wan on 2024/11/14
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

    // Function to handle communication with a single client
    void handleClient(int clientSocket);

    /**
     * Handles "delete" operation for removing a key-value pair.
     * If the key exists, it removes the entry and sends a success response.
     * Otherwise, sends an error response to the client.
     */

    /**
     * Handles "increment" operation for incrementing the value associated with a key.
     * If the key exists and the value is an integer, increments it and updates the store.
     * Otherwise, sends an error response to the client.
     */

public:
    // Constructor
    ClientThread(KVMap& kvMap, int port, bool& isMigrating, std::atomic<bool>& isRunning, JsonParser& jsonParser);

    // Run the thread
    void run();
};
