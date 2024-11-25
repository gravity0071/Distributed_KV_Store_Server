#pragma once

#include "../util/KVMap.h"
#include "../util/JsonParser.h"
#include "../util/TcpConnectionUtility.h"
#include <atomic>
#include <string>

class HeartbeatThread {
private:
    KVMap& kvMap;               // Reference to KV store
    std::string masterIp;              // IP of the master server
    int masterPort;                    // Port of the master server
    std::atomic<bool>& isRunning;      // Shared flag to signal shutdown
    int masterSocket;                  // Socket for the connection to the master
    std::string storeId;
    JsonParser& jsonParser;
    TcpConnectionUtility& tcpConnectionUtility;

    // Function to establish a connection to the master
    bool connectToMaster();

    // Function to send heartbeat messages to the master
    void sendHeartbeat();

public:
    // Constructor
    HeartbeatThread(KVMap& kvMap, const std::string& masterIp, int masterPort,
                    std::atomic<bool>& isRunning, std::string& storeId,
                    JsonParser& jsonParser);

    // Destructor to clean up resources
    ~HeartbeatThread();

    // Run the thread
    void run();

    // Stops the heartbeat thread
    void stopHeartbeat();
};