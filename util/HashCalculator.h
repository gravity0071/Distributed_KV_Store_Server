//
// Created by Shawn Wan on 2024/11/12.
//

#ifndef DISTRIBUTED_KV_STORE_MASTER_HASHCALCULATOR_H
#define DISTRIBUTED_KV_STORE_MASTER_HASHCALCULATOR_H

#include <string>
#include <functional>

class HashCalculator {
public:
    // Constructor sets the modulus range (e.g., 0-10000)
    explicit HashCalculator(size_t range = 10000) : range(range) {}

    // Calculate the hash value for a given key and map it to the specified range
    size_t calculateHash(const std::string &key) const {
        size_t hashValue = std::hash<std::string>()(key); // Default hash calculation
        return hashValue % range; // Scale to the range [0, range)
    }

private:
    size_t range; // The range for the hash value (default is 0-10000)
};

#endif //DISTRIBUTED_KV_STORE_MASTER_HASHCALCULATOR_H