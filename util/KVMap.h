#pragma once

#include <string>
#include <map>
#include <shared_mutex>

class KVMap {
private:
    mutable std::shared_mutex mutex;   
    std::map<std::string, std::string> map; 

public:
    void write(const std::string& key, const std::string& value);

    void put(const std::string &key, const std::string &value);

    bool get(const std::string &key, std::string &value) const;

    bool remove(const std::string& key);

    bool increment(const std::string& key);

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