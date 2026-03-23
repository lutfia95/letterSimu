#include "RnaTranscriptome.hpp"

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

const char* RnaTranscriptome::name() const {
    return "rna_transcriptome";
}

std::vector<TargetParser::TargetParam> RnaTranscriptome::parseTargetParams(
    const ConfigReader::Config& config
) const {
    return {
        makeStringParam("annotation_gtf", config.input.annotationGtf),
        makeStringParam("transcript_fasta", config.input.transcriptFasta),
        makeBoolParam("use_existing_annotation", config.input.useExistingAnnotation),
        makeBoolParam("enable_transcriptome", config.transcriptome.enableTranscriptome),
        makeBoolParam("derive_from_annotation", config.transcriptome.deriveFromAnnotation),
        makeIntParam("number_of_genes", config.transcriptome.numberOfGenes),
        makeBoolParam("propagate_dna_variants_to_rna", config.simulation.propagateDnaVariantsToRna),
        makeBoolParam("transcribe_from_mutated_genome", config.simulation.transcribeFromMutatedGenome),
    };
}
