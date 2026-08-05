#include <catch2/catch_test_macros.hpp>
#include "platform/ShortcutManager.hpp"

TEST_CASE("ShortcutManager - Accelerator Normalization for GSettings", "[shortcuts]") {
    SECTION("Normalizes Ctrl+. to <Primary>period") {
        std::string input = "Ctrl+.";
        std::string result = ShortcutManager::normalizeAcceleratorForGSettings(input);
        REQUIRE(result == "<Primary>period");
    }

    SECTION("Normalizes Control+period to <Primary>period") {
        std::string input = "Control+period";
        std::string result = ShortcutManager::normalizeAcceleratorForGSettings(input);
        REQUIRE(result == "<Primary>period");
    }

    SECTION("Normalizes Super+Space to <Super>space") {
        std::string input = "Super+Space";
        std::string result = ShortcutManager::normalizeAcceleratorForGSettings(input);
        REQUIRE(result == "<Super>space");
    }

    SECTION("Normalizes Ctrl+Shift+E to <Primary><Shift>e") {
        std::string input = "Ctrl+Shift+E";
        std::string result = ShortcutManager::normalizeAcceleratorForGSettings(input);
        REQUIRE(result == "<Primary><Shift>e");
    }

    SECTION("Preserves already formatted GSettings string <Primary>period") {
        std::string input = "<Primary>period";
        std::string result = ShortcutManager::normalizeAcceleratorForGSettings(input);
        REQUIRE(result == "<Primary>period");
    }
}
