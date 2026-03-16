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

    Summary buildSummary() const;
    void printSummary() const;

private:
    std::unordered_map<std::string, Value> values_;
};
