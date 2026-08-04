#include "Emoji.hpp"
#include "utils/Logger.hpp"

#include <algorithm>
#include <cctype>

namespace {
std::string toLowerString(std::string_view sv) {
    std::string result;
    result.reserve(sv.size());
    for (char c : sv) {
        result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return result;
}
} // namespace

std::optional<Emoji> Emoji::fromJson(const nlohmann::json& j) {
    if (!j.is_object()) {
        Logger::warn("Emoji::fromJson: JSON element is not an object");
        return std::nullopt;
    }

    if (!j.contains("emoji") || !j["emoji"].is_string()) {
        Logger::warn("Emoji::fromJson: Missing or invalid 'emoji' field");
        return std::nullopt;
    }

    if (!j.contains("name") || !j["name"].is_string()) {
        Logger::warn("Emoji::fromJson: Missing or invalid 'name' field");
        return std::nullopt;
    }

    Emoji e;
    e.character = j["emoji"].get<std::string>();
    e.name = j["name"].get<std::string>();

    if (j.contains("unicode_name") && j["unicode_name"].is_string()) {
        e.unicodeName = j["unicode_name"].get<std::string>();
    } else {
        e.unicodeName = e.name;
    }

    if (j.contains("category") && j["category"].is_string()) {
        e.category = j["category"].get<std::string>();
    }

    if (j.contains("aliases") && j["aliases"].is_array()) {
        for (const auto& item : j["aliases"]) {
            if (item.is_string()) {
                e.aliases.push_back(item.get<std::string>());
            }
        }
    }

    if (j.contains("keywords") && j["keywords"].is_array()) {
        for (const auto& item : j["keywords"]) {
            if (item.is_string()) {
                e.keywords.push_back(item.get<std::string>());
            }
        }
    }

    // Precompute lowercase fields for fast search
    e.nameLower = toLowerString(e.name);
    e.unicodeNameLower = toLowerString(e.unicodeName);

    e.aliasesLower.reserve(e.aliases.size());
    for (const auto& alias : e.aliases) {
        e.aliasesLower.push_back(toLowerString(alias));
    }

    e.keywordsLower.reserve(e.keywords.size());
    for (const auto& kw : e.keywords) {
        e.keywordsLower.push_back(toLowerString(kw));
    }

    return e;
}
