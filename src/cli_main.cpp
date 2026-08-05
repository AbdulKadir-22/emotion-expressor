#include "core/EmojiDatabase.hpp"
#include "core/EmojiSearch.hpp"
#include "utils/Config.hpp"
#include "utils/Logger.hpp"

#include <chrono>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    Logger::info("Starting Emoji Picker CLI Smoke Test (Phase 1)...");

    // Load configuration
    Config config;
    config.load();

    // Load database
    EmojiDatabase db;
    std::filesystem::path dbPath = "data/emojis.json";

    // If running from build dir or elsewhere, fallback check
    if (!std::filesystem::exists(dbPath) && std::filesystem::exists("../data/emojis.json")) {
        dbPath = "../data/emojis.json";
    }

    auto loadStart = std::chrono::high_resolution_clock::now();
    if (!db.load(dbPath)) {
        Logger::error("Failed to load emoji database from {}", dbPath.string());
        return 1;
    }
    auto loadEnd = std::chrono::high_resolution_clock::now();
    auto loadMs = std::chrono::duration_cast<std::chrono::milliseconds>(loadEnd - loadStart).count();

    std::cout << "========================================" << std::endl;
    std::cout << "Emoji Database Loaded Successfully!" << std::endl;
    std::cout << "Total Emojis: " << db.size() << std::endl;
    std::cout << "Load Time:    " << loadMs << " ms" << std::endl;
    std::cout << "========================================" << std::endl;

    auto runQuery = [&](const std::string& query) {
        auto searchStart = std::chrono::high_resolution_clock::now();
        auto results = EmojiSearch::search(db, query);
        auto searchEnd = std::chrono::high_resolution_clock::now();
        auto searchUs = std::chrono::duration_cast<std::chrono::microseconds>(searchEnd - searchStart).count();

        std::cout << "\nSearch query: '" << query << "' (" << results.size() << " results, "
                  << searchUs << " us)\n";
        std::cout << "--------------------------------------------------------\n";
        for (size_t i = 0; i < results.size() && i < 15; ++i) {
            const auto& res = results[i];
            std::cout << " " << (i + 1) << ". " << res.emoji->character << "  "
                      << res.emoji->name << " [Score: " << res.score << "]";
            if (!res.emoji->aliases.empty()) {
                std::cout << " (:" << res.emoji->aliases[0] << ":)";
            }
            std::cout << "\n";
        }
        std::cout << "--------------------------------------------------------\n";
    };

    if (argc > 1) {
        std::string query;
        for (int i = 1; i < argc; ++i) {
            if (i > 1) query += " ";
            query += argv[i];
        }
        runQuery(query);
        return 0;
    }

    std::cout << "\nEnter search query (or 'q' to quit):" << std::endl;
    std::string line;
    while (true) {
        std::cout << "\n> " << std::flush;
        if (!std::getline(std::cin, line) || line == "q" || line == "quit") {
            break;
        }
        if (line.empty()) continue;
        runQuery(line);
    }

    return 0;
}
