//
// Created by Shawn Wan on 2024/11/5.
//

// ConcurrentMap.h
#pragma once

#include <unordered_map>
#include <shared_mutex>
#include <iostream>

template <typename K, typename V>
class ConcurrentMap {
    std::unordered_map<K, V> map;
    mutable std::shared_mutex mutex;

public:
    // Insert or update a key-value pair
    void put(const K& key, const V& value) {
        std::unique_lock lock(mutex);  // Exclusive lock for write
        map[key] = value;
    }

    // Retrieve a value by key
    bool get(const K& key, V& value) const {
        std::shared_lock lock(mutex);  // Shared lock for read
        auto it = map.find(key);
        if (it != map.end()) {
            value = it->second;
            return true;
        }
        return false;
    }

    // Browse and print all key-value pairs
    void browse() const {
        std::shared_lock lock(mutex);  // Shared lock for read-only operation
        for (const auto& [key, value] : map) {

            //todo should return a map or key-value strings
            std::cout << key << ": " << value << std::endl;
        }
    }
};