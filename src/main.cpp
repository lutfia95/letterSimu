#include "ConfigReader.hpp"
#include "Logger.hpp"

#include <cstdlib>
#include <exception>
#include <string>

int main(int argc, char** argv) {
    const std::string configPath = argc > 1 ? argv[1] : "config.toml";
    Logger::initialize();

    if (const char* logLevel = std::getenv("LETTERSIMU_LOG_LEVEL")) {
        try {
            Logger::setLevel(logLevel);
        } catch (const std::exception& ex) {
            Logger::warn(std::string("Ignoring invalid LETTERSIMU_LOG_LEVEL: ") + ex.what());
        }
    }

    Logger::info("Loading configuration from " + configPath);

    ConfigReader reader;
    std::string errorMessage;
    if (!reader.loadFromFile(configPath, errorMessage)) {
        Logger::error(errorMessage);
        return 1;
    }

    Logger::info("Configuration loaded successfully");
    reader.printSummary();
    return 0;
}
