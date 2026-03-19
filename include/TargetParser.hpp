#pragma once

#include "ConfigReader.hpp"

#include <string>
#include <vector>

class TargetParser {
public:
    struct TargetParam {
        std::string key;
        std::string value;
    };

    virtual ~TargetParser() = default;

    virtual const char* name() const = 0;
    virtual std::vector<TargetParam> parseTargetParams(const ConfigReader::Config& config) const = 0;
};
