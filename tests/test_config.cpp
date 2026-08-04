#include <catch2/catch_test_macros.hpp>
#include "utils/Config.hpp"
#include <filesystem>

TEST_CASE("Config - Save and Load Round-trip", "[config]") {
    std::filesystem::path tempConfigPath = std::filesystem::temp_directory_path() / "emoji_picker_temp_config.json";

    // Clean up if exists
    if (std::filesystem::exists(tempConfigPath)) {
        std::filesystem::remove(tempConfigPath);
    }

    Config config1;
    config1.setShortcut("Super+E");
    config1.addRecentEmoji("😀");
    config1.addRecentEmoji("🔥");
    config1.addRecentEmoji("🐱");

    REQUIRE(config1.save(tempConfigPath));
    REQUIRE(std::filesystem::exists(tempConfigPath));

    Config config2;
    REQUIRE(config2.load(tempConfigPath));
    REQUIRE(config2.getShortcut() == "Super+E");

    const auto& recents = config2.getRecentEmojis();
    REQUIRE(recents.size() == 3);
    // Most recent added is "🐱"
    REQUIRE(recents[0] == "🐱");
    REQUIRE(recents[1] == "🔥");
    REQUIRE(recents[2] == "😀");

    // Test deduplication & recent limit
    config2.addRecentEmoji("🔥"); // Move 🔥 to front
    REQUIRE(config2.getRecentEmojis().size() == 3);
    REQUIRE(config2.getRecentEmojis()[0] == "🔥");

    std::filesystem::remove(tempConfigPath);
}
