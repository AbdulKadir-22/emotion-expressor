#include <catch2/catch_test_macros.hpp>
#include "core/EmojiDatabase.hpp"
#include <fstream>
#include <filesystem>

TEST_CASE("EmojiDatabase - Sample JSON Loading", "[database]") {
    std::filesystem::path samplePath = std::filesystem::temp_directory_path() / "emoji_picker_test_sample.json";
    
    // Create temporary sample JSON
    {
        std::ofstream out(samplePath);
        out << R"([
            {
                "emoji": "😀",
                "name": "grinning face",
                "unicode_name": "GRINNING FACE",
                "aliases": ["grinning"],
                "keywords": ["face", "smile", "happy"],
                "category": "Smileys & Emotion"
            },
            {
                "emoji": "🔥",
                "name": "fire",
                "unicode_name": "FIRE",
                "aliases": ["flame"],
                "keywords": ["hot", "cook"],
                "category": "Travel & Places"
            }
        ])";
    }

    EmojiDatabase db;
    REQUIRE(db.load(samplePath));
    REQUIRE(db.size() == 2);

    const auto* grinning = db.findByAlias("grinning");
    REQUIRE(grinning != nullptr);
    REQUIRE(grinning->character == "😀");
    REQUIRE(grinning->name == "grinning face");

    const auto* grinningWithColon = db.findByAlias(":grinning:");
    REQUIRE(grinningWithColon != nullptr);
    REQUIRE(grinningWithColon->character == "😀");

    const auto* fire = db.findByChar("🔥");
    REQUIRE(fire != nullptr);
    REQUIRE(fire->name == "fire");

    std::filesystem::remove(samplePath);
}

TEST_CASE("EmojiDatabase - Malformed File Handling", "[database]") {
    std::filesystem::path malformedPath = std::filesystem::temp_directory_path() / "emoji_picker_malformed.json";

    // Invalid JSON
    {
        std::ofstream out(malformedPath);
        out << "{ invalid json content }";
    }

    EmojiDatabase db;
    REQUIRE_FALSE(db.load(malformedPath));
    REQUIRE(db.size() == 0);

    // Non-existent file
    REQUIRE_FALSE(db.load(std::filesystem::temp_directory_path() / "emoji_picker_non_existent.json"));

    std::filesystem::remove(malformedPath);
}
