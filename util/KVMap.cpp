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

// 遍历所有键值对
std::string KVMap::browse() const {
    std::shared_lock<std::shared_mutex> lock(mutex); // 共享锁用于读取
    std::ostringstream oss;
    for (const auto& pair : map) {
        oss << pair.first << ": " << pair.second << "\n";
    }
    return oss.str();
}
