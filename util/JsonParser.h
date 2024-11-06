// JsonParser.h
// Created by Shawn Wan on 2024/11/5

#ifndef JSONPARSER_H
#define JSONPARSER_H

#include <iostream>
#include <string>
#include <map>
#include <nlohmann/json.hpp>  // Include the nlohmann/json library

using json = nlohmann::json;

class JsonParser {
public:
    // Function that takes a JSON string and returns a map
    std::map<std::string, std::string> JsonToMap(const std::string &jsonString) const;

    // Function that takes a map and returns a JSON string
    std::string MapToJson(const std::map<std::string, std::string> &dataMap) const;
};

#endif // JSONPARSER_H