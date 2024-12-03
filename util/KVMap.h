//
// Created by Shawn Wan on 2024/11/14.
//
#pragma once

#include <unordered_map>
#include <shared_mutex>
#include <string>

class KVMap {
private:
    std::unordered_map<std::string, std::string> map; // Internal map storage
    mutable std::shared_mutex mutex;                 // Mutex for concurrent access

public:
    // Insert or update a key-value pair
    void put(const std::string &key, const std::string &value);

    // Retrieve a value by key
    bool get(const std::string &key, std::string &value) const;

    // Browse and return all key-value pairs as a string
    std::string browse() const;

    bool deleteKey(const std::string &key);

    auto begin() const {
        return map.begin();
    }

    auto end() const {
        return map.end();
    }

    bool contains(const std::string &key) const;
    void clear();
};