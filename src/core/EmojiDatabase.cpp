#include "EmojiDatabase.hpp"
#include "utils/Logger.hpp"

#include <chrono>
#include <fstream>
#include <nlohmann/json.hpp>

void EmojiDatabase::clear() {
    emojis_.clear();
    aliasMap_.clear();
    charMap_.clear();
}

bool EmojiDatabase::load(const std::filesystem::path& path) {
    clear();

    auto startTime = std::chrono::high_resolution_clock::now();

    if (!std::filesystem::exists(path)) {
        Logger::error("EmojiDatabase::load: File not found: {}", path.string());
        return false;
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        Logger::error("EmojiDatabase::load: Failed to open file: {}", path.string());
        return false;
    }

    nlohmann::json root;
    try {
        file >> root;
    } catch (const nlohmann::json::parse_error& e) {
        Logger::error("EmojiDatabase::load: JSON parse error in {}: {}", path.string(), e.what());
        return false;
    }

    if (!root.is_array()) {
        Logger::error("EmojiDatabase::load: Expected JSON array at root in {}", path.string());
        return false;
    }

    size_t skipped = 0;
    for (const auto& item : root) {
        auto emojiOpt = Emoji::fromJson(item);
        if (emojiOpt) {
            emojis_.push_back(std::move(*emojiOpt));
        } else {
            skipped++;
        }
    }

    rebuildIndices();

    auto endTime = std::chrono::high_resolution_clock::now();
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    Logger::info("EmojiDatabase: Loaded {} emojis (skipped {}) from {} in {} ms",
                 emojis_.size(), skipped, path.string(), elapsedMs);

    return !emojis_.empty();
}

void EmojiDatabase::rebuildIndices() {
    aliasMap_.clear();
    charMap_.clear();

    for (const auto& e : emojis_) {
        charMap_[e.character] = &e;

        for (const auto& alias : e.aliases) {
            aliasMap_[alias] = &e;
            // Also index with leading/trailing colons stripped if present
            if (!alias.empty() && alias.front() == ':' && alias.back() == ':') {
                aliasMap_[alias.substr(1, alias.size() - 2)] = &e;
            }
        }
    }
}

const Emoji* EmojiDatabase::findByAlias(std::string_view alias) const {
    std::string key(alias);
    if (!key.empty() && key.front() == ':' && key.back() == ':') {
        key = key.substr(1, key.size() - 2);
    }
    auto it = aliasMap_.find(key);
    if (it != aliasMap_.end()) {
        return it->second;
    }
    return nullptr;
}

const Emoji* EmojiDatabase::findByChar(std::string_view ch) const {
    auto it = charMap_.find(std::string(ch));
    if (it != charMap_.end()) {
        return it->second;
    }
    return nullptr;
}
