#pragma once

#include <memory>
#include <string>

namespace spdlog {
class logger;
namespace level {
enum level_enum : int;
}
}

class Logger {
public:
    static void initialize(const std::string& logFilePath = "lettersimu.log");
    static void setLevel(const std::string& levelName);

    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warn(const std::string& message);
    static void error(const std::string& message);

private:
    static std::shared_ptr<spdlog::logger> get();
    static spdlog::level::level_enum parseLevel(const std::string& levelName);
};
