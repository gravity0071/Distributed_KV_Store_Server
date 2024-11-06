// ConcurrentMap.tpp
// Created by Shawn Wan on 2024/11/5

#include <iostream>

// Implementation of ConcurrentMap methods

template <typename K, typename V>
void ConcurrentMap<K, V>::put(const K& key, const V& value) {
    std::unique_lock lock(mutex);  // Exclusive lock for write
    map[key] = value;
}

template <typename K, typename V>
bool ConcurrentMap<K, V>::get(const K& key, V& value) const {
    std::shared_lock lock(mutex);  // Shared lock for read
    auto it = map.find(key);
    if (it != map.end()) {
        value = it->second;
        return true;
    }
    return false;
}

template <typename K, typename V>
std::string ConcurrentMap<K, V>::browse() const {
    std::shared_lock lock(mutex);  // Shared lock for read-only operation
    std::ostringstream oss;
    for (const auto& [key, value] : map) {
        oss << key << ": " << value << "\n";
    }
    return oss.str();  // Return as a formatted string
}