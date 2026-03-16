#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

class ConfigReader {
public:
    using Value = std::variant<bool, std::int64_t, double, std::string, std::vector<std::string>>;

    struct RunConfig {
        std::string name;
        std::string description;
        std::int64_t randomSeed = 0;
        std::int64_t replicates = 0;
        std::int64_t threads = 0;
        bool deterministic = false;
        std::string project;
    };

    struct SimulationConfig {
        std::vector<std::string> mode;
        std::string organismName;
        std::int64_t ploidy = 0;
        bool propagateDnaVariantsToRna = false;
        bool transcribeFromMutatedGenome = false;
    };

    struct InputConfig {
        std::string referenceFasta;
        std::string annotationGtf;
        std::string transcriptFasta;
        std::string phylogenyNewick;
        std::string variantHotspotsBed;
        std::string targetRegionsBed;
        std::string knownSitesVcf;
        bool useExistingReference = false;
        bool useExistingAnnotation = false;
        bool generateDenovoSequences = false;
        bool mutateExistingReference = false;
        bool restrictToTargetRegions = false;
    };

    struct SequenceConfig {
        struct BaseCompositionConfig {
            double gcContent = 0.0;
            double atSkew = 0.0;
            double gcSkew = 0.0;
            bool enforceGlobalBaseFrequencies = false;
        };

        struct BaseFrequenciesConfig {
            double a = 0.0;
            double c = 0.0;
            double g = 0.0;
            double t = 0.0;
        };

        struct MotifConfig {
            struct EnrichedMotifConfig {
                std::string name;
                std::string pattern;
                double foldEnrichment = 0.0;
                std::string targetRegions;
            };

            bool enableMotifEnrichment = false;
            bool enableRepeatSimulation = false;
            double repeatFraction = 0.0;
            double lowComplexityFraction = 0.0;
            double homopolymerRate = 0.0;
            std::int64_t maxHomopolymerLength = 0;
            std::vector<EnrichedMotifConfig> enriched;
        };

        std::string alphabet;
        bool ambiguousBasesAllowed = false;
        std::int64_t numberOfSequences = 0;
        std::string namePrefix;
        std::string lengthModel;
        std::int64_t fixedLength = 0;
        std::int64_t minLength = 0;
        std::int64_t maxLength = 0;
        BaseCompositionConfig baseComposition;
        BaseFrequenciesConfig baseFrequencies;
        MotifConfig motifs;
    };

    struct GenomeConfig {
        struct ContigsConfig {
            std::int64_t minGapSize = 0;
            std::int64_t maxGapSize = 0;
            double nFraction = 0.0;
        };

        struct RegionsConfig {
            double conservedFraction = 0.0;
            double neutralFraction = 0.0;
            double selectedFraction = 0.0;
        };

        bool enableGenomeStructure = false;
        std::int64_t numberOfChromosomes = 0;
        bool includeAutosomes = false;
        bool includeSexChromosomes = false;
        bool includeMt = false;
        std::string centromereModel;
        std::string telomereRepeat;
        std::int64_t telomereLengthMean = 0;
        bool maskedRepeats = false;
        double cpgIslandDensity = 0.0;
        double geneDensityPerMb = 0.0;
        ContigsConfig contigs;
        RegionsConfig regions;
    };

    struct TranscriptomeConfig {
        struct GeneModelConfig {
            std::string exonCountModel;
            double meanExonsPerTranscript = 0.0;
            std::int64_t minExonsPerTranscript = 0;
            std::int64_t maxExonsPerTranscript = 0;
            std::string exonLengthModel;
            std::int64_t meanExonLength = 0;
            std::int64_t minExonLength = 0;
            std::int64_t maxExonLength = 0;
            std::string intronLengthModel;
            std::int64_t meanIntronLength = 0;
            std::int64_t minIntronLength = 0;
            std::int64_t maxIntronLength = 0;
            std::int64_t utr5LengthMean = 0;
            std::int64_t utr3LengthMean = 0;
            std::int64_t polyATailLengthMean = 0;
            std::int64_t polyATailLengthSd = 0;
        };

