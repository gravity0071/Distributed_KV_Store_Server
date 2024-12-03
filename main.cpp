#include "handleClient/clientMainFunction.h"
#include "handleLocalCommand/localCommandMainFunction.h"
#include "util/KVMap.h"
#include "util/JsonParser.h"
#include <thread>
#include <iostream>
#include <atomic>
#include <string>
#include <csignal>

// Global running flag
std::atomic<bool> isRunning(true);

// Signal handler for graceful shutdown
void handleSignal(int signal) {
    if (signal == SIGINT) {
        std::cout << "\nReceived SIGINT. Shutting down...\n";
        isRunning = false;
    }
}

int main(int argc, char* argv[]) {
    // Attach signal handler
    std::signal(SIGINT, handleSignal);

    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <client_port> <command_port> <store_id>\n";
        return 1;
    }

    // Parse command-line arguments
    int clientPort = std::stoi(argv[1]);
    int commandPort = std::stoi(argv[2]);
    std::string storeId = argv[3];

//    std::cout << "Starting server with ports:\n"
//              << "Client Port: " << clientPort << "\n"
//              << "Command Port: " << commandPort << "\n"
//              << "Store ID: " << storeId << "\n";

    // Initialize components
    KVMap kvmap;
    JsonParser jsonParser;
    bool isMigrating = false;

    try {
        // Create thread objects
        ClientThread clientThread(kvmap, clientPort, isMigrating, isRunning, jsonParser);
        CommandThread commandThread(kvmap, commandPort, isMigrating, isRunning, jsonParser);

        // Launch threads
        std::thread client(&ClientThread::run, &clientThread);
        std::thread command(&CommandThread::run, &commandThread);

        // Wait for threads to complete
        client.join();
        command.join();
    } catch (const std::exception& e) {
        std::cerr << "Error occurred: " << e.what() << "\n";
        isRunning = false;
    }

    std::cout << "Server shutting down...\n";
    return 0;
}
