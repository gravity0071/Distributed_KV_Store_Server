#include "gtest/gtest.h"
#include "../util/HashCalculator.h"
#include "../util/JsonParser.h"
#include "../util/KVMap.h"
#include "../handleLocalCommand/localCommandMainFunction.h"
#include "../util/TcpConnectionUtility.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <chrono>

class DataTransferTest : public ::testing::Test {
protected:
    KVMap senderKVMap;
    KVMap receiverKVMap;
    JsonParser jsonParser;
    TcpConnectionUtility tcpConnectionUtility;
    bool isMigrating = false;
    std::atomic<bool> isRunning{true};
    int serverPort = 9034;

    void SetUp() override {
        senderKVMap.put("apple", "fruit");
        senderKVMap.put("carrot", "vegetable");
        senderKVMap.put("banana", "fruit");
        senderKVMap.put("dog", "animal");
    }

    void TearDown() override {
        senderKVMap.clear();
        receiverKVMap.clear();
    }
};

TEST_F(DataTransferTest, DataTransferSuccess) {
    // Start server in a separate thread
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_NE(serverSocket, -1) << "Failed to create server socket";

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(serverPort);

    ASSERT_EQ(bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)), 0)
                                << "Failed to bind server socket";

    ASSERT_EQ(listen(serverSocket, 1), 0) << "Failed to listen on server socket";

    std::thread serverThread([&]() {
        sockaddr_in clientAddress{};
        socklen_t clientLen = sizeof(clientAddress);
        int dataRecvSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientLen);
        ASSERT_NE(dataRecvSocket, -1) << "Failed to accept client connection";

        CommandThread receiverCommandThread(receiverKVMap, serverPort, 0, isMigrating, isRunning, "receiverStore", jsonParser);

        std::string keyRange = "0-5000";
        ASSERT_EQ(receiverCommandThread.receiveChunckdata(dataRecvSocket, keyRange), 1)
                                    << "Data reception failed";

        close(dataRecvSocket);
    });

    // Simulate client to send data
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Wait for server to be ready
    int clientSocket = tcpConnectionUtility.connectToServer("127.0.0.1", serverPort);
    ASSERT_NE(clientSocket, -1) << "Failed to connect to server";

    CommandThread senderCommandThread(senderKVMap, 0, 0, isMigrating, isRunning, "senderStore", jsonParser);

    std::string keyRange = "0-5000";
    ASSERT_EQ(senderCommandThread.sendChunkData(clientSocket, keyRange), 1)
                                << "Data transfer failed";

    close(clientSocket);
    serverThread.join();
    close(serverSocket);

    HashCalculator hashCalculator;
    std::cout << "apple: " << hashCalculator.calculateHash("apple") << std::endl;
    std::cout << "carrot: " << hashCalculator.calculateHash("carrot") << std::endl;
    std::cout << "banana: " << hashCalculator.calculateHash("banana") << std::endl;
    std::cout << "dog: " << hashCalculator.calculateHash("dog") << std::endl;

    // Verify that data has been transferred correctly
    std::string value;
    EXPECT_FALSE(receiverKVMap.get("apple", value));
    EXPECT_FALSE(receiverKVMap.get("carrot", value));
    EXPECT_TRUE(receiverKVMap.get("banana", value)); // Out of range
    EXPECT_EQ(value, "fruit");
    EXPECT_TRUE(receiverKVMap.get("dog", value));    // Out of range
    EXPECT_EQ(value, "animal");
}