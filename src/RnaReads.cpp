#include "RnaReads.hpp"

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

const char* RnaReads::name() const {
    return "rna_reads";
}

std::vector<TargetParser::TargetParam> RnaReads::parseTargetParams(
    const ConfigReader::Config& config
) const {
    return {
        makeBoolParam("simulate_rna_reads", config.sequencing.simulateRnaReads),
        makeStringParam("platform", config.sequencing.platform),
        makeStringParam("profile", config.sequencing.profile),
        makeStringParam("output_layout", config.sequencing.outputLayout),
        makeIntParam("read_length", config.sequencing.readLength),
        makeIntParam("read2_length", config.sequencing.read2Length),
        makeStringParam("expression_unit", config.expression.unit),
        makeBoolParam("strand_specific_genes", config.transcriptome.strandSpecificGenes),
    };
}
