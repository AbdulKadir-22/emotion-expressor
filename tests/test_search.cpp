#include <catch2/catch_test_macros.hpp>
#include "core/EmojiDatabase.hpp"
#include "core/EmojiSearch.hpp"
#include <fstream>
#include <filesystem>

TEST_CASE("EmojiSearch - Query Ranking and Matches", "[search]") {
    std::filesystem::path samplePath = std::filesystem::temp_directory_path() / "emoji_picker_test_search.json";

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
                "emoji": "😄",
                "name": "grinning face with big eyes",
                "unicode_name": "GRINNING FACE WITH BIG EYES",
                "aliases": ["smile"],
                "keywords": ["face", "happy", "joy"],
                "category": "Smileys & Emotion"
            },
            {
                "emoji": "🐱",
                "name": "cat face",
                "unicode_name": "CAT FACE",
                "aliases": ["cat"],
                "keywords": ["animal", "pet"],
                "category": "Animals & Nature"
            },
            {
                "emoji": "🔥",
                "name": "fire",
                "unicode_name": "FIRE",
                "aliases": ["flame"],
                "keywords": ["hot"],
                "category": "Objects"
            },
            {
                "emoji": "😆",
                "name": "grinning squinting face",
                "unicode_name": "GRINNING SQUINTING FACE",
                "aliases": ["laughing", "satisfied"],
                "keywords": ["happy", "laugh"],
                "category": "Smileys & Emotion"
            }
        ])";
    }

    EmojiDatabase db;
    REQUIRE(db.load(samplePath));

    SECTION("Empty query returns empty results") {
        auto results = EmojiSearch::search(db, "");
        REQUIRE(results.empty());
    }

    SECTION("Known queries ('smile', 'fire', 'cat', 'laugh') return expected top result") {
        auto smileRes = EmojiSearch::search(db, "smile");
        REQUIRE_FALSE(smileRes.empty());
        // 'smile' is an alias for '😄' (score 950) vs keyword for '😀' (score 600)
        REQUIRE(smileRes[0].emoji->character == "😄");

        auto fireRes = EmojiSearch::search(db, "fire");
        REQUIRE_FALSE(fireRes.empty());
        REQUIRE(fireRes[0].emoji->character == "🔥");

        auto catRes = EmojiSearch::search(db, "cat");
        REQUIRE_FALSE(catRes.empty());
        REQUIRE(catRes[0].emoji->character == "🐱");

        auto laughRes = EmojiSearch::search(db, "laugh");
        REQUIRE_FALSE(laughRes.empty());
        REQUIRE(laughRes[0].emoji->character == "😆");
    }

    SECTION("Ranking order: exact name match beats keyword-only match") {
        // "fire" is exact name for 🔥
        auto fireRes = EmojiSearch::search(db, "fire");
        REQUIRE_FALSE(fireRes.empty());
        REQUIRE(fireRes[0].emoji->name == "fire");
        REQUIRE(fireRes[0].score == 1000);
    }

    SECTION("Alias search works with and without colons") {
        auto aliasNoColon = EmojiSearch::search(db, "grinning");
        REQUIRE_FALSE(aliasNoColon.empty());
        REQUIRE(aliasNoColon[0].emoji->character == "😀");

        auto aliasWithColon = EmojiSearch::search(db, ":grinning:");
        REQUIRE_FALSE(aliasWithColon.empty());
        REQUIRE(aliasWithColon[0].emoji->character == "😀");
    }

    std::filesystem::remove(samplePath);
}
