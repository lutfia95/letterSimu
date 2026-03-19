#include "DnaReads.hpp"

#include <string>

namespace {

TargetParser::TargetParam makeBoolParam(const std::string& key, bool value) {
    return TargetParser::TargetParam{key, value ? "true" : "false"};
}

TargetParser::TargetParam makeStringParam(const std::string& key, const std::string& value) {
    return TargetParser::TargetParam{key, value};
}

TargetParser::TargetParam makeIntParam(const std::string& key, std::int64_t value) {
    return TargetParser::TargetParam{key, std::to_string(value)};
}

}  // namespace

const char* DnaReads::name() const {
    return "dna_reads";
}

std::vector<TargetParser::TargetParam> DnaReads::parseTargetParams(const ConfigReader::Config& config) const {
    return {
        makeBoolParam("simulate_dna_reads", config.sequencing.simulateDnaReads),
        makeStringParam("platform", config.sequencing.platform),
        makeStringParam("profile", config.sequencing.profile),
        makeStringParam("output_layout", config.sequencing.outputLayout),
        makeIntParam("read_length", config.sequencing.readLength),
        makeIntParam("read2_length", config.sequencing.read2Length),
        makeBoolParam("targeted_enable", config.targeted.enable),
        makeStringParam("targeted_mode", config.targeted.mode),
    };
}