        struct SplicingConfig {
            struct EventFractionsConfig {
                double exonSkipping = 0.0;
                double intronRetention = 0.0;
                double alt5Prime = 0.0;
                double alt3Prime = 0.0;
                double mutuallyExclusiveExons = 0.0;
                double alternativeFirstExon = 0.0;
                double alternativeLastExon = 0.0;
            };

            bool enableAlternativeSplicing = false;
            double eventRate = 0.0;
            std::int64_t maxIsoformsPerGene = 0;
            EventFractionsConfig eventFractions;
        };

        struct RnaProcessingConfig {
            bool enableRnaEditing = false;
            double rnaEditingRate = 0.0;
            bool enableFusionTranscripts = false;
            double fusionRate = 0.0;
            double premrnaFraction = 0.0;
            double intronicFraction = 0.0;
            double intergenicFraction = 0.0;
            std::string degradationModel;
            double degradationStrength = 0.0;
        };

        bool enableTranscriptome = false;
        bool deriveFromAnnotation = false;
        std::int64_t numberOfGenes = 0;
        std::string transcriptsPerGeneModel;
        double meanTranscriptsPerGene = 0.0;
        bool strandSpecificGenes = false;
        bool allowOverlappingGenes = false;
        GeneModelConfig geneModel;
        SplicingConfig splicing;
        RnaProcessingConfig rnaProcessing;
    };

    struct VariationConfig {
        struct PlacementConfig {
            std::string model;
            std::string hotspotBed;
            double exonicWeight = 0.0;
            double intronicWeight = 0.0;
            double intergenicWeight = 0.0;
            double cpgWeight = 0.0;
        };

        struct ZygosityConfig {
            double heterozygousFraction = 0.0;
            double homozygousFraction = 0.0;
        };

        struct SnvConfig {
            struct ContextModelConfig {
                bool trinucleotide = false;
                double cpgDeaminationBoost = 0.0;
                std::string signatureProfile;
            };

            double rate = 0.0;
            double transitionTransversionRatio = 0.0;
            bool contextDependent = false;
            ContextModelConfig contextModel;
        };

        struct IndelConfig {
            struct ContextConfig {
                double homopolymerEnrichment = 0.0;
                double microsatelliteEnrichment = 0.0;
                double repeatEnrichment = 0.0;
            };

            double rate = 0.0;
            double insertionFraction = 0.0;
            double deletionFraction = 0.0;
            std::string lengthModel;
            std::int64_t meanLength = 0;
            std::int64_t maxLength = 0;
            ContextConfig context;
        };

        struct StructuralConfig {
            struct BreakpointsConfig {
                std::string model;
                std::int64_t microhomologyMean = 0;
                double repeatBreakpointEnrichment = 0.0;
            };

            bool enable = false;
            double rate = 0.0;
            double deletionFraction = 0.0;
            double insertionFraction = 0.0;
            double duplicationFraction = 0.0;
            double inversionFraction = 0.0;
            double translocationFraction = 0.0;
            double cnvFraction = 0.0;
            std::int64_t minSize = 0;
            std::int64_t maxSize = 0;
            BreakpointsConfig breakpoints;
        };

        struct CopyNumberConfig {
            bool enableAneuploidy = false;
            double chromosomeGainRate = 0.0;
            double chromosomeLossRate = 0.0;
            double focalCnvRate = 0.0;
        };

        struct SomaticConfig {
            bool enable = false;
            std::int64_t subcloneCount = 0;
            double tumorPurity = 0.0;
            double meanVariantAlleleFraction = 0.0;
            double vafDispersion = 0.0;
        };

        struct FunctionalEffectsConfig {
            bool enable = false;
            bool respectCodingFrame = false;
            double spliceSiteMutationRate = 0.0;
            double nonsenseFraction = 0.0;
            double frameshiftFraction = 0.0;
            bool triggerNmdForPrematureStop = false;
        };

        bool enableVariation = false;
        bool applyGermlineVariants = false;
        bool applySomaticVariants = false;
        double heterozygosityRate = 0.0;
        bool phased = false;
        PlacementConfig placement;
        ZygosityConfig zygosity;
        SnvConfig snv;
        IndelConfig indel;
        StructuralConfig structural;
        CopyNumberConfig copyNumber;
        SomaticConfig somatic;
        FunctionalEffectsConfig functionalEffects;
    };

