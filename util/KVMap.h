#pragma once

#include <string>
#include <map>
#include <shared_mutex>

class KVMap {
private:
    mutable std::shared_mutex mutex;   // 共享锁，用于同步读写操作
    std::map<std::string, std::string> map; // 存储键值对

public:
    /**
     * 插入或更新键值对
     * @param key 键
     * @param value 值
     */
    void put(const std::string& key, const std::string& value);
    void write(const std::string& key, const std::string& value);

    /**
     * 根据键获取值
     * @param key 键
     * @param value 返回的值
     * @return 如果键存在返回 true，否则返回 false
     */
    bool get(const std::string& key, std::string& value) const;

    /**
     * 删除键值对
     * @param key 键
     * @return 如果键存在并被删除返回 true，否则返回 false
     */
    bool remove(const std::string& key);

    /**
     * 增加键对应的值（假设值为整数）
     * @param key 键
     * @return 如果键存在且值是整数，则增加后返回 true，否则返回 false
     */
    bool increment(const std::string& key);

    /**
     * 遍历所有键值对
     * @return 包含所有键值对的字符串
     */
    std::string browse() const;
};
