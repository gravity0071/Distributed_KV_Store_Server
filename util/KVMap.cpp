#include "KVMap.h"
#include <sstream>
#include <iostream> // 必须包含以使用 std::cout

// 插入或更新键值对
void KVMap::put(const std::string& key, const std::string& value) {
    std::unique_lock<std::shared_mutex> lock(mutex); // 独占锁用于写入
    map[key] = value;
    std::cout << "KVMap: Put key=" << key << ", value=" << value << std::endl;
}

// 根据键获取值
bool KVMap::get(const std::string& key, std::string& value) const {
    std::shared_lock<std::shared_mutex> lock(mutex); // 共享锁用于读取
    auto it = map.find(key);
    if (it != map.end()) {
        value = it->second;
        std::cout << "KVMap: Found key=" << key << ", value=" << value << std::endl;
        return true;
    }
    std::cout << "KVMap: Key=" << key << " not found.\n";
    return false;
}

// 删除键值对
bool KVMap::remove(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mutex); // 独占锁用于写入
    auto it = map.find(key);
    if (it != map.end()) {
        map.erase(it);
        std::cout << "KVMap: Removed key=" << key << std::endl;
        return true;
    }
    std::cout << "KVMap: Key=" << key << " not found. Remove operation failed.\n";
    return false;
}

// 增加键对应的值（假设值为整数）
bool KVMap::increment(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mutex); // 独占锁用于写入
    auto it = map.find(key);
    if (it != map.end()) {
        try {
            int currentValue = std::stoi(it->second);
            ++currentValue;
            it->second = std::to_string(currentValue);
            std::cout << "KVMap: Incremented key=" << key << ", new value=" << currentValue << std::endl;
            return true;
        } catch (const std::exception& e) {
            std::cout << "KVMap: Increment failed for key=" << key << ". Value is not an integer.\n";
            return false;
        }
    }
    std::cout << "KVMap: Key=" << key << " not found. Increment operation failed.\n";
    return false;
}

// 遍历所有键值对
std::string KVMap::browse() const {
    std::shared_lock<std::shared_mutex> lock(mutex); // 共享锁用于读取
    std::ostringstream oss;
    for (const auto& pair : map) {
        oss << pair.first << ": " << pair.second << "\n";
    }
    return oss.str();
}
