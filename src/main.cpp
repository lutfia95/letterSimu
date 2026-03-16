#include "ConfigReader.hpp"

#include <iostream>
#include <string>

int main(int argc, char** argv) {
    const std::string configPath = argc > 1 ? argv[1] : "config.toml";

    ConfigReader reader;
    std::string errorMessage;
    if (!reader.loadFromFile(configPath, errorMessage)) {
        std::cerr << errorMessage << '\n';
        return 1;
    }

    reader.printSummary();
    return 0;
}
