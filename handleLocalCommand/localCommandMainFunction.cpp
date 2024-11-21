#include "localCommandMainFunction.h"
#include "../util/Server.h"
#include <iostream>
#include <cstring>
#include <thread>
#include <unistd.h> // For close()

//initiate brach
// Constructor
CommandThread::CommandThread(KVMap &kvMap, int port, bool &isMigrating, std::atomic<bool> &isRunning,
                             std::string storeId,
                             JsonParser &jsonParser)
        : kvMap(kvMap), port(port), isMigrating(isMigrating), isRunning(isRunning), jsonParser(jsonParser),
          storeId(storeId),
          commandSocket(-1), tcpConnectionUtility(tcpConnectionUtility) {}

// Destructor
CommandThread::~CommandThread() {
    if (commandSocket != -1) {
        close(commandSocket);
        std::cout << "CommandThread: Connection closed.\n";
    }
}

int CommandThread::distinguishSendorRec(int clientSocket) {
    std::string buffer = "";
    receiveData(clientSocket, buffer);
    std::cout << "CommandThread: Received command: " << buffer << "\n";

    std::map<std::string, std::string> firstOpRec;
    try {
        firstOpRec = jsonParser.JsonToMap(buffer);
    } catch (const std::exception &e) {
        std::cerr << "JSON parsing error: " << e.what() << "\n";
        return 0;
    }
    if (firstOpRec.find("storeId") == firstOpRec.end() || firstOpRec.at("storeId") != storeId) {
        std::cerr << "CommandThread: operation send to wrong server. \n";
        return 0;
    }
    std::string operation = "";
    if (firstOpRec.find("operation") != firstOpRec.end()) {
        operation = firstOpRec["operation"];
    } else {
        std::cerr << "operation read error" << std::endl;
        return 0;
    }

    std::map<std::string, std::string> NACK;
    NACK["ACK"] = "false";
    std::string step6NACK_json = jsonParser.MapToJson(NACK);

    if (operation == "source") {
        if(!sendOperation(firstOpRec, clientSocket)){
            if (send(clientSocket, step6NACK_json.c_str(), step6NACK_json.size(), 0) < 0) {
                std::cerr << "CommandThread: send: NACK send to master failed.\n";
                return 0;
            }
            std::cerr << "CommandThread: send: send operation failed" << std::endl;
            return 0;
        }
    } else if (operation == "recv") {
        if(!recvOperation(firstOpRec, clientSocket)){
            if (send(clientSocket, step6NACK_json.c_str(), step6NACK_json.size(), 0) < 0) {
                std::cerr << "CommandThread: recv: NACK send to master failed.\n";
                return 0;
            }
            std::cerr << "CommandThread: recv: receive operation failed" << std::endl;
            return 0;
        }
    } else {
        std::cerr << "CommandThread: operation read error" << std::endl;
        return 0;
    }

    //todo: implement the return value after the data exchange
    std::map<std::string, std::string> returnSuccessToMaster;
    returnSuccessToMaster["ACK"] = "true";
    std::string step6_json = jsonParser.MapToJson(returnSuccessToMaster);
    if (send(clientSocket, step6_json.c_str(), step6_json.size(), 0) < 0) {
        std::cerr << "CommandThread: ACK send to master failed.\n";
        return 0;
    }

    std::string lastOperationFromMaster = "";
    receiveData(clientSocket, lastOperationFromMaster);
    std::map<std::string, std::string> lastOperationFromMasterMap = jsonParser.JsonToMap(lastOperationFromMaster);
    if (lastOperationFromMasterMap.find("operation") != lastOperationFromMasterMap.end()) {
        std::string lastOperation = lastOperationFromMasterMap["operation"];
        if (lastOperation == "close") {
            isRunning = false; //close server
            send(clientSocket, step6_json.c_str(), step6_json.size(), 0); //send ACK to master
        } else if (lastOperation == "delete") {
            if (!deleteKey(lastOperationFromMasterMap))
                send(clientSocket, step6_json.c_str(), step6_json.size(), 0); //send ACK to master
            else {
                send(clientSocket, step6NACK_json.c_str(), step6NACK_json.size(), 0);
            }
        } else {
            send(clientSocket, step6_json.c_str(), step6_json.size(), 0); //send ACK to master
        }
    } else {
        std::cerr << "CommandThread: last operation receive from master failed" << std::endl;
        return 0;
    }
    return 1;
}

