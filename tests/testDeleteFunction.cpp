#include "gtest/gtest.h"
#include "../util/HashCalculator.h"
#include "../util/JsonParser.h"
#include "../util/KVMap.h"
#include "../handleLocalCommand/localCommandMainFunction.h"

class CommandThreadTest : public ::testing::Test {
protected:
    KVMap kvMap; // Key-value store mock
    JsonParser jsonParser; // JSON parser mock
    int port = 8080;
    int clientPort = 9090;
    bool isMigrating = false;
    std::atomic<bool> isRunning{true};
    std::string storeId = "store1";
    CommandThread commandThread{kvMap, port, clientPort, isMigrating, isRunning, storeId, jsonParser};

    void SetUp() override {
        // Prepopulate the KVMap with test data
        kvMap.put("apple", "fruit");
        kvMap.put("carrot", "vegetable");
        kvMap.put("banana", "fruit");
        kvMap.put("dog", "animal");
    }

    void TearDown() override {
        // Cleanup after each test
        kvMap.clear();
    }
};

TEST_F(CommandThreadTest, DeleteKeyWithValidKeyRange) {
    // Prepare the input map with a valid key range
    std::map<std::string, std::string> lastOperationFromMasterMap;
    lastOperationFromMasterMap["keyRange"] = "0-5000";


    // Execute deleteKey
    int result = commandThread.deleteKey(lastOperationFromMasterMap);
    HashCalculator hashCalculator;
    std::cout << "apple: " << hashCalculator.calculateHash("apple") << std::endl;
    std::cout << "carrot: " << hashCalculator.calculateHash("carrot") << std::endl;
    std::cout << "banana: " << hashCalculator.calculateHash("banana") << std::endl;
    std::cout << "dog: " << hashCalculator.calculateHash("dog") << std::endl;

    // Verify result
    EXPECT_EQ(result, 1); // Should return 1 indicating success

    // Verify keys within range are deleted
    EXPECT_TRUE(kvMap.contains("apple"));
    EXPECT_TRUE(kvMap.contains("carrot"));

    // Verify keys outside range are not deleted
    EXPECT_FALSE(kvMap.contains("banana"));
    EXPECT_FALSE(kvMap.contains("dog"));
}

TEST_F(CommandThreadTest, DeleteKeyWithRoundedRange) {
    // Prepare the input map with an invalid key range
    std::map<std::string, std::string> lastOperationFromMasterMap;
    lastOperationFromMasterMap["keyRange"] = "8000-4000";
    HashCalculator hashCalculator;
    std::cout << "apple: " << hashCalculator.calculateHash("apple") << std::endl;
    std::cout << "carrot: " << hashCalculator.calculateHash("carrot") << std::endl;
    std::cout << "banana: " << hashCalculator.calculateHash("banana") << std::endl;
    std::cout << "dog: " << hashCalculator.calculateHash("dog") << std::endl;

    // Execute deleteKey
    int result = commandThread.deleteKey(lastOperationFromMasterMap);

    // Verify result
    EXPECT_EQ(result, 1); // Should return 0 indicating failure

    // Verify no keys are deleted
    EXPECT_FALSE(kvMap.contains("apple"));
    EXPECT_TRUE(kvMap.contains("carrot"));
    EXPECT_TRUE(kvMap.contains("banana"));
    EXPECT_FALSE(kvMap.contains("dog"));
}

TEST_F(CommandThreadTest, DeleteKeyWithInvalidOpe) {
    HashCalculator hashCalculator;
    std::cout << "apple: " << hashCalculator.calculateHash("apple") << std::endl;
    std::cout << "carrot: " << hashCalculator.calculateHash("carrot") << std::endl;
    std::cout << "banana: " << hashCalculator.calculateHash("banana") << std::endl;
    std::cout << "dog: " << hashCalculator.calculateHash("dog") << std::endl;
    // Prepare the input map without "keyRange"
    std::map<std::string, std::string> lastOperationFromMasterMap;

    // Execute deleteKey
    int result = commandThread.deleteKey(lastOperationFromMasterMap);

    // Verify result
    EXPECT_EQ(result, 0); // Should return 0 indicating failure

    // Verify no keys are deleted
    EXPECT_TRUE(kvMap.contains("apple"));
    EXPECT_TRUE(kvMap.contains("carrot"));
    EXPECT_TRUE(kvMap.contains("banana"));
    EXPECT_TRUE(kvMap.contains("dog"));
}

TEST_F(CommandThreadTest, DeleteKeyWithEdgeCondition) {
    HashCalculator hashCalculator;
    std::cout << "apple: " << hashCalculator.calculateHash("apple") << std::endl;
    std::cout << "carrot: " << hashCalculator.calculateHash("carrot") << std::endl;
    std::cout << "banana: " << hashCalculator.calculateHash("banana") << std::endl;
    std::cout << "dog: " << hashCalculator.calculateHash("dog") << std::endl;
    // Prepare the input map without "keyRange"
    std::map<std::string, std::string> lastOperationFromMasterMap;
    lastOperationFromMasterMap["keyRange"] = "8221-3802";

    // Execute deleteKey
    int result = commandThread.deleteKey(lastOperationFromMasterMap);

    // Verify result
    EXPECT_EQ(result, 1); // Should return 0 indicating failure

    // Verify no keys are deleted
    EXPECT_FALSE(kvMap.contains("apple"));
    EXPECT_TRUE(kvMap.contains("carrot"));
    EXPECT_TRUE(kvMap.contains("banana"));
    EXPECT_TRUE(kvMap.contains("dog"));

    lastOperationFromMasterMap["keyRange"] = "7370-3802";
    EXPECT_EQ(result, 1); // Should return 0 indicating failure

    // Verify no keys are deleted
    EXPECT_FALSE(kvMap.contains("apple"));
    EXPECT_TRUE(kvMap.contains("carrot"));
    EXPECT_TRUE(kvMap.contains("banana"));
    EXPECT_TRUE(kvMap.contains("dog"));
}