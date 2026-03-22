#include "ConfigReader.hpp"
#include "Logger.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <type_traits>

namespace {

std::string trim(const std::string& input) {
    const auto first = input.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }

    const auto last = input.find_last_not_of(" \t\r\n");
    return input.substr(first, last - first + 1);
}

std::string stripComments(const std::string& line) {
    bool inString = false;

    for (std::size_t i = 0; i < line.size(); ++i) {
        if (line[i] == '"' && (i == 0 || line[i - 1] != '\\')) {
            inString = !inString;
        }

        if (!inString && line[i] == '#') {
            return line.substr(0, i);
        }
    }

    return line;
}

std::size_t findAssignment(const std::string& line) {
    bool inString = false;
    int bracketDepth = 0;

    for (std::size_t i = 0; i < line.size(); ++i) {
        if (line[i] == '"' && (i == 0 || line[i - 1] != '\\')) {
            inString = !inString;
            continue;
        }

        if (inString) {
            continue;
        }

        if (line[i] == '[') {
            ++bracketDepth;
        } else if (line[i] == ']') {
            --bracketDepth;
        } else if (line[i] == '=' && bracketDepth == 0) {
            return i;
        }
    }

    return std::string::npos;
}

std::vector<std::string> parseArrayItems(const std::string& input) {
    std::vector<std::string> items;
    std::string current;
    bool inString = false;

    for (std::size_t i = 0; i < input.size(); ++i) {
        const char ch = input[i];
        if (ch == '"' && (i == 0 || input[i - 1] != '\\')) {
            inString = !inString;
            current.push_back(ch);
            continue;
        }

        if (ch == ',' && !inString) {
            items.push_back(trim(current));
            current.clear();
            continue;
        }

        current.push_back(ch);
    }

    if (!trim(current).empty()) {
        items.push_back(trim(current));
    }

    return items;
}

std::string unquote(const std::string& value) {
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        return value.substr(1, value.size() - 2);
    }
    return value;
}

std::string formatStringArray(const std::vector<std::string>& values) {
    std::ostringstream oss;
    oss << "[";
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0) {
            oss << ", ";
        }
        oss << '"' << values[i] << '"';
    }
    oss << "]";
    return oss.str();
}

std::string formatValue(const ConfigReader::Value& value) {
    return std::visit([](const auto& typedValue) -> std::string {
        using T = std::decay_t<decltype(typedValue)>;

        if constexpr (std::is_same_v<T, bool>) {
            return typedValue ? "true" : "false";
        } else if constexpr (std::is_same_v<T, std::int64_t>) {
            return std::to_string(typedValue);
        } else if constexpr (std::is_same_v<T, double>) {
            std::ostringstream oss;
            oss << typedValue;
            return oss.str();
        } else if constexpr (std::is_same_v<T, std::string>) {
            return '"' + typedValue + '"';
        } else {
            return formatStringArray(typedValue);
        }
    }, value);
}

ConfigReader::Value parseValue(const std::string& rawValue) {
    const std::string value = trim(rawValue);
    if (value.empty()) {
        throw std::runtime_error("empty value");
    }

    if (value == "true") {
        return true;
    }

    if (value == "false") {
        return false;
    }

    if (value.front() == '"' && value.back() == '"') {
        return unquote(value);
    }

    if (value.front() == '[' && value.back() == ']') {
        const std::string inner = trim(value.substr(1, value.size() - 2));
        std::vector<std::string> result;
        if (inner.empty()) {
            return result;
        }

        for (const std::string& item : parseArrayItems(inner)) {
            result.push_back(unquote(trim(item)));
        }
        return result;
    }

    if (value.find_first_of(".eE") != std::string::npos) {
        return std::stod(value);
    }

    return static_cast<std::int64_t>(std::stoll(value));
}

template <typename T>
std::optional<T> getAs(const std::unordered_map<std::string, ConfigReader::Value>& values, const std::string& key) {
    const auto it = values.find(key);
    if (it == values.end()) {
        return std::nullopt;
    }

    if (const auto* value = std::get_if<T>(&it->second)) {
        return *value;
    }

    return std::nullopt;
}

template <typename T>
void assignIfPresent(const std::optional<T>& value, T& target) {
    if (value.has_value()) {
        target = *value;
    }
}

