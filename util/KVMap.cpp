#include "KVMap.h"
#include <sstream>
#include <iostream>

void KVMap::put(const std::string& key, const std::string& value) {
    std::unique_lock<std::shared_mutex> lock(mutex); 
    map[key] = value;
//    std::cout << "KVMap: Put key=" << key << ", value=" << value << std::endl;
}

void KVMap::write(const std::string& key, const std::string& value) {
    std::unique_lock<std::shared_mutex> lock(mutex); 
    map[key] = value;
    std::cout << "KVMap: Write key=" << key << ", value=" << value << std::endl;
}

bool KVMap::get(const std::string& key, std::string& value) const {
    std::shared_lock<std::shared_mutex> lock(mutex); 
    auto it = map.find(key);
    if (it != map.end()) {
        value = it->second;
        std::cout << "KVMap: Found key=" << key << ", value=" << value << std::endl;
        return true;
    }
    std::cout << "KVMap: Key=" << key << " not found.\n";
    return false;
}

bool KVMap::remove(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    auto it = map.find(key);
    if (it != map.end()) {
        map.erase(it);
        std::cout << "KVMap: Removed key=" << key << std::endl;
        return true;
    }
    std::cout << "KVMap: Key=" << key << " not found. Remove operation failed.\n";
    return false;
}

bool KVMap::increment(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mutex); 
    auto it = map.find(key);
    if (it != map.end()) {
        try {
            int currentValue = std::stoi(it->second);
            ++currentValue;
            it->second = std::to_string(currentValue);
//            std::cout << "KVMap: Incremented key=" << key << ", new value=" << currentValue << std::endl;
            return true;
        } catch (const std::exception& e) {
//            std::cout << "KVMap: Increment failed for key=" << key << ". Value is not an integer.\n";
            return false;
        }
    }
    std::cout << "KVMap: Key=" << key << " not found. Increment operation failed.\n";
    return false;
}

std::string KVMap::browse() const {
    std::shared_lock<std::shared_mutex> lock(mutex); 
    std::ostringstream oss;
    for (const auto& pair : map) {
        oss << pair.first << ": " << pair.second << ", ";
    }
    oss << "\n";
    return oss.str();
}

bool KVMap::deleteKey(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    return map.erase(key) > 0;
}

bool KVMap::contains(const std::string &key) const {
    std::shared_lock lock(mutex);
    return map.find(key) != map.end();
}

void KVMap::clear() {
    std::unique_lock lock(mutex);
    map.clear();
}