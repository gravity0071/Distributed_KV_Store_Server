#include "handleClient/clientMainFunction.h"
#include "handleHeartbeat/heartbeatMainFunction.h"
#include "handleLocalCommand/localCommandMainFunction.h"
#include "util/KVMap.h"
#include "util/JsonParser.h"
#include <thread>
#include <iostream>
#include <atomic>
#include <string>

#define masterIp "127.0.0.1"
#define heartbeatMasterPort 8081

int main(int argc, char* argv[]) {
    // Check for correct number of arguments
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <client_port> <heartbeat_port> <command_port> <store_id>\n";
        return 1; // Exit with error code
    }

    // Parse command-line arguments
    int clientPort = std::stoi(argv[1]);
    int heartbeatPort = std::stoi(argv[2]);
    int commandPort = std::stoi(argv[3]);
    std::string storeId = argv[4];

    // Output parsed ports
    std::cout << "Starting server with ports:\n"
              << "Client Port: " << clientPort << "\n"
              << "Heartbeat Port: " << heartbeatPort << "\n"
              << "Command Port: " << commandPort << "\n"
              << "store Id: " << storeId << "\n";

    // Initialize components
    KVMap kvmap;
    JsonParser jsonParser;
    bool isMigrating = false; //if is migrating, block all the write function
    std::atomic<bool> isRunning(true); // Shared shutdown flag

    // Create thread objects with respective components and ports
    ClientThread clientThread(kvmap, clientPort, isMigrating, isRunning, jsonParser);
    HeartbeatThread heartbeatThread(kvmap, masterIp, heartbeatMasterPort, isRunning, storeId, jsonParser);
    CommandThread commandThread(kvmap, commandPort, isMigrating, isRunning, jsonParser);

    // Launch threads
    std::thread client(&ClientThread::run, &clientThread);
    std::thread heartbeat(&HeartbeatThread::run, &heartbeatThread);
    std::thread command(&CommandThread::run, &commandThread);

    // Wait for threads to complete
    client.join();
    heartbeat.join();
    command.join();

    std::cout << "Server shutting down...\n";
    return 0;
}