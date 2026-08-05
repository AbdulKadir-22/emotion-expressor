#pragma once

#include "Emoji.hpp"
#include "EmojiDatabase.hpp"

#include <string_view>
#include <vector>

struct SearchResult {
    const Emoji* emoji{nullptr};
    int score{0};

    bool operator>(const SearchResult& other) const {
        if (score != other.score) {
            return score > other.score;
        }
        // Tie-breaker: shorter name preferred
        return emoji->name.size() < other.emoji->name.size();
    }
};

class EmojiSearch {
public:
    static std::vector<SearchResult> search(const EmojiDatabase& db,
                                            std::string_view query,
                                            size_t maxResults = 50);

private:
    static int calculateScore(const Emoji& emoji, std::string_view queryLower);
    static bool isSubsequence(std::string_view needle, std::string_view haystack);
};
