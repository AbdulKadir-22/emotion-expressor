#pragma once

#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>

enum class LogLevel { Debug, Info, Warn, Error };

class Logger {
public:
    static void setLevel(LogLevel level);
    static void setLogFile(const std::filesystem::path& path);
    static void log(LogLevel level, std::string_view msg);

    template <typename... Args>
    static void debug(std::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::Debug, std::format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    static void info(std::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::Info, std::format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    static void warn(std::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::Warn, std::format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    static void error(std::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::Error, std::format(fmt, std::forward<Args>(args)...));
    }

private:
    static inline LogLevel s_level{LogLevel::Info};
    static inline std::filesystem::path s_logFilePath{};
    static inline std::ofstream s_fileStream{};
    static inline std::mutex s_mutex{};

    static std::string_view levelToString(LogLevel level);
};
