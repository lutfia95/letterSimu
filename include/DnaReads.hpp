#pragma once

#include "TargetParser.hpp"

class DnaReads final : public TargetParser {
public:
    const char* name() const override;
    std::vector<TargetParam> parseTargetParams(const ConfigReader::Config& config) const override;
};
