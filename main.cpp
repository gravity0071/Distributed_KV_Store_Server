//
// Created by Shawn Wan on 2024/11/5.
//
#include <iostream>
#include <thread>
#include "util/ConcurrentMap.h"
#include "util/JsonParser.h"
#include "util/Server.h"

using namespace std;

//// Function to insert key-value pairs in the map
//void putValues(ConcurrentMap<int, string>& cmap) {
//    for (int i = 0; i < 10; ++i) {
//        cmap.put(i, "Value" + to_string(i));
//        cout << "Inserted key " << i << " with value: " << "Value" + to_string(i) << endl;
//    }
//}
//
//// Function to get values from the map
//void getValues(ConcurrentMap<int, string>& cmap) {
//    for (int i = 0; i < 10; ++i) {
//        string value;
//        if (cmap.get(i, value)) {
//            cout << "Retrieved key " << i << " with value: " << value << endl;
//        } else {
//            cout << "Key " << i << " not found" << endl;
//        }
//    }
//}
//
//// Function to browse all key-value pairs
//void browseMap(const ConcurrentMap<int, string>& cmap) {
//    cout << "Browsing all key-value pairs:" << endl;
//    cmap.browse();
//}
//
//int main() {
//    // Create a ConcurrentMap of <int, string>
//    ConcurrentMap<int, string> cmap;
//
//    // Create threads to test concurrent put, get, and browse operations
//    thread t1(putValues, ref(cmap));        // Thread to insert values
//    thread t2(getValues, ref(cmap));        // Thread to get values
//    thread t3(browseMap, cref(cmap));       // Thread to browse values
//
//    // Wait for all threads to complete
//    t1.join();
//    t2.join();
//    t3.join();
//
//    cout << "\nFinal browsing of all key-value pairs:" << endl;
//    cmap.browse();
//
//    return 0;
//}

int main() {
    Server server; // Initialize server on default port 8080
    server.start(); // Start the server to listen for clients

    return 0;
}

//int main() {
//    JsonParser parser;
//
//    // Convert map to JSON string
//    std::map<std::string, std::string> sampleMap = {
//            {"name", "Alice"},
//            {"age", "25"},
//            {"city", "Los Angeles"}
//    };
//
//    std::string jsonStr = parser.MapToJson(sampleMap);
//    std::cout << "Map to JSON: " << jsonStr << std::endl;
//
//    // Convert JSON string back to map
//    std::map<std::string, std::string> resultMap = parser.JsonToMap(jsonStr);
//    std::cout << "JSON to Map:" << std::endl;
//    for (const auto &[key, value] : resultMap) {
//        std::cout << key << ": " << value << std::endl;
//    }
//
//    return 0;
//}