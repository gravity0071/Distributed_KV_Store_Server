#include "localCommandMainFunction.h"
#include "../util/Server.h"
#include <iostream>
#include <cstring>
#include <unistd.h> // For close()
//initiate brach
// Constructor
CommandThread::CommandThread(KVMap &kvMap, int port, bool &isMigrating, std::atomic<bool> &isRunning,
                             JsonParser &jsonParser)
        : kvMap(kvMap), port(port), isMigrating(isMigrating), isRunning(isRunning), jsonParser(jsonParser),
          commandSocket(-1) {}

// Destructor
CommandThread::~CommandThread() {
    if (commandSocket != -1) {
        close(commandSocket);
        std::cout << "CommandThread: Connection closed.\n";
    }
}

// Establish a connection with the command client
bool CommandThread::connectToClient() {
    Server server(port);

    // Initialize the server
    if (!server.initialize()) {
        std::cerr << "CommandThread: Server initialization failed.\n";
        return false;
    }

    // Accept a single client connection
    commandSocket = server.acceptConnection();
    if (commandSocket < 0) {
        std::cerr << "CommandThread: Failed to accept client connection.\n";
        return false;
    }

    std::cout << "CommandThread: Accepted connection on port " << port << "\n";
    return true;
}

// Process commands received from the client
void CommandThread::processCommands() {
    char buffer[1024] = {0};

    while (isRunning) {
        // Receive a command from the client
        int bytesRead = recv(commandSocket, buffer, sizeof(buffer) - 1, 0); // Leave space for null termination
        if (bytesRead <= 0) {
            if (bytesRead == 0) {
                std::cout << "CommandThread: Client disconnected.\n";
            } else {
                std::cerr << "CommandThread: Error reading from client.\n";
            }
            close(commandSocket); // Clean up socket
            break;
        }

        buffer[bytesRead] = '\0'; // Null-terminate the received data
        std::cout << "CommandThread: Received command: " << buffer << "\n";

        // Process command
        if (std::string(buffer) == "close") {
            std::cout << "CommandThread: Shutting down...\n";
            isRunning = false; // Signal to stop the server
            break;
        }

        memset(buffer, 0, sizeof(buffer)); // Clear the buffer for the next command
    }
    close(commandSocket); // Ensure the socket is closed on exit
}

// Run the thread
void CommandThread::run() {
    if (!connectToClient()) {
        std::cerr << "CommandThread: Failed to establish connection. Exiting thread.\n";
        return;
    }

    processCommands();

    // Clean up the socket
    if (commandSocket != -1) {
        close(commandSocket);
        commandSocket = -1;
    }

    std::cout << "CommandThread: Stopped.\n";
}