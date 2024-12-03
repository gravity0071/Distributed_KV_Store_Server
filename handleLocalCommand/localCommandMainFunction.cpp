#include "localCommandMainFunction.h"
#include "../util/Server.h"
#include "../util/HashCalculator.h"
#include <iostream>
#include <cstring>
#include <thread>
#include <unistd.h>
#include <openssl/md5.h>

CommandThread::CommandThread(KVMap &kvMap, int port, int clientPort, bool &isMigrating, std::atomic<bool> &isRunning,
                             std::string storeId,
                             JsonParser &jsonParser)
        : kvMap(kvMap), port(port), clientPort(clientPort), isMigrating(isMigrating), isRunning(isRunning),
          jsonParser(jsonParser),
          storeId(storeId),
          commandSocket(-1) {
    tcpConnectionUtility = *new TcpConnectionUtility();
}

// Destructor
CommandThread::~CommandThread() {
    if (commandSocket != -1) {
        close(commandSocket);
//        std::cout << "Store Server " << storeId << "CommandThread: Connection closed.\n";
    }
}

int CommandThread::distinguishSendorRec(int clientSocket) {
    isMigrating = true;

    std::string buffer = "";
    receiveData(clientSocket, buffer);
//    std::cout << "Store Server " << storeId  << "CommandThread: Received command: " << buffer << "\n";

    std::map<std::string, std::string> firstOpRec;
    try {
        firstOpRec = jsonParser.JsonToMap(buffer);
    } catch (const std::exception &e) {
        std::cerr << "JSON parsing error: " << e.what() << "\n";
        isMigrating = false;
        return 0;
    }
    if (firstOpRec.find("storeId") == firstOpRec.end() || firstOpRec.at("storeId") != storeId) {
        std::cerr << "CommandThread: operation send to wrong server. \n";
        isMigrating = false;
        return 0;
    }
    std::string operation = "";
    if (firstOpRec.find("operation") != firstOpRec.end()) {
        operation = firstOpRec["operation"];
    } else {
        std::cerr << "operation read error" << std::endl;
        isMigrating = false;
        return 0;
    }

    std::map<std::string, std::string> NACK;
    NACK["ACK"] = "false";
    std::string step6NACK_json = jsonParser.MapToJson(NACK);

    if (operation == "source") {
        if (!sendOperation(firstOpRec, clientSocket)) {
//            std::cout << "Store Server " << storeId << " :starting sending data step6NACK_json " << step6NACK_json <<std::endl;
            if (send(clientSocket, step6NACK_json.c_str(), step6NACK_json.size(), 0) < 0) {
                std::cerr << "CommandThread: send: NACK send to master failed.\n";
                isMigrating = false;
                return 0;
            }
            std::cerr << "CommandThread: send: send operation failed" << std::endl;
            isMigrating = false;
            return 0;
        }
    } else if (operation == "recv") {
        if (!recvOperation(firstOpRec, clientSocket)) {
//            std::cout << "Store Server " << storeId << " :starting sending data step6NACK_json " << step6NACK_json <<std::endl;
            if (send(clientSocket, step6NACK_json.c_str(), step6NACK_json.size(), 0) < 0) {
                std::cerr << "CommandThread: recv: NACK send to master failed.\n";
                isMigrating = false;
                return 0;
            }
            std::cerr << "CommandThread: recv: receive operation failed" << std::endl;
            isMigrating = false;
            return 0;
        }
    } else if (operation == "close") {
        isRunning = false;
        std::cout << "Store Server " << storeId << "CommandThread: system shutting down" << std::endl;
        isMigrating = false;
        return 1;
    } else {
        std::cerr << "CommandThread: operation read error" << std::endl;
        isMigrating = false;
        return 0;
    }

    std::string b = "";
    if (!receiveData(clientSocket, b)) {
        std::cerr << "Store Server: " << storeId << ": isFinish recieve error";
        isMigrating = false;
        return 0;
    }
//    std::cout << "store Server: " << storeId << ": isFinish recieved: " << b << std::endl;

    std::map<std::string, std::string> returnSuccessToMaster;
    returnSuccessToMaster["ACK"] = "true";
    std::string step6_json = jsonParser.MapToJson(returnSuccessToMaster);
    if (send(clientSocket, step6_json.c_str(), step6_json.size(), 0) < 0) {
        std::cerr << "CommandThread: ACK send to master failed.\n";
        isMigrating = false;
        return 0;
    }

    std::string lastOperationFromMaster = "";
    receiveData(clientSocket, lastOperationFromMaster);
    std::map<std::string, std::string> lastOperationFromMasterMap = jsonParser.JsonToMap(lastOperationFromMaster);
//    std::cout << "store Server: " << storeId << ": " << lastOperationFromMaster
//              << "_------------------------------------------------------------------------\n";
    if (lastOperationFromMasterMap.find("operation") != lastOperationFromMasterMap.end()) {
        std::string lastOperation = lastOperationFromMasterMap["operation"];
        if (lastOperation == "close") {
            isRunning = false; //close server
//            std::cout << "Store Server " << storeId << " :starting sending data step6_json " << step6_json <<std::endl;
            send(clientSocket, step6_json.c_str(), step6_json.size(), 0); //send ACK to master
        } else if (lastOperation == "delete") {
//            std::cout << "Store Server " << storeId << " delete operation received" << lastOperationFromMaster
//                      << std::endl;
            if (!deleteKey(lastOperationFromMasterMap))
                send(clientSocket, step6_json.c_str(), step6_json.size(), 0); //send ACK to master
            else {
//                std::cout << "Store Server " << storeId << " :starting sending data step6NACK_json " << step6NACK_json <<std::endl;
                send(clientSocket, step6NACK_json.c_str(), step6NACK_json.size(), 0);
            }
        } else {
//            std::cout << "Store Server " << storeId << " :starting sending data step6_json " << step6_json <<std::endl;
            send(clientSocket, step6_json.c_str(), step6_json.size(), 0); //send ACK to master
        }
    } else {
        std::cerr << "Store Server " << storeId << " :CommandThread: last operation receive from master failed"
                  << std::endl;
        isMigrating = false;
        return 0;
    }

    std::cout << "Store Server " << storeId << ": kvData: " << kvMap.browse() << std::endl;
    isMigrating = false;
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
//    std::cout << "Store Server " << storeId << " :starting sending data step2_json" << step2_json <<std::endl;
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
//    std::cout << "Store Server " << storeId << " :starting sending data step4_resp " << step4_resp <<std::endl;
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
    for (auto &t: CommandThreads) {
        if (t.joinable()) {
            t.join();
        }
    }

    server.closeServer();
//    std::cout << "Store Server " << storeId << "CommandThread: Server stopped.\n";
}