ConfigReader::Config buildConfigFromValues(const std::unordered_map<std::string, ConfigReader::Value>& values) {
    auto getString = [&](const std::string& key) { return getAs<std::string>(values, key); };
    auto getInt = [&](const std::string& key) { return getAs<std::int64_t>(values, key); };
    auto getDouble = [&](const std::string& key) { return getAs<double>(values, key); };
    auto getBool = [&](const std::string& key) { return getAs<bool>(values, key); };
    auto getStringArray = [&](const std::string& key) { return getAs<std::vector<std::string>>(values, key); };

    ConfigReader::Config config;

    assignIfPresent(getString("version"), config.version);

    assignIfPresent(getString("run.name"), config.run.name);
    assignIfPresent(getString("run.description"), config.run.description);
    assignIfPresent(getInt("run.random_seed"), config.run.randomSeed);
    assignIfPresent(getInt("run.replicates"), config.run.replicates);
    assignIfPresent(getInt("run.threads"), config.run.threads);
    assignIfPresent(getBool("run.deterministic"), config.run.deterministic);
    assignIfPresent(getString("run.project"), config.run.project);

    assignIfPresent(getStringArray("simulation.mode"), config.simulation.mode);
    assignIfPresent(getString("simulation.organism_name"), config.simulation.organismName);
    assignIfPresent(getInt("simulation.ploidy"), config.simulation.ploidy);
    assignIfPresent(getBool("simulation.propagate_dna_variants_to_rna"), config.simulation.propagateDnaVariantsToRna);
    assignIfPresent(getBool("simulation.transcribe_from_mutated_genome"), config.simulation.transcribeFromMutatedGenome);

    assignIfPresent(getString("input.reference_fasta"), config.input.referenceFasta);
    assignIfPresent(getString("input.annotation_gtf"), config.input.annotationGtf);
    assignIfPresent(getString("input.transcript_fasta"), config.input.transcriptFasta);
    assignIfPresent(getString("input.phylogeny_newick"), config.input.phylogenyNewick);
    assignIfPresent(getString("input.variant_hotspots_bed"), config.input.variantHotspotsBed);
    assignIfPresent(getString("input.target_regions_bed"), config.input.targetRegionsBed);
    assignIfPresent(getString("input.known_sites_vcf"), config.input.knownSitesVcf);
    assignIfPresent(getBool("input.use_existing_reference"), config.input.useExistingReference);
    assignIfPresent(getBool("input.use_existing_annotation"), config.input.useExistingAnnotation);
    assignIfPresent(getBool("input.generate_denovo_sequences"), config.input.generateDenovoSequences);
    assignIfPresent(getBool("input.mutate_existing_reference"), config.input.mutateExistingReference);
    assignIfPresent(getBool("input.restrict_to_target_regions"), config.input.restrictToTargetRegions);

    assignIfPresent(getString("sequence.alphabet"), config.sequence.alphabet);
    assignIfPresent(getBool("sequence.ambiguous_bases_allowed"), config.sequence.ambiguousBasesAllowed);
    assignIfPresent(getInt("sequence.number_of_sequences"), config.sequence.numberOfSequences);
    assignIfPresent(getString("sequence.name_prefix"), config.sequence.namePrefix);
    assignIfPresent(getString("sequence.length_model"), config.sequence.lengthModel);
    assignIfPresent(getInt("sequence.fixed_length"), config.sequence.fixedLength);
    assignIfPresent(getInt("sequence.min_length"), config.sequence.minLength);
    assignIfPresent(getInt("sequence.max_length"), config.sequence.maxLength);
    assignIfPresent(getDouble("sequence.base_composition.gc_content"), config.sequence.baseComposition.gcContent);
    assignIfPresent(getDouble("sequence.base_composition.at_skew"), config.sequence.baseComposition.atSkew);
    assignIfPresent(getDouble("sequence.base_composition.gc_skew"), config.sequence.baseComposition.gcSkew);
    assignIfPresent(getBool("sequence.base_composition.enforce_global_base_frequencies"),
                    config.sequence.baseComposition.enforceGlobalBaseFrequencies);
    assignIfPresent(getDouble("sequence.base_frequencies.A"), config.sequence.baseFrequencies.a);
    assignIfPresent(getDouble("sequence.base_frequencies.C"), config.sequence.baseFrequencies.c);
    assignIfPresent(getDouble("sequence.base_frequencies.G"), config.sequence.baseFrequencies.g);
    assignIfPresent(getDouble("sequence.base_frequencies.T"), config.sequence.baseFrequencies.t);
    assignIfPresent(getBool("sequence.motifs.enable_motif_enrichment"), config.sequence.motifs.enableMotifEnrichment);
    assignIfPresent(getBool("sequence.motifs.enable_repeat_simulation"), config.sequence.motifs.enableRepeatSimulation);
    assignIfPresent(getDouble("sequence.motifs.repeat_fraction"), config.sequence.motifs.repeatFraction);
    assignIfPresent(getDouble("sequence.motifs.low_complexity_fraction"), config.sequence.motifs.lowComplexityFraction);
    assignIfPresent(getDouble("sequence.motifs.homopolymer_rate"), config.sequence.motifs.homopolymerRate);
    assignIfPresent(getInt("sequence.motifs.max_homopolymer_length"), config.sequence.motifs.maxHomopolymerLength);
    for (std::size_t index = 0;; ++index) {
        const std::string prefix = "sequence.motifs.enriched[" + std::to_string(index) + "]";
        if (!getString(prefix + ".name").has_value()) {
            break;
        }

        ConfigReader::SequenceConfig::MotifConfig::EnrichedMotifConfig enriched;
        assignIfPresent(getString(prefix + ".name"), enriched.name);
        assignIfPresent(getString(prefix + ".pattern"), enriched.pattern);
        assignIfPresent(getDouble(prefix + ".fold_enrichment"), enriched.foldEnrichment);
        assignIfPresent(getString(prefix + ".target_regions"), enriched.targetRegions);
        config.sequence.motifs.enriched.push_back(enriched);
    }

    assignIfPresent(getBool("genome.enable_genome_structure"), config.genome.enableGenomeStructure);
    assignIfPresent(getInt("genome.number_of_chromosomes"), config.genome.numberOfChromosomes);
    assignIfPresent(getBool("genome.include_autosomes"), config.genome.includeAutosomes);
    assignIfPresent(getBool("genome.include_sex_chromosomes"), config.genome.includeSexChromosomes);
    assignIfPresent(getBool("genome.include_mt"), config.genome.includeMt);
    assignIfPresent(getString("genome.centromere_model"), config.genome.centromereModel);
    assignIfPresent(getString("genome.telomere_repeat"), config.genome.telomereRepeat);
    assignIfPresent(getInt("genome.telomere_length_mean"), config.genome.telomereLengthMean);
    assignIfPresent(getBool("genome.masked_repeats"), config.genome.maskedRepeats);
    assignIfPresent(getDouble("genome.cpg_island_density"), config.genome.cpgIslandDensity);
    assignIfPresent(getDouble("genome.gene_density_per_mb"), config.genome.geneDensityPerMb);
    assignIfPresent(getInt("genome.contigs.min_gap_size"), config.genome.contigs.minGapSize);
    assignIfPresent(getInt("genome.contigs.max_gap_size"), config.genome.contigs.maxGapSize);
    assignIfPresent(getDouble("genome.contigs.n_fraction"), config.genome.contigs.nFraction);
    assignIfPresent(getDouble("genome.regions.conserved_fraction"), config.genome.regions.conservedFraction);
    assignIfPresent(getDouble("genome.regions.neutral_fraction"), config.genome.regions.neutralFraction);
    assignIfPresent(getDouble("genome.regions.selected_fraction"), config.genome.regions.selectedFraction);

    assignIfPresent(getBool("transcriptome.enable_transcriptome"), config.transcriptome.enableTranscriptome);
    assignIfPresent(getBool("transcriptome.derive_from_annotation"), config.transcriptome.deriveFromAnnotation);
    assignIfPresent(getInt("transcriptome.number_of_genes"), config.transcriptome.numberOfGenes);
    assignIfPresent(getString("transcriptome.transcripts_per_gene_model"), config.transcriptome.transcriptsPerGeneModel);
    assignIfPresent(getDouble("transcriptome.mean_transcripts_per_gene"), config.transcriptome.meanTranscriptsPerGene);
    assignIfPresent(getBool("transcriptome.strand_specific_genes"), config.transcriptome.strandSpecificGenes);
    assignIfPresent(getBool("transcriptome.allow_overlapping_genes"), config.transcriptome.allowOverlappingGenes);
    assignIfPresent(getString("transcriptome.gene_model.exon_count_model"), config.transcriptome.geneModel.exonCountModel);
    assignIfPresent(getDouble("transcriptome.gene_model.mean_exons_per_transcript"),
                    config.transcriptome.geneModel.meanExonsPerTranscript);
    assignIfPresent(getInt("transcriptome.gene_model.min_exons_per_transcript"),
                    config.transcriptome.geneModel.minExonsPerTranscript);
    assignIfPresent(getInt("transcriptome.gene_model.max_exons_per_transcript"),
                    config.transcriptome.geneModel.maxExonsPerTranscript);
    assignIfPresent(getString("transcriptome.gene_model.exon_length_model"), config.transcriptome.geneModel.exonLengthModel);
    assignIfPresent(getInt("transcriptome.gene_model.mean_exon_length"), config.transcriptome.geneModel.meanExonLength);
    assignIfPresent(getInt("transcriptome.gene_model.min_exon_length"), config.transcriptome.geneModel.minExonLength);
    assignIfPresent(getInt("transcriptome.gene_model.max_exon_length"), config.transcriptome.geneModel.maxExonLength);
    assignIfPresent(getString("transcriptome.gene_model.intron_length_model"), config.transcriptome.geneModel.intronLengthModel);
    assignIfPresent(getInt("transcriptome.gene_model.mean_intron_length"), config.transcriptome.geneModel.meanIntronLength);
    assignIfPresent(getInt("transcriptome.gene_model.min_intron_length"), config.transcriptome.geneModel.minIntronLength);
    assignIfPresent(getInt("transcriptome.gene_model.max_intron_length"), config.transcriptome.geneModel.maxIntronLength);
    assignIfPresent(getInt("transcriptome.gene_model.utr5_length_mean"), config.transcriptome.geneModel.utr5LengthMean);
    assignIfPresent(getInt("transcriptome.gene_model.utr3_length_mean"), config.transcriptome.geneModel.utr3LengthMean);
    assignIfPresent(getInt("transcriptome.gene_model.polyA_tail_length_mean"), config.transcriptome.geneModel.polyATailLengthMean);
    assignIfPresent(getInt("transcriptome.gene_model.polyA_tail_length_sd"), config.transcriptome.geneModel.polyATailLengthSd);
    assignIfPresent(getBool("transcriptome.splicing.enable_alternative_splicing"), config.transcriptome.splicing.enableAlternativeSplicing);
    assignIfPresent(getDouble("transcriptome.splicing.event_rate"), config.transcriptome.splicing.eventRate);
    assignIfPresent(getInt("transcriptome.splicing.max_isoforms_per_gene"), config.transcriptome.splicing.maxIsoformsPerGene);
    assignIfPresent(getDouble("transcriptome.splicing.event_fractions.exon_skipping"),
                    config.transcriptome.splicing.eventFractions.exonSkipping);
    assignIfPresent(getDouble("transcriptome.splicing.event_fractions.intron_retention"),
                    config.transcriptome.splicing.eventFractions.intronRetention);
    assignIfPresent(getDouble("transcriptome.splicing.event_fractions.alt_5prime"),
                    config.transcriptome.splicing.eventFractions.alt5Prime);
    assignIfPresent(getDouble("transcriptome.splicing.event_fractions.alt_3prime"),
                    config.transcriptome.splicing.eventFractions.alt3Prime);
    assignIfPresent(getDouble("transcriptome.splicing.event_fractions.mutually_exclusive_exons"),
                    config.transcriptome.splicing.eventFractions.mutuallyExclusiveExons);
    assignIfPresent(getDouble("transcriptome.splicing.event_fractions.alternative_first_exon"),
                    config.transcriptome.splicing.eventFractions.alternativeFirstExon);
    assignIfPresent(getDouble("transcriptome.splicing.event_fractions.alternative_last_exon"),
                    config.transcriptome.splicing.eventFractions.alternativeLastExon);
    assignIfPresent(getBool("transcriptome.rna_processing.enable_rna_editing"), config.transcriptome.rnaProcessing.enableRnaEditing);
    assignIfPresent(getDouble("transcriptome.rna_processing.rna_editing_rate"), config.transcriptome.rnaProcessing.rnaEditingRate);
    assignIfPresent(getBool("transcriptome.rna_processing.enable_fusion_transcripts"),
                    config.transcriptome.rnaProcessing.enableFusionTranscripts);
    assignIfPresent(getDouble("transcriptome.rna_processing.fusion_rate"), config.transcriptome.rnaProcessing.fusionRate);
    assignIfPresent(getDouble("transcriptome.rna_processing.premrna_fraction"), config.transcriptome.rnaProcessing.premrnaFraction);
    assignIfPresent(getDouble("transcriptome.rna_processing.intronic_fraction"), config.transcriptome.rnaProcessing.intronicFraction);
    assignIfPresent(getDouble("transcriptome.rna_processing.intergenic_fraction"),
                    config.transcriptome.rnaProcessing.intergenicFraction);
    assignIfPresent(getString("transcriptome.rna_processing.degradation_model"), config.transcriptome.rnaProcessing.degradationModel);
    assignIfPresent(getDouble("transcriptome.rna_processing.degradation_strength"),
                    config.transcriptome.rnaProcessing.degradationStrength);

    assignIfPresent(getBool("variation.enable_variation"), config.variation.enableVariation);
    assignIfPresent(getBool("variation.apply_germline_variants"), config.variation.applyGermlineVariants);
    assignIfPresent(getBool("variation.apply_somatic_variants"), config.variation.applySomaticVariants);
    assignIfPresent(getDouble("variation.heterozygosity_rate"), config.variation.heterozygosityRate);
    assignIfPresent(getBool("variation.phased"), config.variation.phased);
    assignIfPresent(getString("variation.placement.model"), config.variation.placement.model);
    assignIfPresent(getString("variation.placement.hotspot_bed"), config.variation.placement.hotspotBed);
    assignIfPresent(getDouble("variation.placement.exonic_weight"), config.variation.placement.exonicWeight);
    assignIfPresent(getDouble("variation.placement.intronic_weight"), config.variation.placement.intronicWeight);
    assignIfPresent(getDouble("variation.placement.intergenic_weight"), config.variation.placement.intergenicWeight);
    assignIfPresent(getDouble("variation.placement.cpg_weight"), config.variation.placement.cpgWeight);
    assignIfPresent(getDouble("variation.zygosity.heterozygous_fraction"), config.variation.zygosity.heterozygousFraction);
    assignIfPresent(getDouble("variation.zygosity.homozygous_fraction"), config.variation.zygosity.homozygousFraction);
    assignIfPresent(getDouble("variation.snv.rate"), config.variation.snv.rate);
    assignIfPresent(getDouble("variation.snv.transition_transversion_ratio"), config.variation.snv.transitionTransversionRatio);
    assignIfPresent(getBool("variation.snv.context_dependent"), config.variation.snv.contextDependent);
    assignIfPresent(getBool("variation.snv.context_model.trinucleotide"), config.variation.snv.contextModel.trinucleotide);
    assignIfPresent(getDouble("variation.snv.context_model.cpg_deamination_boost"), config.variation.snv.contextModel.cpgDeaminationBoost);
    assignIfPresent(getString("variation.snv.context_model.signature_profile"), config.variation.snv.contextModel.signatureProfile);
    assignIfPresent(getDouble("variation.indel.rate"), config.variation.indel.rate);
    assignIfPresent(getDouble("variation.indel.insertion_fraction"), config.variation.indel.insertionFraction);
    assignIfPresent(getDouble("variation.indel.deletion_fraction"), config.variation.indel.deletionFraction);
    assignIfPresent(getString("variation.indel.length_model"), config.variation.indel.lengthModel);
    assignIfPresent(getInt("variation.indel.mean_length"), config.variation.indel.meanLength);
    assignIfPresent(getInt("variation.indel.max_length"), config.variation.indel.maxLength);
    assignIfPresent(getDouble("variation.indel.context.homopolymer_enrichment"), config.variation.indel.context.homopolymerEnrichment);
    assignIfPresent(getDouble("variation.indel.context.microsatellite_enrichment"), config.variation.indel.context.microsatelliteEnrichment);
    assignIfPresent(getDouble("variation.indel.context.repeat_enrichment"), config.variation.indel.context.repeatEnrichment);
    assignIfPresent(getBool("variation.structural.enable"), config.variation.structural.enable);
    assignIfPresent(getDouble("variation.structural.rate"), config.variation.structural.rate);
    assignIfPresent(getDouble("variation.structural.deletion_fraction"), config.variation.structural.deletionFraction);
    assignIfPresent(getDouble("variation.structural.insertion_fraction"), config.variation.structural.insertionFraction);
    assignIfPresent(getDouble("variation.structural.duplication_fraction"), config.variation.structural.duplicationFraction);
    assignIfPresent(getDouble("variation.structural.inversion_fraction"), config.variation.structural.inversionFraction);
    assignIfPresent(getDouble("variation.structural.translocation_fraction"), config.variation.structural.translocationFraction);
    assignIfPresent(getDouble("variation.structural.cnv_fraction"), config.variation.structural.cnvFraction);
    assignIfPresent(getInt("variation.structural.min_size"), config.variation.structural.minSize);
    assignIfPresent(getInt("variation.structural.max_size"), config.variation.structural.maxSize);
    assignIfPresent(getString("variation.structural.breakpoints.model"), config.variation.structural.breakpoints.model);
    assignIfPresent(getInt("variation.structural.breakpoints.microhomology_mean"),
                    config.variation.structural.breakpoints.microhomologyMean);
    assignIfPresent(getDouble("variation.structural.breakpoints.repeat_breakpoint_enrichment"),
                    config.variation.structural.breakpoints.repeatBreakpointEnrichment);
    assignIfPresent(getBool("variation.copy_number.enable_aneuploidy"), config.variation.copyNumber.enableAneuploidy);
    assignIfPresent(getDouble("variation.copy_number.chromosome_gain_rate"), config.variation.copyNumber.chromosomeGainRate);
    assignIfPresent(getDouble("variation.copy_number.chromosome_loss_rate"), config.variation.copyNumber.chromosomeLossRate);
    assignIfPresent(getDouble("variation.copy_number.focal_cnv_rate"), config.variation.copyNumber.focalCnvRate);
    assignIfPresent(getBool("variation.somatic.enable"), config.variation.somatic.enable);
    assignIfPresent(getInt("variation.somatic.subclone_count"), config.variation.somatic.subcloneCount);
    assignIfPresent(getDouble("variation.somatic.tumor_purity"), config.variation.somatic.tumorPurity);
    assignIfPresent(getDouble("variation.somatic.mean_variant_allele_fraction"), config.variation.somatic.meanVariantAlleleFraction);
    assignIfPresent(getDouble("variation.somatic.vaf_dispersion"), config.variation.somatic.vafDispersion);
    assignIfPresent(getBool("variation.functional_effects.enable"), config.variation.functionalEffects.enable);
    assignIfPresent(getBool("variation.functional_effects.respect_coding_frame"), config.variation.functionalEffects.respectCodingFrame);
    assignIfPresent(getDouble("variation.functional_effects.splice_site_mutation_rate"),
                    config.variation.functionalEffects.spliceSiteMutationRate);
    assignIfPresent(getDouble("variation.functional_effects.nonsense_fraction"), config.variation.functionalEffects.nonsenseFraction);
    assignIfPresent(getDouble("variation.functional_effects.frameshift_fraction"), config.variation.functionalEffects.frameshiftFraction);
    assignIfPresent(getBool("variation.functional_effects.trigger_nmd_for_premature_stop"),
                    config.variation.functionalEffects.triggerNmdForPrematureStop);

    assignIfPresent(getBool("evolution.enable_evolution"), config.evolution.enableEvolution);
    assignIfPresent(getString("evolution.substitution_model"), config.evolution.substitutionModel);
    assignIfPresent(getString("evolution.indel_model"), config.evolution.indelModel);
    assignIfPresent(getDouble("evolution.branch_length_scale"), config.evolution.branchLengthScale);
    assignIfPresent(getBool("evolution.site_rate_heterogeneity"), config.evolution.siteRateHeterogeneity);
    assignIfPresent(getDouble("evolution.gamma_shape"), config.evolution.gammaShape);
    assignIfPresent(getDouble("evolution.proportion_invariant"), config.evolution.proportionInvariant);
    assignIfPresent(getInt("evolution.simulate_descendants"), config.evolution.simulateDescendants);

    assignIfPresent(getBool("expression.enable_expression"), config.expression.enableExpression);
    assignIfPresent(getString("expression.unit"), config.expression.unit);
    assignIfPresent(getString("expression.library_size_model"), config.expression.librarySizeModel);
    assignIfPresent(getInt("expression.mean_library_size"), config.expression.meanLibrarySize);
    assignIfPresent(getInt("expression.library_size_sd"), config.expression.librarySizeSd);
    assignIfPresent(getString("expression.abundance_model"), config.expression.abundanceModel);
    assignIfPresent(getString("expression.isoform_usage_model"), config.expression.isoformUsageModel);
    assignIfPresent(getDouble("expression.biological_cv"), config.expression.biologicalCv);
    assignIfPresent(getDouble("expression.technical_cv"), config.expression.technicalCv);
    assignIfPresent(getDouble("expression.baseline.housekeeping_fraction"), config.expression.baseline.housekeepingFraction);
    assignIfPresent(getDouble("expression.baseline.low_expression_fraction"), config.expression.baseline.lowExpressionFraction);
    assignIfPresent(getDouble("expression.baseline.medium_expression_fraction"), config.expression.baseline.mediumExpressionFraction);
    assignIfPresent(getDouble("expression.baseline.high_expression_fraction"), config.expression.baseline.highExpressionFraction);
    assignIfPresent(getBool("expression.differential_expression.enable"), config.expression.differentialExpression.enable);
    assignIfPresent(getDouble("expression.differential_expression.fraction_of_genes"),
                    config.expression.differentialExpression.fractionOfGenes);
    assignIfPresent(getDouble("expression.differential_expression.up_fraction"), config.expression.differentialExpression.upFraction);
    assignIfPresent(getDouble("expression.differential_expression.down_fraction"), config.expression.differentialExpression.downFraction);
    assignIfPresent(getString("expression.differential_expression.log2fc_model"), config.expression.differentialExpression.log2fcModel);
    assignIfPresent(getDouble("expression.differential_expression.mean_abs_log2fc"),
                    config.expression.differentialExpression.meanAbsLog2fc);
    assignIfPresent(getDouble("expression.differential_expression.sd_abs_log2fc"),
                    config.expression.differentialExpression.sdAbsLog2fc);
    assignIfPresent(getBool("expression.differential_transcript_usage.enable"), config.expression.differentialTranscriptUsage.enable);
    assignIfPresent(getDouble("expression.differential_transcript_usage.fraction_of_genes"),
                    config.expression.differentialTranscriptUsage.fractionOfGenes);
    assignIfPresent(getDouble("expression.differential_transcript_usage.mean_isoform_shift"),
                    config.expression.differentialTranscriptUsage.meanIsoformShift);
    assignIfPresent(getBool("expression.allele_specific.enable"), config.expression.alleleSpecific.enable);
    assignIfPresent(getDouble("expression.allele_specific.fraction_of_genes"), config.expression.alleleSpecific.fractionOfGenes);
    assignIfPresent(getDouble("expression.allele_specific.mean_major_allele_fraction"),
                    config.expression.alleleSpecific.meanMajorAlleleFraction);

    assignIfPresent(getStringArray("samples.conditions"), config.samples.conditions);
    assignIfPresent(getInt("samples.replicates_per_condition"), config.samples.replicatesPerCondition);
    assignIfPresent(getBool("samples.paired_design"), config.samples.pairedDesign);
    assignIfPresent(getInt("samples.batch_count"), config.samples.batchCount);
    assignIfPresent(getDouble("samples.batch_effect_sd"), config.samples.batchEffectSd);
    for (std::size_t index = 0;; ++index) {
        const std::string prefix = "samples.sample[" + std::to_string(index) + "]";
        if (!getString(prefix + ".sample_id").has_value()) {
            break;
        }

        ConfigReader::SamplesConfig::SampleConfig sample;
        assignIfPresent(getString(prefix + ".sample_id"), sample.sampleId);
        assignIfPresent(getString(prefix + ".condition"), sample.condition);
        assignIfPresent(getString(prefix + ".batch"), sample.batch);
        assignIfPresent(getString(prefix + ".sex"), sample.sex);
        config.samples.sample.push_back(sample);
    }

    assignIfPresent(getBool("sequencing.enable_read_simulation"), config.sequencing.enableReadSimulation);
    assignIfPresent(getBool("sequencing.simulate_dna_reads"), config.sequencing.simulateDnaReads);
    assignIfPresent(getBool("sequencing.simulate_rna_reads"), config.sequencing.simulateRnaReads);
    assignIfPresent(getString("sequencing.platform"), config.sequencing.platform);
    assignIfPresent(getString("sequencing.profile"), config.sequencing.profile);
    assignIfPresent(getString("sequencing.output_layout"), config.sequencing.outputLayout);
    assignIfPresent(getInt("sequencing.read_length"), config.sequencing.readLength);
    assignIfPresent(getInt("sequencing.read2_length"), config.sequencing.read2Length);
    assignIfPresent(getString("sequencing.fragmentation.fragment_length_model"), config.sequencing.fragmentation.fragmentLengthModel);
    assignIfPresent(getInt("sequencing.fragmentation.fragment_mean"), config.sequencing.fragmentation.fragmentMean);
    assignIfPresent(getInt("sequencing.fragmentation.fragment_sd"), config.sequencing.fragmentation.fragmentSd);
    assignIfPresent(getInt("sequencing.fragmentation.min_fragment_length"), config.sequencing.fragmentation.minFragmentLength);
    assignIfPresent(getInt("sequencing.fragmentation.max_fragment_length"), config.sequencing.fragmentation.maxFragmentLength);
    assignIfPresent(getDouble("sequencing.depth.dna_mean_coverage"), config.sequencing.depth.dnaMeanCoverage);
    assignIfPresent(getInt("sequencing.depth.rna_mean_read_count"), config.sequencing.depth.rnaMeanReadCount);
    assignIfPresent(getString("sequencing.depth.coverage_model"), config.sequencing.depth.coverageModel);
    assignIfPresent(getDouble("sequencing.depth.coverage_dispersion"), config.sequencing.depth.coverageDispersion);
    assignIfPresent(getBool("sequencing.depth.region_specific_coverage"), config.sequencing.depth.regionSpecificCoverage);
    assignIfPresent(getBool("sequencing.bias.enable_gc_bias"), config.sequencing.bias.enableGcBias);
    assignIfPresent(getDouble("sequencing.bias.gc_bias_strength"), config.sequencing.bias.gcBiasStrength);
    assignIfPresent(getBool("sequencing.bias.enable_positional_bias"), config.sequencing.bias.enablePositionalBias);
    assignIfPresent(getDouble("sequencing.bias.five_prime_bias"), config.sequencing.bias.fivePrimeBias);
    assignIfPresent(getDouble("sequencing.bias.three_prime_bias"), config.sequencing.bias.threePrimeBias);
    assignIfPresent(getBool("sequencing.bias.enable_sequence_context_bias"), config.sequencing.bias.enableSequenceContextBias);
    assignIfPresent(getBool("sequencing.bias.enable_pcr_duplication"), config.sequencing.bias.enablePcrDuplication);
    assignIfPresent(getDouble("sequencing.bias.pcr_duplicate_rate"), config.sequencing.bias.pcrDuplicateRate);
    assignIfPresent(getBool("sequencing.bias.enable_adapter_contamination"), config.sequencing.bias.enableAdapterContamination);
    assignIfPresent(getDouble("sequencing.bias.adapter_contamination_rate"), config.sequencing.bias.adapterContaminationRate);
    assignIfPresent(getDouble("sequencing.error_model.substitution_rate"), config.sequencing.errorModel.substitutionRate);
    assignIfPresent(getDouble("sequencing.error_model.insertion_rate"), config.sequencing.errorModel.insertionRate);
    assignIfPresent(getDouble("sequencing.error_model.deletion_rate"), config.sequencing.errorModel.deletionRate);
    assignIfPresent(getBool("sequencing.error_model.position_dependent"), config.sequencing.errorModel.positionDependent);
    assignIfPresent(getBool("sequencing.error_model.context_dependent"), config.sequencing.errorModel.contextDependent);
    assignIfPresent(getString("sequencing.error_model.quality_profile"), config.sequencing.errorModel.qualityProfile);
    assignIfPresent(getInt("sequencing.error_model.mean_phred"), config.sequencing.errorModel.meanPhred);
    assignIfPresent(getInt("sequencing.error_model.phred_sd"), config.sequencing.errorModel.phredSd);
    assignIfPresent(getDouble("sequencing.error_model.n_base_rate"), config.sequencing.errorModel.nBaseRate);
    assignIfPresent(getString("sequencing.paired_end.orientation"), config.sequencing.pairedEnd.orientation);
    assignIfPresent(getBool("sequencing.paired_end.mate_rescue"), config.sequencing.pairedEnd.mateRescue);
    assignIfPresent(getBool("sequencing.long_reads.enable_chimeras"), config.sequencing.longReads.enableChimeras);
    assignIfPresent(getDouble("sequencing.long_reads.chimera_rate"), config.sequencing.longReads.chimeraRate);
    assignIfPresent(getString("sequencing.long_reads.length_model"), config.sequencing.longReads.lengthModel);
    assignIfPresent(getInt("sequencing.long_reads.mean_read_length"), config.sequencing.longReads.meanReadLength);
    assignIfPresent(getInt("sequencing.long_reads.read_length_sd"), config.sequencing.longReads.readLengthSd);
    assignIfPresent(getInt("sequencing.long_reads.max_read_length"), config.sequencing.longReads.maxReadLength);
    assignIfPresent(getDouble("sequencing.long_reads.homopolymer_error_boost"), config.sequencing.longReads.homopolymerErrorBoost);
    assignIfPresent(getBool("sequencing.umi.enable"), config.sequencing.umi.enable);
    assignIfPresent(getInt("sequencing.umi.length"), config.sequencing.umi.length);
    assignIfPresent(getString("sequencing.umi.position"), config.sequencing.umi.position);

    assignIfPresent(getBool("targeted.enable"), config.targeted.enable);
    assignIfPresent(getString("targeted.mode"), config.targeted.mode);
    assignIfPresent(getDouble("targeted.on_target_rate"), config.targeted.onTargetRate);
    assignIfPresent(getDouble("targeted.off_target_rate"), config.targeted.offTargetRate);
    assignIfPresent(getDouble("targeted.uniformity"), config.targeted.uniformity);

    assignIfPresent(getBool("artifacts.enable"), config.artifacts.enable);
    assignIfPresent(getDouble("artifacts.cross_sample_contamination_rate"), config.artifacts.crossSampleContaminationRate);
    assignIfPresent(getBool("artifacts.adapter_dimers"), config.artifacts.adapterDimers);
    assignIfPresent(getDouble("artifacts.strand_bias"), config.artifacts.strandBias);
    assignIfPresent(getDouble("artifacts.reference_bias"), config.artifacts.referenceBias);
    assignIfPresent(getDouble("artifacts.background_contamination_rate"), config.artifacts.backgroundContaminationRate);

    assignIfPresent(getBool("truth.emit_truth_reference"), config.truth.emitTruthReference);
    assignIfPresent(getBool("truth.emit_truth_transcripts"), config.truth.emitTruthTranscripts);
    assignIfPresent(getBool("truth.emit_truth_gene_model"), config.truth.emitTruthGeneModel);
    assignIfPresent(getBool("truth.emit_truth_vcf"), config.truth.emitTruthVcf);
    assignIfPresent(getBool("truth.emit_truth_bed"), config.truth.emitTruthBed);
    assignIfPresent(getBool("truth.emit_truth_expression"), config.truth.emitTruthExpression);
    assignIfPresent(getBool("truth.emit_truth_isoform_usage"), config.truth.emitTruthIsoformUsage);
    assignIfPresent(getBool("truth.emit_truth_read_origin"), config.truth.emitTruthReadOrigin);
    assignIfPresent(getBool("truth.emit_truth_fusion_table"), config.truth.emitTruthFusionTable);
    assignIfPresent(getBool("truth.emit_phased_truth"), config.truth.emitPhasedTruth);
    assignIfPresent(getBool("truth.emit_functional_effects"), config.truth.emitFunctionalEffects);

    assignIfPresent(getString("output.directory"), config.output.directory);
    assignIfPresent(getString("output.prefix"), config.output.prefix);
    assignIfPresent(getBool("output.overwrite"), config.output.overwrite);
    assignIfPresent(getBool("output.compress"), config.output.compress);
    assignIfPresent(getBool("output.formats.fasta"), config.output.formats.fasta);
    assignIfPresent(getBool("output.formats.fastq"), config.output.formats.fastq);
    assignIfPresent(getBool("output.formats.gtf"), config.output.formats.gtf);
    assignIfPresent(getBool("output.formats.gff3"), config.output.formats.gff3);
    assignIfPresent(getBool("output.formats.vcf"), config.output.formats.vcf);
    assignIfPresent(getBool("output.formats.bed"), config.output.formats.bed);
    assignIfPresent(getBool("output.formats.tsv"), config.output.formats.tsv);
    assignIfPresent(getBool("output.formats.json_manifest"), config.output.formats.jsonManifest);
    assignIfPresent(getBool("output.naming.per_sample_subdirs"), config.output.naming.perSampleSubdirs);
    assignIfPresent(getBool("output.naming.include_condition_in_filename"), config.output.naming.includeConditionInFilename);
    assignIfPresent(getBool("output.naming.include_replicate_in_filename"), config.output.naming.includeReplicateInFilename);

    assignIfPresent(getBool("qc.emit_summary"), config.qc.emitSummary);
    assignIfPresent(getBool("qc.emit_plots"), config.qc.emitPlots);
    assignIfPresent(getBool("qc.kmer_spectra"), config.qc.kmerSpectra);
    assignIfPresent(getBool("qc.gc_distribution"), config.qc.gcDistribution);
    assignIfPresent(getBool("qc.coverage_histogram"), config.qc.coverageHistogram);
    assignIfPresent(getBool("qc.fragment_size_histogram"), config.qc.fragmentSizeHistogram);
    assignIfPresent(getBool("qc.error_profiles"), config.qc.errorProfiles);

    assignIfPresent(getBool("validation.strict"), config.validation.strict);
    assignIfPresent(getBool("validation.fail_on_missing_input"), config.validation.failOnMissingInput);
    assignIfPresent(getBool("validation.fail_on_unknown_keys"), config.validation.failOnUnknownKeys);
    assignIfPresent(getBool("validation.warn_on_unused_sections"), config.validation.warnOnUnusedSections);

    return config;
}

}  // namespace

