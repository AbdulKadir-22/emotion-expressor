#pragma once

#include "Emoji.hpp"

#include <filesystem>
#include <string_view>
#include <unordered_map>
#include <vector>

class EmojiDatabase {
public:
    EmojiDatabase() = default;

    bool load(const std::filesystem::path& path);
    const std::vector<Emoji>& all() const { return emojis_; }

    const Emoji* findByAlias(std::string_view alias) const;
    const Emoji* findByChar(std::string_view ch) const;

    size_t size() const { return emojis_.size(); }
    void clear();

private:
    std::vector<Emoji> emojis_;
    std::unordered_map<std::string, const Emoji*> aliasMap_;
    std::unordered_map<std::string, const Emoji*> charMap_;

    void rebuildIndices();
};
