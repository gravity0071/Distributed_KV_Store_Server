#include "heartbeatMainFunction.h"
#include <iostream>
#include <cstring>
#include <unistd.h> // For close()
#include <arpa/inet.h> // For socket functions
#include <thread> // For std::this_thread::sleep_for
#include <chrono> // For std::chrono::seconds

// Constructor
HeartbeatThread::HeartbeatThread(KVMap& kvMap, const std::string& masterIp, int masterPort, std::atomic<bool>& isRunning, std::string& storeId, JsonParser& jsonParser)
        : kvMap(kvMap), masterIp(masterIp), masterPort(masterPort), isRunning(isRunning), storeId(storeId), jsonParser(jsonParser), masterSocket(-1), tcpConnectionUtility(tcpConnectionUtility) {}

// Destructor
HeartbeatThread::~HeartbeatThread() {
    if (masterSocket != -1) {
        close(masterSocket); // Close the master socket if open
        std::cout << "HeartbeatThread: Connection to master closed.\n";
    }
    stopHeartbeat();
}

// Function to send heartbeat messages to the master
void HeartbeatThread::sendHeartbeat() {
    std::map<std::string, std::string> heartbeatData;
    heartbeatData["operation"] = "heartbeat";
    heartbeatData["storeId"] = storeId;

    std::string heartbeatMessage = jsonParser.MapToJson(heartbeatData);

    if (send(masterSocket, heartbeatMessage.c_str(), heartbeatMessage.size(), 0) < 0) {
        perror("Failed to send heartbeat");
        if (!connectToMaster()) {
            std::cerr << "Reconnection to master failed. Retrying...\n";
        }
    } else {
//        std::cout << "Heartbeat sent to master.\n";
//        std::cout << isRunning << std::endl;
    }
}

// Function to establish a connection to the master
bool HeartbeatThread::connectToMaster() {
    masterSocket = tcpConnectionUtility.connectToServer(masterIp, masterPort);
    if (masterSocket == -1) {
        std::cerr << "Failed to connect to master at " << masterIp << ":" << masterPort << std::endl;
        return false;
    }

    std::cout << "Successfully connected to master." << std::endl;
    return true;
}

// Run the thread
void HeartbeatThread::run() {
    std::this_thread::sleep_for(std::chrono::seconds(1)); // wait the master for the heartbeat thread finishing creating
    if (!connectToMaster()) {
        std::cerr << "Initial connection to master failed. Exiting heartbeat thread.\n";
        return;
    }

    while (isRunning) {
        std::this_thread::sleep_for(std::chrono::seconds(5)); // Heartbeat interval
        sendHeartbeat();
    }

    // Close the connection when shutting down
    if (masterSocket != -1) {
        close(masterSocket);
        masterSocket = -1;
    }

    std::cout << "Heartbeat thread stopped.\n";
}

// Stop the thread
void HeartbeatThread::stopHeartbeat() {
    isRunning = false; // Stop the thread loop
}