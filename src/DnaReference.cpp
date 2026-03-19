#include "DnaReference.hpp"

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

const char* DnaReference::name() const {
    return "dna_reference";
}

std::vector<TargetParser::TargetParam> DnaReference::parseTargetParams(const ConfigReader::Config& config) const {
    return {
        makeStringParam("reference_fasta", config.input.referenceFasta),
        makeStringParam("target_regions_bed", config.input.targetRegionsBed),
        makeBoolParam("use_existing_reference", config.input.useExistingReference),
        makeBoolParam("generate_denovo_sequences", config.input.generateDenovoSequences),
        makeBoolParam("mutate_existing_reference", config.input.mutateExistingReference),
        makeBoolParam("restrict_to_target_regions", config.input.restrictToTargetRegions),
        makeStringParam("alphabet", config.sequence.alphabet),
        makeIntParam("ploidy", config.simulation.ploidy),
    };
}