bool ConfigReader::loadFromFile(const std::string& path, std::string& errorMessage) {
    values_.clear();
    config_ = Config{};

    std::ifstream input(path);
    if (!input) {
        errorMessage = "Unable to open config file: " + path;
        return false;
    }

    std::unordered_map<std::string, std::size_t> arrayTableIndices;
    std::string currentSection;
    std::string line;
    std::size_t lineNumber = 0;

    try {
        while (std::getline(input, line)) {
            ++lineNumber;

            const std::string withoutComments = trim(stripComments(line));
            if (withoutComments.empty()) {
                continue;
            }

            if (withoutComments.size() >= 4 && withoutComments.rfind("[[", 0) == 0 &&
                withoutComments.substr(withoutComments.size() - 2) == "]]") {
                const std::string sectionName = trim(withoutComments.substr(2, withoutComments.size() - 4));
                const std::size_t index = arrayTableIndices[sectionName]++;
                currentSection = sectionName + "[" + std::to_string(index) + "]";
                continue;
            }

            if (withoutComments.size() >= 2 && withoutComments.front() == '[' && withoutComments.back() == ']') {
                currentSection = trim(withoutComments.substr(1, withoutComments.size() - 2));
                continue;
            }

            const std::size_t assignmentPos = findAssignment(withoutComments);
            if (assignmentPos == std::string::npos) {
                throw std::runtime_error("missing '='");
            }

            const std::string key = trim(withoutComments.substr(0, assignmentPos));
            const std::string rawValue = trim(withoutComments.substr(assignmentPos + 1));
            const std::string fullKey = currentSection.empty() ? key : currentSection + "." + key;
            values_[fullKey] = parseValue(rawValue);
        }

        config_ = buildConfigFromValues(values_);
    } catch (const std::exception& ex) {
        std::ostringstream oss;
        oss << "Parse error on line " << lineNumber << ": " << ex.what();
        errorMessage = oss.str();
        values_.clear();
        config_ = Config{};
        return false;
    }

    errorMessage.clear();
    return true;
}

