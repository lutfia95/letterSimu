#include "ConfigReader.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

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

}  // namespace

bool ConfigReader::loadFromFile(const std::string& path, std::string& errorMessage) {
    values_.clear();

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

            if (withoutComments.size() >= 4 && withoutComments.rfind("[[", 0) == 0 && withoutComments.substr(withoutComments.size() - 2) == "]]") {
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
    } catch (const std::exception& ex) {
        std::ostringstream oss;
        oss << "Parse error on line " << lineNumber << ": " << ex.what();
        errorMessage = oss.str();
        values_.clear();
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

ConfigReader::Summary ConfigReader::buildSummary() const {
    Summary summary;

    summary.version = getString("version").value_or("");
    summary.runName = getString("run.name").value_or("");
    summary.description = getString("run.description").value_or("");
    summary.randomSeed = getInt("run.random_seed").value_or(0);
    summary.replicates = getInt("run.replicates").value_or(0);
    summary.threads = getInt("run.threads").value_or(0);
    summary.modes = getStringArray("simulation.mode").value_or(std::vector<std::string>{});
    summary.organismName = getString("simulation.organism_name").value_or("");
    summary.ploidy = getInt("simulation.ploidy").value_or(0);
    summary.referenceFasta = getString("input.reference_fasta").value_or("");
    summary.annotationGtf = getString("input.annotation_gtf").value_or("");
    summary.conditions = getStringArray("samples.conditions").value_or(std::vector<std::string>{});
    summary.platform = getString("sequencing.platform").value_or("");
    summary.readLength = getInt("sequencing.read_length").value_or(0);
    summary.outputDirectory = getString("output.directory").value_or("");
    summary.outputPrefix = getString("output.prefix").value_or("");
    summary.strictValidation = getBool("validation.strict").value_or(false);

    for (std::size_t index = 0;; ++index) {
        const std::string prefix = "samples.sample[" + std::to_string(index) + "]";
        if (!has(prefix + ".sample_id")) {
            break;
        }
        ++summary.sampleCount;
    }

    return summary;
}

void ConfigReader::printSummary() const {
    const Summary summary = buildSummary();

    auto printList = [](const std::vector<std::string>& values) {
        for (std::size_t i = 0; i < values.size(); ++i) {
            if (i != 0) {
                std::cout << ", ";
            }
            std::cout << values[i];
        }
    };

    std::cout << "Config summary\n";
    std::cout << "--------------\n";
    std::cout << "Version: " << summary.version << '\n';
    std::cout << "Run name: " << summary.runName << '\n';
    std::cout << "Description: " << summary.description << '\n';
    std::cout << "Seed: " << summary.randomSeed << '\n';
    std::cout << "Replicates: " << summary.replicates << '\n';
    std::cout << "Threads: " << summary.threads << '\n';
    std::cout << "Modes: ";
    printList(summary.modes);
    std::cout << '\n';
    std::cout << "Organism: " << summary.organismName << '\n';
    std::cout << "Ploidy: " << summary.ploidy << '\n';
    std::cout << "Reference FASTA: " << summary.referenceFasta << '\n';
    std::cout << "Annotation GTF: " << summary.annotationGtf << '\n';
    std::cout << "Conditions: ";
    printList(summary.conditions);
    std::cout << '\n';
    std::cout << "Samples: " << summary.sampleCount << '\n';
    std::cout << "Sequencing platform: " << summary.platform << '\n';
    std::cout << "Read length: " << summary.readLength << '\n';
    std::cout << "Output directory: " << summary.outputDirectory << '\n';
    std::cout << "Output prefix: " << summary.outputPrefix << '\n';
    std::cout << "Strict validation: " << std::boolalpha << summary.strictValidation << '\n';
    std::cout << "Loaded keys: " << values_.size() << '\n';
}
