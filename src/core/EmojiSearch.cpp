#include "EmojiSearch.hpp"

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

std::string_view stripColons(std::string_view sv) {
    if (sv.size() >= 2 && sv.front() == ':' && sv.back() == ':') {
        return sv.substr(1, sv.size() - 2);
    }
    if (sv.size() >= 1 && sv.front() == ':') {
        return sv.substr(1);
    }
    return sv;
}
} // namespace

bool EmojiSearch::isSubsequence(std::string_view needle, std::string_view haystack) {
    if (needle.empty()) return true;
    if (needle.size() > haystack.size()) return false;

    size_t needleIdx = 0;
    for (char c : haystack) {
        if (c == needle[needleIdx]) {
            needleIdx++;
            if (needleIdx == needle.size()) {
                return true;
            }
        }
    }
    return false;
}

int EmojiSearch::calculateScore(const Emoji& emoji, std::string_view queryLower) {
    if (queryLower.empty()) {
        return 0;
    }

    // Exact emoji character match
    if (emoji.character == queryLower) {
        return 1000;
    }

    std::string_view q = stripColons(queryLower);
    if (q.empty()) {
        return 0;
    }

    // 1. Exact name match
    if (emoji.nameLower == q) {
        return 1000;
    }

    // 2. Exact alias match
    for (const auto& alias : emoji.aliasesLower) {
        std::string_view a = stripColons(alias);
        if (a == q) {
            return 950;
        }
    }

    // 3. Exact keyword match
    for (const auto& kw : emoji.keywordsLower) {
        if (kw == q) {
            return 900;
        }
    }

    // 4. Prefix match on name
    if (emoji.nameLower.starts_with(q)) {
        return 800;
    }

    // 5. Prefix match on alias
    for (const auto& alias : emoji.aliasesLower) {
        std::string_view a = stripColons(alias);
        if (a.starts_with(q)) {
            return 700;
        }
    }

    // 6. Prefix match on keyword
    for (const auto& kw : emoji.keywordsLower) {
        if (kw.starts_with(q)) {
            return 600;
        }
    }

    // 7. Substring match on name
    if (emoji.nameLower.find(q) != std::string::npos) {
        return 500;
    }

    // 8. Substring match on alias
    for (const auto& alias : emoji.aliasesLower) {
        std::string_view a = stripColons(alias);
        if (a.find(q) != std::string_view::npos) {
            return 400;
        }
    }

    // 9. Substring match on keyword
    for (const auto& kw : emoji.keywordsLower) {
        if (kw.find(q) != std::string::npos) {
            return 300;
        }
    }

    // 10. Subsequence (fuzzy) match on name / alias / keyword (only for queries length >= 2)
    if (q.size() >= 2) {
        if (isSubsequence(q, emoji.nameLower)) {
            return 200;
        }
        for (const auto& alias : emoji.aliasesLower) {
            std::string_view a = stripColons(alias);
            if (isSubsequence(q, a)) {
                return 180;
            }
        }
        for (const auto& kw : emoji.keywordsLower) {
            if (isSubsequence(q, kw)) {
                return 150;
            }
        }
    }

    return 0;
}

std::vector<SearchResult> EmojiSearch::search(const EmojiDatabase& db,
                                               std::string_view query,
                                               size_t maxResults) {
    if (query.empty()) {
        return {};
    }

    std::string queryLower = toLowerString(query);
    std::vector<SearchResult> results;
    results.reserve(std::min(db.all().size(), size_t(100)));

    for (const auto& emoji : db.all()) {
        int score = calculateScore(emoji, queryLower);
        if (score > 0) {
            results.push_back({&emoji, score});
        }
    }

    std::sort(results.begin(), results.end(), std::greater<SearchResult>());

    if (results.size() > maxResults) {
        results.resize(maxResults);
    }

    return results;
}
