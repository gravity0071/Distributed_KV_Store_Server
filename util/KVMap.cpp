//
// Created by Shawn Wan on 2024/11/14.
//

#include "KVMap.h"
#include <sstream>

// Insert or update a key-value pair
void KVMap::put(const std::string& key, const std::string& value) {
    std::unique_lock<std::shared_mutex> lock(mutex); // Exclusive lock for writing
    map[key] = value;
}

// Retrieve a value by key
bool KVMap::get(const std::string& key, std::string& value) const {
    std::shared_lock<std::shared_mutex> lock(mutex); // Shared lock for reading
    auto it = map.find(key);
    if (it != map.end()) {
        value = it->second;
        return true;
    }
    return false;
}

// Browse and return all key-value pairs as a string
std::string KVMap::browse() const {
    std::shared_lock<std::shared_mutex> lock(mutex); // Shared lock for reading
    std::ostringstream oss;

    for (const auto& pair : map) {
        oss << pair.first << ": " << pair.second << "\n";
    }

    return oss.str();
}