bool ConfigReader::has(const std::string& key) const {
    return values_.find(key) != values_.end();
}

std::optional<std::string> ConfigReader::getString(const std::string& key) const {
    return getAs<std::string>(values_, key);
}

std::optional<std::int64_t> ConfigReader::getInt(const std::string& key) const {
    return getAs<std::int64_t>(values_, key);
}

std::optional<double> ConfigReader::getDouble(const std::string& key) const {
    return getAs<double>(values_, key);
}

std::optional<bool> ConfigReader::getBool(const std::string& key) const {
    return getAs<bool>(values_, key);
}

std::optional<std::vector<std::string>> ConfigReader::getStringArray(const std::string& key) const {
    return getAs<std::vector<std::string>>(values_, key);
}

const ConfigReader::Config& ConfigReader::config() const {
    return config_;
}

ConfigReader::Summary ConfigReader::buildSummary() const {
    Summary summary;

    summary.version = config_.version;
    summary.runName = config_.run.name;
    summary.description = config_.run.description;
    summary.randomSeed = config_.run.randomSeed;
    summary.replicates = config_.run.replicates;
    summary.threads = config_.run.threads;
    summary.modes = config_.simulation.mode;
    summary.organismName = config_.simulation.organismName;
    summary.ploidy = config_.simulation.ploidy;
    summary.referenceFasta = config_.input.referenceFasta;
    summary.annotationGtf = config_.input.annotationGtf;
    summary.conditions = config_.samples.conditions;
    summary.sampleCount = config_.samples.sample.size();
    summary.platform = config_.sequencing.platform;
    summary.readLength = config_.sequencing.readLength;
    summary.outputDirectory = config_.output.directory;
    summary.outputPrefix = config_.output.prefix;
    summary.strictValidation = config_.validation.strict;

    return summary;
}

void ConfigReader::printSummary() const {
    Logger::info("Config summary");
    Logger::info("--------------");
    Logger::info("Loaded keys: " + std::to_string(values_.size()));

    std::vector<std::string> keys;
    keys.reserve(values_.size());
    for (const auto& [key, value] : values_) {
        (void)value;
        keys.push_back(key);
    }
    std::sort(keys.begin(), keys.end());

    for (const std::string& key : keys) {
        Logger::info(key + " = " + formatValue(values_.at(key)));
    }
}