bool parseRange(const std::string &rangeKey, size_t &start, size_t &end) {
    std::istringstream rangeStream(rangeKey);
    char dash;
    rangeStream >> start >> dash >> end;
    return rangeStream && dash == '-';
}

int CommandThread::isInRange(size_t key, std::string keyRange) {
    size_t start, end;
    parseRange(keyRange, start, end);
    if (start == end)
        return 1;
    else if (start < end) {
        if (key >= start && key < end)
            return 1;
        else
            return 0;
    } else {
        if (key >= start || key < end)
            return 1;
        else
            return 0;
    }
    return 1;
}

int CommandThread::deleteKey(std::map<std::string, std::string> lastOperationFromMasterMap) {
    std::string keyRange = "";
    if (lastOperationFromMasterMap.find("keyRange") == lastOperationFromMasterMap.end()) {
        std::cerr << "Store Server " << storeId << ": didn't find keyRange in deleteKey function" << std::endl;
        return 0;
    } else {
        keyRange = lastOperationFromMasterMap["keyRange"];
    }

    HashCalculator hashCalculator;
    std::vector<std::string> keysToDelete;

    for (const auto &pair: kvMap) {
        if (isInRange(hashCalculator.calculateHash(pair.first), keyRange)) {
            keysToDelete.push_back(pair.first);
        }
    }

    for (const auto &key: keysToDelete) {
        kvMap.deleteKey(key);
    }

//    std::cout << "Store Server " << storeId << " :deleteKey instruction: " << keyRange << std::endl;
    return 1;
}

int CommandThread::receiveData(int clientSocket, std::string &receivedData) {
    char buffer[4096] = {0};
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead <= 0) {
        if (bytesRead == 0) {
            std::cout << "Store Server " << storeId << ": CommandThread: receiveData: TCP disconnected.\n";
        } else {
            std::cerr << "Store Server " << storeId << ":CommandThread: receiveData: Error reading from master.\n";
        }
        close(clientSocket);
        return 0;
    }

    buffer[bytesRead] = '\0'; // Null-terminate the received data
    receivedData = std::string(buffer); // Store in the output parameter
    return 1;
}

