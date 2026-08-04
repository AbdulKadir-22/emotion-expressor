#pragma once

#include <filesystem>
#include <string>
#include <vector>

class Config {
public:
    static std::filesystem::path getDefaultPath();

    Config() = default;

    bool load(const std::filesystem::path& path = getDefaultPath());
    bool save(const std::filesystem::path& path = getDefaultPath()) const;

    const std::string& getShortcut() const { return shortcut_; }
    void setShortcut(const std::string& shortcut) { shortcut_ = shortcut; }

    const std::vector<std::string>& getRecentEmojis() const { return recentEmojis_; }
    void addRecentEmoji(const std::string& emojiChar, size_t maxCount = 20);
    void clearRecentEmojis() { recentEmojis_.clear(); }

private:
    std::string shortcut_{"Ctrl+."};
    std::vector<std::string> recentEmojis_;
};
