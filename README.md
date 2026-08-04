# Emotion Expressor 😃🔥🐱

A lightweight, native Linux emoji picker built in C++20 for GNOME/Wayland and Linux desktop environments.

## Features (Phase 1: Core Data Layer)

- **Pure C++20 Core Logic**: Decoupled core data layer with zero GUI dependencies.
- **Fast Startup & Dataset**: Loads **1,870 Unicode emojis** from a structured dataset in **~15 ms**.
- **Sub-Millisecond Ranked Search**: High-performance search engine returning ranked matches in **< 500 microseconds**.
- **Allocation-Conscious Design**: Precomputed lowercase search strings to prevent memory allocations per keystroke.
- **Multi-Tier Search Priority**:
  1. Exact Name / Character Match (`1000 pts`)
  2. Exact Alias Match e.g. `:smile:` (`950 pts`)
  3. Exact Keyword Match (`900 pts`)
  4. Prefix Match on Name (`800 pts`)
  5. Prefix Match on Alias (`700 pts`)
  6. Substring & Fuzzy Subsequence Matching (`100–500 pts`)
- **Config & Logging**:
  - Thread-safe leveled `Logger` (DEBUG, INFO, WARN, ERROR) supporting C++20 `std::format`.
  - JSON configuration manager storing shortcut bindings and recent emojis at `~/.config/emoji-picker/config.json`.
- **CLI Harness & Test Suite**: Included CLI smoke test harness and Catch2 unit test suite integrated with CTest.

---

## Project Structure

```text
├── CMakeLists.txt          # CMake build configuration (C++20, FetchContent nlohmann_json & Catch2)
├── data/
│   ├── emojis.json         # 1,870 unicode emoji dataset (Unicode CLDR / Gemoji MIT)
│   └── README.md           # Dataset documentation and licensing
├── docs/
│   ├── architecture.md     # Multi-phase system architecture
│   └── phase1_core.md      # Core data layer API documentation
├── src/
│   ├── core/
│   │   ├── Emoji.hpp / .cpp         # Emoji value type & json parser
│   │   ├── EmojiDatabase.hpp / .cpp # Emoji database loader & lookup maps
│   │   └── EmojiSearch.hpp / .cpp   # Multi-tier ranked search engine
│   ├── utils/
│   │   ├── Config.hpp / .cpp        # Config manager (~/.config/emoji-picker/config.json)
│   │   └── Logger.hpp / .cpp        # Leveled std::format logger
│   └── main.cpp            # CLI smoke test harness (interactive & query mode)
└── tests/                  # Catch2 unit test suite
    ├── test_database.cpp
    ├── test_search.cpp
    └── test_config.cpp
```

---

## Prerequisites

- **C++ Compiler**: GCC 13+ or Clang 16+ supporting C++20 (tested on GCC 16.1 / Fedora)
- **Build System**: CMake 3.20+ and Ninja or Make

---

## Building and Running

### 1. Build the Project

```bash
# Configure build directory
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Compile core library, CLI harness, and unit tests
cmake --build build
```

### 2. Run Unit Tests

```bash
ctest --test-dir build --output-on-failure
```

Or run the Catch2 test runner directly:

```bash
./build/emoji_tests
```

### 3. Run CLI Smoke Test

Single query execution:

```bash
./build/emoji_cli smile
./build/emoji_cli fire
./build/emoji_cli cat
./build/emoji_cli laugh
```

Interactive REPL mode:

```bash
./build/emoji_cli
```

---

## Roadmap

- [x] **Phase 1**: Core Data Layer (Dataset, Database, Ranked Search Engine, Config, Logger, CLI & Tests)
- [ ] **Phase 2**: GTK4 / gtkmm UI (Grid view, search bar, category navigation, recents)
- [ ] **Phase 3**: Global Shortcuts & Wayland Clipboard Integration (Portal / XDG Global Shortcuts)
- [ ] **Phase 4**: Packaging, Flatpak & Performance Optimization

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
The emoji dataset is derived from Unicode CLDR / GitHub Gemoji (MIT License).
