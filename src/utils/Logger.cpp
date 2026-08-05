#include "Logger.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_level = level;
}

void Logger::setLogFile(const std::filesystem::path& path) {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_logFilePath = path;
    if (s_fileStream.is_open()) {
        s_fileStream.close();
    }
    if (!s_logFilePath.empty()) {
        std::filesystem::create_directories(s_logFilePath.parent_path());
        s_fileStream.open(s_logFilePath, std::ios::app);
    }
}

std::string_view Logger::levelToString(LogLevel level) {
    switch (level) {
    case LogLevel::Debug:
        return "DEBUG";
    case LogLevel::Info:
        return "INFO";
    case LogLevel::Warn:
        return "WARN";
    case LogLevel::Error:
        return "ERROR";
    }
    return "UNKNOWN";
}

void Logger::log(LogLevel level, std::string_view msg) {
    std::lock_guard<std::mutex> lock(s_mutex);
    if (level < s_level) {
        return;
    }

    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tmBuf{};
    localtime_r(&timeT, &tmBuf);

    std::ostringstream ss;
    ss << std::put_time(&tmBuf, "%Y-%m-%d %H:%M:%S") << '.'
       << std::setfill('0') << std::setw(3) << ms.count()
       << " [" << levelToString(level) << "] " << msg << '\n';

    std::string formattedStr = ss.str();
    std::cerr << formattedStr;
    if (s_fileStream.is_open()) {
        s_fileStream << formattedStr;
        s_fileStream.flush();
    }
}
