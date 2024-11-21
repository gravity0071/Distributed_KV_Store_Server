#include "handleClient/clientMainFunction.h"
#include "handleHeartbeat/heartbeatMainFunction.h"
#include "handleLocalCommand/localCommandMainFunction.h"
#include "util/KVMap.h"
#include "util/JsonParser.h"
#include <thread>
#include <iostream>
#include <atomic>
#include <string>

int main(int argc, char* argv[]) {
    // Check for correct number of arguments
    if (argc != 6) {
        std::cerr << "Usage: " << argv[0] << " <client_port> <command_port> <store_id>\n";
        return 1; // Exit with error code
    }

    // Parse command-line arguments
    int clientPort = std::stoi(argv[1]);
    int commandPort = std::stoi(argv[2]);
    std::string storeId = argv[3];
    std::string masterIp = argv[4];
    int masterHeartBeatPort = std::stoi(argv[5]);

    // Output parsed ports
    std::cout << "Starting server with ports:\n"
              << "Client Port: " << clientPort << "\n"
              << "Command Port: " << commandPort << "\n"
              << "store Id: " << storeId << "\n";

    // Initialize components
    KVMap kvmap;
    JsonParser jsonParser;
    bool isMigrating = false; //if is migrating, block all the write function
    std::atomic<bool> isRunning(true); // Shared shutdown flag

    // Create thread objects with respective components and ports
    ClientThread clientThread(kvmap, clientPort, isMigrating, isRunning, jsonParser);
    HeartbeatThread heartbeatThread(kvmap, masterIp, masterHeartBeatPort, isRunning, storeId, jsonParser);
    CommandThread commandThread(kvmap, commandPort, isMigrating, isRunning, storeId, jsonParser);

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