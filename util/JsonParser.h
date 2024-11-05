//
// Created by Shawn Wan on 2024/11/5.
//
/*
 *  JsonParser parser;
 *  parser.MapToJson(Map); -> return string
 *  parser.JsonToMap(JsonString); -> return map
 */
#ifndef JSONPARSER_H
#define JSONPARSER_H

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <nlohmann/json.hpp>  // Include the nlohmann/json library

using json = nlohmann::json;

class JsonParser {
public:
    // Function that takes a JSON string and returns a map
    std::map<std::string, std::string> JsonToMap(const std::string& jsonString) const {
        std::map<std::string, std::string> result;
        try {
            // Parse the JSON string into a json object
            json jsonData = json::parse(jsonString);

            // Check if jsonData is a JSON object (not an array or other type)
            if (jsonData.is_object()) {
                for (auto& [key, value] : jsonData.items()) {
                    result[key] = value.get<std::string>();  // Convert each value to a string
                }
            }
        } catch (json::parse_error& e) {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
        } catch (json::type_error& e) {
            std::cerr << "JSON type error: " << e.what() << std::endl;
        }
        return result;
    }

    std::string MapToJson(const std::map<std::string, std::string>& dataMap) const {
        json jsonData;
        for (const auto& [key, value] : dataMap) {
            jsonData[key] = value;
        }
        return jsonData.dump();  // Return as compact JSON string
    }
};

#endif // JSONPARSER_H