    struct EvolutionConfig {
        bool enableEvolution = false;
        std::string substitutionModel;
        std::string indelModel;
        double branchLengthScale = 0.0;
        bool siteRateHeterogeneity = false;
        double gammaShape = 0.0;
        double proportionInvariant = 0.0;
        std::int64_t simulateDescendants = 0;
    };

    struct ExpressionConfig {
        struct BaselineConfig {
            double housekeepingFraction = 0.0;
            double lowExpressionFraction = 0.0;
            double mediumExpressionFraction = 0.0;
            double highExpressionFraction = 0.0;
        };

        struct DifferentialExpressionConfig {
            bool enable = false;
            double fractionOfGenes = 0.0;
            double upFraction = 0.0;
            double downFraction = 0.0;
            std::string log2fcModel;
            double meanAbsLog2fc = 0.0;
            double sdAbsLog2fc = 0.0;
        };

        struct DifferentialTranscriptUsageConfig {
            bool enable = false;
            double fractionOfGenes = 0.0;
            double meanIsoformShift = 0.0;
        };

        struct AlleleSpecificConfig {
            bool enable = false;
            double fractionOfGenes = 0.0;
            double meanMajorAlleleFraction = 0.0;
        };

        bool enableExpression = false;
        std::string unit;
        std::string librarySizeModel;
        std::int64_t meanLibrarySize = 0;
        std::int64_t librarySizeSd = 0;
        std::string abundanceModel;
        std::string isoformUsageModel;
        double biologicalCv = 0.0;
        double technicalCv = 0.0;
        BaselineConfig baseline;
        DifferentialExpressionConfig differentialExpression;
        DifferentialTranscriptUsageConfig differentialTranscriptUsage;
        AlleleSpecificConfig alleleSpecific;
    };

    struct SamplesConfig {
        struct SampleConfig {
            std::string sampleId;
            std::string condition;
            std::string batch;
            std::string sex;
        };

        std::vector<std::string> conditions;
        std::int64_t replicatesPerCondition = 0;
        bool pairedDesign = false;
        std::int64_t batchCount = 0;
        double batchEffectSd = 0.0;
        std::vector<SampleConfig> sample;
    };

    struct SequencingConfig {
        struct FragmentationConfig {
            std::string fragmentLengthModel;
            std::int64_t fragmentMean = 0;
            std::int64_t fragmentSd = 0;
            std::int64_t minFragmentLength = 0;
            std::int64_t maxFragmentLength = 0;
        };

        struct DepthConfig {
            double dnaMeanCoverage = 0.0;
            std::int64_t rnaMeanReadCount = 0;
            std::string coverageModel;
            double coverageDispersion = 0.0;
            bool regionSpecificCoverage = false;
        };

        struct BiasConfig {
            bool enableGcBias = false;
            double gcBiasStrength = 0.0;
            bool enablePositionalBias = false;
            double fivePrimeBias = 0.0;
            double threePrimeBias = 0.0;
            bool enableSequenceContextBias = false;
            bool enablePcrDuplication = false;
            double pcrDuplicateRate = 0.0;
            bool enableAdapterContamination = false;
            double adapterContaminationRate = 0.0;
        };

        struct ErrorModelConfig {
            double substitutionRate = 0.0;
            double insertionRate = 0.0;
            double deletionRate = 0.0;
            bool positionDependent = false;
            bool contextDependent = false;
            std::string qualityProfile;
            std::int64_t meanPhred = 0;
            std::int64_t phredSd = 0;
            double nBaseRate = 0.0;
        };

        struct PairedEndConfig {
            std::string orientation;
            bool mateRescue = false;
        };

        struct LongReadsConfig {
            bool enableChimeras = false;
            double chimeraRate = 0.0;
            std::string lengthModel;
            std::int64_t meanReadLength = 0;
            std::int64_t readLengthSd = 0;
            std::int64_t maxReadLength = 0;
            double homopolymerErrorBoost = 0.0;
        };

        struct UmiConfig {
            bool enable = false;
            std::int64_t length = 0;
            std::string position;
        };