int CommandThread::sendChunkData(int datatransferSocket, std::string &keyRange) {
//    std::cout << "Store Server " << storeId << " :starting sending data" << std::endl;

    HashCalculator hashCalculator;
    std::ostringstream dataStream;

    for (const auto &pair: kvMap) {
        if (isInRange(hashCalculator.calculateHash(pair.first), keyRange)) {
//            std::cout << "Store Server " << storeId << " :data sending: " << pair.first << ": " << pair.second << std::endl;
            dataStream << pair.first << "=" << pair.second << "\n";
        }
    }
    dataStream << "\n\n";
    std::string data = dataStream.str();

    unsigned char checksum[MD5_DIGEST_LENGTH];
    MD5(reinterpret_cast<const unsigned char *>(data.c_str()), data.size(), checksum);

    if (send(datatransferSocket, data.c_str(), data.size(), 0) < 0) {
        std::cerr << "Error sending data" << std::endl;
        return 0;
    }

    std::string ackResponse;
    if (!receiveData(datatransferSocket, ackResponse) || ackResponse != "ACK") {
        std::cerr << "Error receiving ACK for data" << std::endl;
        return 0;
    }

    if (send(datatransferSocket, checksum, MD5_DIGEST_LENGTH, 0) < 0) {
        std::cerr << "Error sending checksum" << std::endl;
        return 0;
    }

    if (!receiveData(datatransferSocket, ackResponse) || ackResponse != "ACK") {
        std::cerr << "Error receiving ACK for checksum" << std::endl;
        return 0;
    }

//    std::cout << "Store Server " << storeId << " :data sent successfully" << std::endl;
    return 1;
}

int CommandThread::receiveChunckdata(int dataRecvSocket, std::string &keyRange) {
//    std::cout << "Store Server " << storeId << " :starting receiving data" << std::endl;

    char buffer[4096] = {0};
    std::ostringstream receivedDataStream;
    ssize_t bytesRead;

    while ((bytesRead = recv(dataRecvSocket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytesRead] = '\0';
        receivedDataStream << buffer;

        if (std::string(buffer).find("\n\n") != std::string::npos) {
            break;
        }
    }

    if (bytesRead < 0) {
        std::cerr << "Error receiving data" << std::endl;
        return 0;
    }

    std::string receivedData = receivedDataStream.str();

    if (send(dataRecvSocket, "ACK", 3, 0) < 0) {
        std::cerr << "Error sending ACK for data" << std::endl;
        return 0;
    }

    unsigned char receivedChecksum[MD5_DIGEST_LENGTH];
    bytesRead = recv(dataRecvSocket, receivedChecksum, MD5_DIGEST_LENGTH, 0);

    if (bytesRead != MD5_DIGEST_LENGTH) {
        std::cerr << "Error receiving checksum" << std::endl;
        return 0;
    }

    unsigned char calculatedChecksum[MD5_DIGEST_LENGTH];
    MD5(reinterpret_cast<const unsigned char *>(receivedData.c_str()), receivedData.size(), calculatedChecksum);

    if (memcmp(receivedChecksum, calculatedChecksum, MD5_DIGEST_LENGTH) != 0) {
        std::cerr << "Checksum validation failed" << std::endl;
        return 0;
    }

    if (send(dataRecvSocket, "ACK", 3, 0) < 0) {
        std::cerr << "Error sending ACK for checksum" << std::endl;
        return 0;
    }

    std::istringstream dataStream(receivedData);
    std::string line;

    while (std::getline(dataStream, line)) {
        size_t delimiterPos = line.find('=');
        if (delimiterPos != std::string::npos) {
            std::string key = line.substr(0, delimiterPos);
            std::string value = line.substr(delimiterPos + 1);
//            std::cout << "Store Server " << storeId << " :data received: " << key << ": " << value << std::endl;
            kvMap.put(key, value); // Insert the key-value pair into KVMap
        }
    }

//    std::cout << "Store Server " << storeId << " :data received and stored successfully" << std::endl;
    return 1;
}