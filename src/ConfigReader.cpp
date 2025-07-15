#include "ConfigReader.h"
#include <fstream>
#include <iostream>
#include <algorithm>

static inline std::string trim(const std::string& str) {
    auto start = std::find_if_not(str.begin(), str.end(), ::isspace);
    auto end = std::find_if_not(str.rbegin(), str.rend(), ::isspace).base();
    return (start < end ? std::string(start, end) : std::string());
}

std::optional<std::map<std::string, std::string>> readConfig(const std::string& path) {
    std::ifstream config(path);
    
    if (!config.is_open()) {
        std::cerr << "Failed to open config file at: " << path << std::endl;
        return {};
    }

    std::string line;
    std::map<std::string, std::string> data;
    int lineNumber = 0;
    
    while (std::getline(config, line)) {
        lineNumber++;
        std::string trimmedLine = trim(line);
        
        if (trimmedLine.empty() || trimmedLine[0] == '#') {
            continue;
        }
        
        size_t pos = trimmedLine.find('=');
        if (pos != std::string::npos) {
            std::string key = trim(trimmedLine.substr(0, pos));
            std::string value = trim(trimmedLine.substr(pos + 1));
            
            if (key.empty()) {
                std::cerr << "Warning: Empty key at line " << lineNumber << std::endl;
                continue;
            }
            
            data[key] = value;
        } else {
            std::cerr << "Warning: Invalid format at line " << lineNumber << ": " << line << std::endl;
        }
    }
    
    if (data.empty()) {
        std::cerr << "Warning: No valid configuration entries found in " << path << std::endl;
        return {};
    }
    
    return data;
}