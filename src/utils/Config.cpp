#include "Config.hpp"
#include "Logger.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <nlohmann/json.hpp>

std::filesystem::path Config::getDefaultPath() {
    const char* home = std::getenv("HOME");
    if (home) {
        return std::filesystem::path(home) / ".config" / "emoji-picker" / "config.json";
    }
    return std::filesystem::current_path() / "config.json";
}

bool Config::load(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        Logger::info("Config::load: Config file does not exist at {}, using defaults", path.string());
        return false;
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        Logger::warn("Config::load: Failed to open config file at {}", path.string());
        return false;
    }

    nlohmann::json j;
    try {
        file >> j;
    } catch (const nlohmann::json::parse_error& e) {
        Logger::warn("Config::load: JSON parse error in {}: {}", path.string(), e.what());
        return false;
    }

    if (j.contains("shortcut") && j["shortcut"].is_string()) {
        shortcut_ = j["shortcut"].get<std::string>();
    }

    if (j.contains("recent_emojis") && j["recent_emojis"].is_array()) {
        recentEmojis_.clear();
        for (const auto& item : j["recent_emojis"]) {
            if (item.is_string()) {
                recentEmojis_.push_back(item.get<std::string>());
            }
        }
    }

    Logger::info("Config::load: Successfully loaded config from {}", path.string());
    return true;
}

bool Config::save(const std::filesystem::path& path) const {
    try {
        std::filesystem::create_directories(path.parent_path());
    } catch (const std::exception& e) {
        Logger::error("Config::save: Failed to create directories for {}: {}", path.string(), e.what());
        return false;
    }

    nlohmann::json j;
    j["shortcut"] = shortcut_;
    j["recent_emojis"] = recentEmojis_;

    std::ofstream file(path);
    if (!file.is_open()) {
        Logger::error("Config::save: Failed to open file for writing at {}", path.string());
        return false;
    }

    file << j.dump(2) << '\n';
    Logger::info("Config::save: Saved config to {}", path.string());
    return true;
}

void Config::addRecentEmoji(const std::string& emojiChar, size_t maxCount) {
    auto it = std::find(recentEmojis_.begin(), recentEmojis_.end(), emojiChar);
    if (it != recentEmojis_.end()) {
        recentEmojis_.erase(it);
    }
    recentEmojis_.insert(recentEmojis_.begin(), emojiChar);
    if (recentEmojis_.size() > maxCount) {
        recentEmojis_.resize(maxCount);
    }
}
