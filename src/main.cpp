#include "ConfigReader.hpp"
#include "DnaReads.hpp"
#include "DnaReference.hpp"
#include "Logger.hpp"
#include "RnaReads.hpp"
#include "RnaTranscriptome.hpp"
#include "TargetParser.hpp"

#include <sharg/all.hpp>

#include <cstdlib>
#include <exception>
#include <memory>
#include <string>
#include <vector>

namespace {

struct CliOptions {
    std::string configPath{"config.toml"};
    std::string logLevel{};
    std::string target{};
    bool listTargets{false};
};

std::vector<std::unique_ptr<TargetParser>> makeTargetParsers() {
    std::vector<std::unique_ptr<TargetParser>> parsers;
    parsers.push_back(std::make_unique<DnaReference>());
    parsers.push_back(std::make_unique<RnaTranscriptome>());
    parsers.push_back(std::make_unique<DnaReads>());
    parsers.push_back(std::make_unique<RnaReads>());
    return parsers;
}

const TargetParser* findTargetParser(
    const std::vector<std::unique_ptr<TargetParser>>& parsers,
    const std::string& targetName
) {
    for (const auto& parser : parsers) {
        if (parser->name() == targetName) {
            return parser.get();
        }
    }

    return nullptr;
}

void printAvailableTargets(const std::vector<std::unique_ptr<TargetParser>>& parsers) {
    Logger::info("Available targets");
    for (const auto& parser : parsers) {
        Logger::info(std::string(" - ") + parser->name());
    }
}

void printTargetParams(const TargetParser& parser, const ConfigReader::Config& config) {
    Logger::info(std::string("Target parameters for ") + parser.name());
    for (const auto& param : parser.parseTargetParams(config)) {
        Logger::info(param.key + " = " + param.value);
    }
}

CliOptions parseCommandLine(int argc, char** argv) {
    CliOptions options;
    sharg::parser parser{"lettersimu", argc, argv};

    parser.info.author = "letterSimu";
    parser.info.short_description = "Load a TOML config and inspect target-specific parameters.";

    parser.add_option(options.configPath,
                      sharg::config{.short_id = 'c',
                                    .long_id = "config",
                                    .description = "Path to the TOML configuration file."});

    parser.add_option(options.logLevel,
                      sharg::config{.short_id = 'l',
                                    .long_id = "log-level",
                                    .description = "Override LETTERSIMU_LOG_LEVEL for this run."});

    parser.add_option(options.target,
                      sharg::config{.short_id = 't',
                                    .long_id = "target",
                                    .description = "Print derived parameters for one target parser."});

    parser.add_flag(options.listTargets,
                    sharg::config{.long_id = "list-targets",
                                  .description = "List available target parser names and exit."});

    parser.parse();
    return options;
}

}  // namespace

int main(int argc, char** argv) {
    Logger::initialize();

    CliOptions options;
    try {
        options = parseCommandLine(argc, argv);
    } catch (const std::exception& ex) {
        Logger::error(ex.what());
        return 1;
    }

    if (!options.logLevel.empty()) {
        try {
            Logger::setLevel(options.logLevel);
        } catch (const std::exception& ex) {
            Logger::warn(std::string("Ignoring invalid --log-level: ") + ex.what());
        }
    } else if (const char* logLevel = std::getenv("LETTERSIMU_LOG_LEVEL")) {
        try {
            Logger::setLevel(logLevel);
        } catch (const std::exception& ex) {
            Logger::warn(std::string("Ignoring invalid LETTERSIMU_LOG_LEVEL: ") + ex.what());
        }
    }

    const auto targetParsers = makeTargetParsers();

    if (options.listTargets) {
        printAvailableTargets(targetParsers);
        return 0;
    }

    Logger::info("Loading configuration from " + options.configPath);

    ConfigReader reader;
    std::string errorMessage;
    if (!reader.loadFromFile(options.configPath, errorMessage)) {
        Logger::error(errorMessage);
        return 1;
    }

    Logger::info("Configuration loaded successfully");

    if (!options.target.empty()) {
        const TargetParser* parser = findTargetParser(targetParsers, options.target);
        if (parser == nullptr) {
            Logger::error("Unknown target parser: " + options.target);
            printAvailableTargets(targetParsers);
            return 1;
        }

        printTargetParams(*parser, reader.config());
        return 0;
    }

    reader.printSummary();
    return 0;
}