int CommandThread::sendOperation(const std::map<std::string, std::string> &firstOpRec, int clientSocket) {
    std::map<std::string, std::string> step2_map;

    Server server;
    if (!server.initialize()) {
        std::cerr << "CommandThread: sender receiving server initialization failed.\n";
        return 0;
    }

    step2_map["sender_Ip"] = server.getServerIP();
    step2_map["sender_Port"] = std::to_string(server.getServerPort());
    std::string step2_json = jsonParser.MapToJson(step2_map);
    if (send(clientSocket, step2_json.c_str(), step2_json.size(), 0) < 0) {
        std::cerr << "CommandThread: ip and port send to master failed.\n";
        return 0;
    }

    int datatransferSocket = server.acceptConnection();
    if (datatransferSocket < 0) {
        std::cerr << "data transfer socket initiate error. \n";
        return 0;
    }

    std::string keyRange = "";
    if (firstOpRec.find("keyRange") != firstOpRec.end()) {
        keyRange = firstOpRec.at("keyRange");
    } else {
        std::cerr << "CommandThread: recv: key range get error" << std::endl;
        return 0;
    }
    if (!sendChunkData(datatransferSocket, keyRange)) {
        std::cerr << "CommandThread: recv: data migration failed." << std::endl;
        return 0;
    }
    return 1;
}

int CommandThread::recvOperation(const std::map<std::string, std::string> &firstOpRec, int clientSocket) {
    std::string sourceIp = "";
    int sourcePort = 0;
    if (firstOpRec.find("sourcePort") != firstOpRec.end()) {
        sourcePort = std::stoi(firstOpRec.at("sourcePort"));
    } else {
        std::cerr << "CommandThread: recv: source Port read error. \n";
        return 0;
    }
    if (firstOpRec.find("sourceIp") != firstOpRec.end())
        sourceIp = firstOpRec.at("sourceIp");
    else {
        std::cerr << "CommandThread: recv: source IP read error. \n";
        return 0;
    }

    std::map<std::string, std::string> step4_map;
    step4_map["operation"] = "connect";
    step4_map["ACK"] = "true";
    step4_map["store_id"] = storeId;
    std::string step4_resp = jsonParser.MapToJson(step4_map);
    if (send(clientSocket, step4_resp.c_str(), step4_resp.size(), 0) < 0) {
        std::cerr << "CommandThread: recv: message sending to master" << std::endl;
        return 0;
    }

    int dataRecvSocket = tcpConnectionUtility.connectToServer(sourceIp, sourcePort);
    if (dataRecvSocket == -1) {
        std::cerr << "Failed to connect sender at " << sourceIp << ":" << sourcePort << std::endl;
        return 0;
    }

    std::string keyRange = "";
    if (firstOpRec.find("keyRange") != firstOpRec.end()) {
        keyRange = firstOpRec.at("keyRange");
    } else {
        std::cerr << "CommandThread: recv: key range get error" << std::endl;
        return 0;
    }

    if (!receiveChunckdata(dataRecvSocket, keyRange)) {
        std::cerr << "CommandThread: recv: data migration failed." << std::endl;
        return 0;
    }
    close(dataRecvSocket);
    return 1;
}

void CommandThread::run() {
    Server server(port);
    if (!server.initialize()) {
        std::cerr << "CommandThread: Server initialization failed." << std::endl;
        return;
    }
    std::vector<std::thread> CommandThreads;
    while (isRunning) {
        int CommandSocket = server.acceptConnection();
        if (CommandSocket < 0) {
            if (!isRunning) break; // Exit if server is shutting down
            std::cerr << "Failed to accept client connection.\n";
            continue;
        }

        CommandThreads.emplace_back(&CommandThread::distinguishSendorRec, this, CommandSocket);
        for (auto it = CommandThreads.begin(); it != CommandThreads.end();) {
            if (it->joinable()) {
                it->join();
                it = CommandThreads.erase(it);
            } else {
                ++it;
            }
        }
    }
    for (auto &t : CommandThreads) {
        if (t.joinable()) {
            t.join();
        }
    }

    server.closeServer();
    std::cout << "CommandThread: Server stopped.\n";
}

//todo: need to implement the data exchange here
int CommandThread::receiveChunckdata(int dataRecvSocket, std::string &keyRange) {
    std::cout << "starting receiving data" <<std::endl;
    return 1;
}

//todo: need to implement the data exchange here
int CommandThread::sendChunkData(int datatransferSocket, std::string &keyRange) {
    std::cout << "starting sending data" <<std::endl;

    return 1;
}

//todo: need to implement delete function
int CommandThread::deleteKey(std::map<std::string, std::string> lastOperationFromMasterMap) {
    std::cout << "deleteKey instruction: " << jsonParser.MapToJson(lastOperationFromMasterMap) << std::endl;
    return 1;
}

int CommandThread::receiveData(int clientSocket, std::string &receivedData) {
    char buffer[1024] = {0};
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead <= 0) {
        if (bytesRead == 0) {
            std::cout << "CommandThread: receiveData: TCP disconnected.\n";
        } else {
            std::cerr << "CommandThread: receiveData: Error reading from client.\n";
        }
        close(clientSocket);
        return 0;
    }

    buffer[bytesRead] = '\0'; // Null-terminate the received data
    receivedData = std::string(buffer); // Store in the output parameter
    return 1;
}