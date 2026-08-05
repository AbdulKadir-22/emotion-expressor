
# Emotion Expressor 😃🔥🐱

A lightweight, native Linux emoji picker built in C++20 for GNOME/Wayland and Linux desktop environments.

## Features

- **Pure C++20 Core Logic**: Decoupled core data layer with zero GUI dependencies.
- **Fast Startup & Dataset**: Loads **1,870 Unicode emojis** from a structured dataset in **~15 ms**.
- **Sub-Millisecond Ranked Search**: High-performance search engine returning ranked matches in **< 500 microseconds**.
- **Allocation-Conscious Design**: Precomputed lowercase search strings to prevent memory allocations per keystroke.
- **Persistent Background Daemon**: Runs continuously in background (`0.0%` idle CPU) and pops up instantly on global hotkey trigger (`Ctrl+.`).
- **Multi-Backend Global Shortcuts**: Multi-tier shortcut registration supporting **XDG Desktop Portal `GlobalShortcuts`** with GNOME **`gsettings` custom-keybinding** fallback.
- **Instant Focus & Dismissal**: Auto-focuses search entry on presentation and dismisses cleanly on `Esc` or focus loss.
- **Single-Instance D-Bus Activation**: Launching `emotion_expressor --toggle` signals existing daemon instance via D-Bus IPC.
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
- **Modern Dark Slate UI**: High-fidelity 7-column emoji tile grid with royal blue selection ring (`#2563eb`), 2D keyboard navigation (`Up/Down/Left/Right`), and dynamic footer preview card displaying selected emoji details and key command badges (`Esc Close`, `Enter Copy`).
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
│   ├── phase1_core.md      # Core data layer API documentation
│   ├── phase2_ui.md        # GTK4 / gtkmm UI layer API documentation
│   └── phase3_platform.md  # Background daemon & global shortcuts API documentation
├── src/
│   ├── app/                # GTK Application setup & CSS provider
│   ├── core/
│   │   ├── Emoji.hpp / .cpp         # Emoji value type & json parser
│   │   ├── EmojiDatabase.hpp / .cpp # Emoji database loader & lookup maps
│   │   └── EmojiSearch.hpp / .cpp   # Multi-tier ranked search engine
│   ├── platform/           # Platform integration (Clipboard, ShortcutManager, WindowManager)
│   ├── ui/                 # GTK4 UI components (MainWindow, SearchBar)
│   ├── utils/
│   │   ├── Config.hpp / .cpp        # Config manager (~/.config/emoji-picker/config.json)
│   │   └── Logger.hpp / .cpp        # Leveled std::format logger
│   ├── cli_main.cpp        # CLI smoke test harness (interactive & query mode)
│   └── main.cpp            # GTK4 application entry point
└── tests/                  # Catch2 unit test suite
    ├── test_database.cpp
    ├── test_search.cpp
    ├── test_config.cpp
    └── test_shortcuts.cpp
```

---

## Prerequisites

- **C++ Compiler**: GCC 13+ or Clang 16+ supporting C++20 (tested on GCC 16.1 / Fedora)
- **Build System**: CMake 3.20+ and Ninja or Make
- **Libraries**: `gtkmm-4.0`

---

## Building and Running

### 1. Build the Project

```bash
# Configure build directory
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Compile application daemon, CLI harness, and unit tests
cmake --build build
```

### 2. Run Background Daemon / Toggle Window

```bash
# Start background daemon (or toggle window visibility)
./build/emotion_expressor --toggle
```

### 3. Run Unit Tests

```bash
ctest --test-dir build --output-on-failure
```

Or run the Catch2 test runner directly:

```bash
./build/emoji_tests
```

### 4. Run CLI Smoke Test

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
- [x] **Phase 2**: GTK4 / gtkmm UI (Window layout, SearchBar, ListView, real-time filtering & clipboard copy)
- [x] **Phase 3**: Global Shortcuts & Persistent Background Daemon (XDG Portal / GSettings, WindowManager, D-Bus activation)
- [ ] **Phase 4**: Packaging, Flatpak & Performance Optimization

---

## License

This project is licensed under the **GNU General Public License v3.0 (GPL-3.0)** - see the [LICENSE](LICENSE) file for details.

The emoji dataset (`data/emojis.json`) is derived from Unicode CLDR / GitHub Gemoji and is licensed under the **MIT License**.

