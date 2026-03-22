#include "Logger.hpp"

#include <algorithm>
#include <cctype>
#include <mutex>
#include <stdexcept>
#include <vector>

#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace {

constexpr const char* kLoggerName = "lettersimu";

std::once_flag g_loggerInitFlag;

}  // namespace

void Logger::initialize(const std::string& logFilePath) {
    std::call_once(g_loggerInitFlag, [&logFilePath]() {
        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath, true));

        auto logger = std::make_shared<spdlog::logger>(kLoggerName, sinks.begin(), sinks.end());
        logger->set_level(spdlog::level::info);
        logger->flush_on(spdlog::level::info);
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

        spdlog::set_default_logger(logger);
    });
}

void Logger::setLevel(const std::string& levelName) {
    get()->set_level(parseLevel(levelName));
}

void Logger::debug(const std::string& message) {
    get()->debug(message);
}

void Logger::info(const std::string& message) {
    get()->info(message);
}

void Logger::warn(const std::string& message) {
    get()->warn(message);
}

void Logger::error(const std::string& message) {
    get()->error(message);
}

std::shared_ptr<spdlog::logger> Logger::get() {
    initialize();

    auto logger = spdlog::get(kLoggerName);
    if (!logger) {
        throw std::runtime_error("Logger is not initialized");
    }

    return logger;
}

spdlog::level::level_enum Logger::parseLevel(const std::string& levelName) {
    std::string normalized = levelName;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    if (normalized == "trace") {
        return spdlog::level::trace;
    }
    if (normalized == "debug") {
        return spdlog::level::debug;
    }
    if (normalized == "info") {
        return spdlog::level::info;
    }
    if (normalized == "warn" || normalized == "warning") {
        return spdlog::level::warn;
    }
    if (normalized == "error") {
        return spdlog::level::err;
    }
    if (normalized == "critical") {
        return spdlog::level::critical;
    }
    if (normalized == "off") {
        return spdlog::level::off;
    }

    throw std::invalid_argument("Unsupported log level: " + levelName);
}
