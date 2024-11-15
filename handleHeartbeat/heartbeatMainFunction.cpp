#include "heartbeatMainFunction.h"
#include <iostream>
#include <cstring>
#include <unistd.h> // For close()
#include <arpa/inet.h> // For socket functions
#include <thread> // For std::this_thread::sleep_for
#include <chrono> // For std::chrono::seconds

// Constructor
HeartbeatThread::HeartbeatThread(KVMap& kvMap, const std::string& masterIp, int masterPort, std::atomic<bool>& isRunning, std::string& storeId, JsonParser& jsonParser)
        : kvMap(kvMap), masterIp(masterIp), masterPort(masterPort), isRunning(isRunning), storeId(storeId), jsonParser(jsonParser), masterSocket(-1) {}

// Destructor
HeartbeatThread::~HeartbeatThread() {
    if (masterSocket != -1) {
        close(masterSocket); // Close the master socket if open
        std::cout << "HeartbeatThread: Connection to master closed.\n";
    }
}

// Function to establish a connection to the master
bool HeartbeatThread::connectToMaster() {
    if (masterSocket != -1) {
        close(masterSocket); // Close existing socket, if any
    }

    // Create a new socket
    masterSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (masterSocket < 0) {
        perror("Socket creation failed");
        return false;
    }

    sockaddr_in masterAddress{};
    masterAddress.sin_family = AF_INET;
    masterAddress.sin_port = htons(masterPort);

    // Convert master IP to binary form
    if (inet_pton(AF_INET, masterIp.c_str(), &masterAddress.sin_addr) <= 0) {
        perror("Invalid master IP address");
        return false;
    }

    // Connect to the master
    if (connect(masterSocket, (struct sockaddr*)&masterAddress, sizeof(masterAddress)) < 0) {
        perror("Connection to master failed");
        return false;
    }

    std::cout << "Connected to master at " << masterIp << ":" << masterPort << std::endl;
    return true;
}

// Function to send heartbeat messages to the master
//todo: need to implement the heartbeat protocol
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
        std::cout << "Heartbeat sent to master.\n";
    }
}

// Run the thread
void HeartbeatThread::run() {
    if (!connectToMaster()) {
        std::cerr << "Initial connection to master failed. Exiting heartbeat thread.\n";
        return;
    }

    while (isRunning) {
        sendHeartbeat();
        std::this_thread::sleep_for(std::chrono::seconds(5)); // Heartbeat interval
    }

    // Close the connection when shutting down
    if (masterSocket != -1) {
        close(masterSocket);
        masterSocket = -1;
    }

    std::cout << "Heartbeat thread stopped.\n";
}