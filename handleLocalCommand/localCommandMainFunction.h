#pragma once

#include "../util/KVMap.h"
#include "../util/JsonParser.h"
#include "../util/TcpConnectionUtility.h"
#include <atomic>
#include <string>


class CommandThread {
private:              // Reference to the key-value store
    KVMap &kvMap;
    int port;                          // Command port
    bool &isMigrating;
    std::atomic<bool> &isRunning;      // Shared flag for graceful shutdown
    std::string storeId;
    int commandSocket;                 // Persistent socket for client connection
    JsonParser &jsonParser;
    TcpConnectionUtility &tcpConnectionUtility;

    int distinguishSendorRec(int clientSocket); //first step, seperate send and recv side
    int sendOperation(const std::map<std::string, std::string> &firstOpRec, int clientSocket);

    int recvOperation(const std::map<std::string, std::string> &firstOpRec, int clientSocket);

    int receiveChunckdata(int dataRecvSocket, std::string &keyRange);

    int sendChunkData(int dataRecvSocket, std::string &keyRange);

    int receiveData(int clientSocket, std::string &receivedData);

    int deleteKey(std::map<std::string, std::string> lastOperationFromMasterMap);


public:
    // Constructor
    CommandThread(KVMap &kvMap, int port, bool &isMigrating, std::atomic<bool> &isRunning, std::string storeId,
                  JsonParser &jsonParser);

    // Destructor
    ~CommandThread();

    // Run the thread
    void run();
};