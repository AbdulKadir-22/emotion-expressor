#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

struct Emoji {
    std::string character;
    std::string name;
    std::string unicodeName;
    std::vector<std::string> aliases;
    std::vector<std::string> keywords;
    std::string category;

    // Precomputed lowercased fields for allocation-conscious search
    std::string nameLower;
    std::string unicodeNameLower;
    std::vector<std::string> aliasesLower;
    std::vector<std::string> keywordsLower;

    static std::optional<Emoji> fromJson(const nlohmann::json& json);

    bool operator==(const Emoji& other) const {
        return character == other.character;
    }
};
