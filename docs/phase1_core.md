# Phase 1: Core Data Layer API Documentation

## 1. `Emoji` (`src/core/Emoji.hpp`)

Represents a single emoji entry.

### Fields
- `character` (`std::string`): The UTF-8 emoji character (e.g. `"😀"`).
- `name` (`std::string`): Canonical descriptive name (e.g. `"grinning face"`).
- `unicodeName` (`std::string`): Standard uppercase Unicode name (e.g. `"GRINNING FACE"`).
- `aliases` (`std::vector<std::string>`): Shortcode aliases (e.g. `["grinning"]`).
- `keywords` (`std::vector<std::string>`): Search keywords (e.g. `["face", "smile", "happy"]`).
- `category` (`std::string`): Emoji category (e.g. `"Smileys & Emotion"`).

### Precomputed Lowercase Fields (Zero Allocation Search)
- `nameLower`, `unicodeNameLower`, `aliasesLower`, `keywordsLower`

### API Methods
```cpp
static std::optional<Emoji> Emoji::fromJson(const nlohmann::json& json);
```
Parses a JSON object into an `Emoji` struct. Logs warnings and returns `std::nullopt` for invalid entries.

---

## 2. `EmojiDatabase` (`src/core/EmojiDatabase.hpp`)

Manages dataset loading and in-memory indexing.

### API Methods
```cpp
bool load(const std::filesystem::path& path);
const std::vector<Emoji>& all() const;
const Emoji* findByAlias(std::string_view alias) const;
const Emoji* findByChar(std::string_view ch) const;
size_t size() const;
```

---

## 3. `EmojiSearch` (`src/core/EmojiSearch.hpp`)

Multi-tier ranked search engine.

### Data Types
```cpp
struct SearchResult {
    const Emoji* emoji;
    int score;
};
```

### API Methods
```cpp
static std::vector<SearchResult> search(const EmojiDatabase& db,
                                            std::string_view query,
                                            size_t maxResults = 50);
```

---

## 4. `Config` (`src/utils/Config.hpp`)

Handles configuration persistence at `~/.config/emoji-picker/config.json`.

### API Methods
```cpp
static std::filesystem::path getDefaultPath();
bool load(const std::filesystem::path& path = getDefaultPath());
bool save(const std::filesystem::path& path = getDefaultPath()) const;

const std::string& getShortcut() const;
void setShortcut(const std::string& shortcut);

const std::vector<std::string>& getRecentEmojis() const;
void addRecentEmoji(const std::string& emojiChar, size_t maxCount = 20);
```

---

## 5. `Logger` (`src/utils/Logger.hpp`)

Thread-safe leveled logging utility supporting C++20 `std::format`.

### API Methods
```cpp
static void setLevel(LogLevel level);
static void setLogFile(const std::filesystem::path& path);

Logger::debug("message {}", arg);
Logger::info("message {}", arg);
Logger::warn("message {}", arg);
Logger::error("message {}", arg);
```