        bool enableReadSimulation = false;
        bool simulateDnaReads = false;
        bool simulateRnaReads = false;
        std::string platform;
        std::string profile;
        std::string outputLayout;
        std::int64_t readLength = 0;
        std::int64_t read2Length = 0;
        FragmentationConfig fragmentation;
        DepthConfig depth;
        BiasConfig bias;
        ErrorModelConfig errorModel;
        PairedEndConfig pairedEnd;
        LongReadsConfig longReads;
        UmiConfig umi;
    };

    struct TargetedConfig {
        bool enable = false;
        std::string mode;
        double onTargetRate = 0.0;
        double offTargetRate = 0.0;
        double uniformity = 0.0;
    };

    struct ArtifactsConfig {
        bool enable = false;
        double crossSampleContaminationRate = 0.0;
        bool adapterDimers = false;
        double strandBias = 0.0;
        double referenceBias = 0.0;
        double backgroundContaminationRate = 0.0;
    };

    struct TruthConfig {
        bool emitTruthReference = false;
        bool emitTruthTranscripts = false;
        bool emitTruthGeneModel = false;
        bool emitTruthVcf = false;
        bool emitTruthBed = false;
        bool emitTruthExpression = false;
        bool emitTruthIsoformUsage = false;
        bool emitTruthReadOrigin = false;
        bool emitTruthFusionTable = false;
        bool emitPhasedTruth = false;
        bool emitFunctionalEffects = false;
    };

    struct OutputConfig {
        struct FormatsConfig {
            bool fasta = false;
            bool fastq = false;
            bool gtf = false;
            bool gff3 = false;
            bool vcf = false;
            bool bed = false;
            bool tsv = false;
            bool jsonManifest = false;
        };

        struct NamingConfig {
            bool perSampleSubdirs = false;
            bool includeConditionInFilename = false;
            bool includeReplicateInFilename = false;
        };

        std::string directory;
        std::string prefix;
        bool overwrite = false;
        bool compress = false;
        FormatsConfig formats;
        NamingConfig naming;
    };

    struct QcConfig {
        bool emitSummary = false;
        bool emitPlots = false;
        bool kmerSpectra = false;
        bool gcDistribution = false;
        bool coverageHistogram = false;
        bool fragmentSizeHistogram = false;
        bool errorProfiles = false;
    };

    struct ValidationConfig {
        bool strict = false;
        bool failOnMissingInput = false;
        bool failOnUnknownKeys = false;
        bool warnOnUnusedSections = false;
    };

    struct Config {
        std::string version;
        RunConfig run;
        SimulationConfig simulation;
        InputConfig input;
        SequenceConfig sequence;
        GenomeConfig genome;
        TranscriptomeConfig transcriptome;
        VariationConfig variation;
        EvolutionConfig evolution;
        ExpressionConfig expression;
        SamplesConfig samples;
        SequencingConfig sequencing;
        TargetedConfig targeted;
        ArtifactsConfig artifacts;
        TruthConfig truth;
        OutputConfig output;
        QcConfig qc;
        ValidationConfig validation;
    };

    struct Summary {
        std::string version;
        std::string runName;
        std::string description;
        std::int64_t randomSeed = 0;
        std::int64_t replicates = 0;
        std::int64_t threads = 0;
        std::vector<std::string> modes;
        std::string organismName;
        std::int64_t ploidy = 0;
        std::string referenceFasta;
        std::string annotationGtf;
        std::vector<std::string> conditions;
        std::size_t sampleCount = 0;
        std::string platform;
        std::int64_t readLength = 0;
        std::string outputDirectory;
        std::string outputPrefix;
        bool strictValidation = false;
    };

    bool loadFromFile(const std::string& path, std::string& errorMessage);

    bool has(const std::string& key) const;
    std::optional<std::string> getString(const std::string& key) const;
    std::optional<std::int64_t> getInt(const std::string& key) const;
    std::optional<double> getDouble(const std::string& key) const;
    std::optional<bool> getBool(const std::string& key) const;
    std::optional<std::vector<std::string>> getStringArray(const std::string& key) const;
    const Config& config() const;

    Summary buildSummary() const;
    void printSummary() const;

private:
    Config config_;
    std::unordered_map<std::string, Value> values_;
};
