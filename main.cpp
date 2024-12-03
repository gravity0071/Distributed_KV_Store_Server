#include "handleClient/clientMainFunction.h"
#include "handleLocalCommand/localCommandMainFunction.h"
#include "handleHeartbeat/heartbeatMainFunction.h"
#include "util/TcpConnectionUtility.h"
#include "util/KVMap.h"
#include "util/JsonParser.h"
#include <thread>
#include <atomic>
#include <iostream>
#include <set>
#include <string>
#include <cstdlib>
#include <cstdio>

// Get used ports via lsof
std::set<int> getUsedPorts() {
    std::set<int> usedPorts;
    FILE* pipe = popen("lsof -i -nP | grep -E 'TCP|UDP'", "r");
    if (!pipe) {
        perror("popen failed");
        return usedPorts;
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::string line(buffer);
        size_t colonPos = line.rfind(':'); // Find the last ':'
        if (colonPos != std::string::npos) {
            size_t spacePos = line.find(' ', colonPos); // Find the next space after the port
            if (spacePos != std::string::npos) {
                try {
                    int port = std::stoi(line.substr(colonPos + 1, spacePos - colonPos - 1));
                    usedPorts.insert(port);
                } catch (const std::exception& e) {
//                    std::cerr << "Error parsing port: " << e.what() << std::endl;
                }
            }
        }
    }

    pclose(pipe);
    return usedPorts;
}

// Find two available ports
std::pair<int, int> findAvailablePorts() {
    const int minPort = 1024;  // Start of user port range
    const int maxPort = 65535; // End of port range
    auto usedPorts = getUsedPorts();

    int firstPort = -1, secondPort = -1;
    for (int port = minPort; port <= maxPort; ++port) {
        if (usedPorts.find(port) == usedPorts.end()) {
            if (firstPort == -1) {
                firstPort = port;
            } else {
                secondPort = port;
                break;
            }
        }
    }

    return {firstPort, secondPort};
}

int sendPortsToMaster(int clientPort, int commandPort, std::string masterIp, int masterTmpPort, std::string storeId){
    std::map<std::string, std::string> sendPortsToMaster;
    sendPortsToMaster["clientPort"] = std::to_string(clientPort);
    sendPortsToMaster["commandPort"] = std::to_string(commandPort);
    sendPortsToMaster["storeId"] = storeId;
    JsonParser jsonParser;
    std::string sendPortstoMasterString = jsonParser.MapToJson(sendPortsToMaster);

//    std::this_thread::sleep_for(std::chrono::seconds(2));
    TcpConnectionUtility tcpConnectionUtility;
    int socket = tcpConnectionUtility.connectToServer(masterIp, masterTmpPort);
    if(socket < 0){
        std::cerr << "Main: connect to server failed" << std::endl;
        return 0;
    }

    if(!send(socket, sendPortstoMasterString.c_str(), sendPortstoMasterString.size(), 0)){
        std::cerr << "Main: send address failed.\n";
        return 0;
    }
    return 1;
}

int main(int argc, char* argv[]) {
    // Check for correct number of arguments
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <store_id> <master_ip> <master_heartbeat_port> <master_tmp_port>\n";
        return 1; // Exit with error code
    }

    // Randomize seed for port selection
    std::srand(std::time(nullptr));
    auto [port1, port2] = findAvailablePorts();

    int clientPort = port1;
    int commandPort = port2;
    std::string masterIp = argv[2];
    std::string storeId = argv[1];
    int masterHeartBeatPort = std::stoi(argv[3]);
    int masterTmpPort = std::stoi(argv[4]);

    if(!sendPortsToMaster(clientPort, commandPort, masterIp, masterTmpPort, storeId)){
        std::cerr << "Main: cannot send two ports to server.\n";
        return 0;
    }

//    std::cout << "Starting server with ports:\n"
//              << "Client Port: " << clientPort << "\n"
//              << "Command Port: " << commandPort << "\n"
//              << "Store ID: " << storeId << "\n";

    // Initialize components
    KVMap kvmap;
    JsonParser jsonParser;
    bool isMigrating = false; // If migrating, block write operations
    std::atomic<bool> isRunning(true); // Shared shutdown flag

    // Create threads for client, heartbeat, and commands
    ClientThread clientThread(kvmap, clientPort, isMigrating, isRunning, jsonParser);
    HeartbeatThread heartbeatThread(kvmap, masterIp, masterHeartBeatPort, isRunning, storeId, jsonParser);
    CommandThread commandThread(kvmap, commandPort, clientPort, isMigrating, isRunning, storeId, jsonParser);

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