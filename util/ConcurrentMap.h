// ConcurrentMap.h
// Created by Shawn Wan on 2024/11/5

#pragma once

#include <unordered_map>
#include <shared_mutex>
#include <sstream>

template <typename K, typename V>
class ConcurrentMap {
    std::unordered_map<K, V> map;        // Internal map storage
    mutable std::shared_mutex mutex;     // Mutex for concurrent access

public:
    // Insert or update a key-value pair
    void put(const K& key, const V& value);

    // Retrieve a value by key
    bool get(const K& key, V& value) const;

    // Browse and return all key-value pairs as a string
    std::string browse() const;
};

// Include the template implementation file
#include "ConcurrentMap.tpp"