# Emotion Expressor 😃🔥🐱

A lightweight, high-performance, native Linux emoji picker built in modern C++20 for Wayland and X11 desktop environments (GNOME, KDE, XFCE, Hyprland, etc.).

Designed to be instant, allocation-conscious, and seamless—Emotion Expressor runs silently in the background and presents a fast, searchable emoji picker on a global shortcut press (`Ctrl+.`) or CLI command.

---

## 🚀 Features

- **Blazing-Fast Search**: High-performance, multi-tier search engine returning ranked emoji matches in under **500 microseconds**.
- **Instant Launch & Low Footprint**: Loads **1,870+ Unicode emojis** in **~15 ms** with `0.0%` idle CPU usage while running in the background.
- **Allocation-Conscious C++20 Core**: Decoupled core data layer with precomputed lowercased search strings for zero-allocation keystroke evaluation.
- **Global Hotkey & Multi-Backend Shortcuts**: Supports **XDG Desktop Portal `GlobalShortcuts`** with seamless fallback to GNOME **`gsettings` custom-keybinding** and X11 `XGrabKey`.
- **Modern GTK4 / gtkmm-4 Interface**: Dark slate design with a 7-column tile grid, selection ring indicator (`#2563eb`), full 2D keyboard navigation (`Up`/`Down`/`Left`/`Right`), and dynamic footer preview card.
- **Instant Focus & Focus-Loss Dismissal**: Auto-focuses the search bar on presentation and hides immediately upon losing window focus, copying an emoji, or pressing `Esc`.
- **Single-Instance D-Bus IPC**: Launching `emotion_expressor --toggle` communicates via D-Bus to toggle the existing background process instantly.
- **Recent Emojis & Configuration**: Stores recently used emojis and custom settings locally at `~/.config/emoji-picker/config.json`.
- **CLI Mode & Unit Test Suite**: Includes a dedicated CLI search harness (`emoji_cli`) and Catch2 unit tests integrated with CTest.

---

## ⚡ Quick One-Line Installation

Install prebuilt release binaries without needing CMake, GCC/Clang, or build tools:

```bash
curl -fsSL https://raw.githubusercontent.com/AbdulKadir-22/emotion-expressor/main/install.sh | bash
```

### 🗑️ Uninstallation

To completely remove Emotion Expressor:

```bash
curl -fsSL https://raw.githubusercontent.com/AbdulKadir-22/emotion-expressor/main/uninstall.sh | bash
```

---

## 🛠️ Prerequisites & Runtime Requirements

### Prebuilt Binary Users
Prebuilt binaries run on any modern Linux distribution (Ubuntu 24.04+, Fedora 39+, Arch Linux, Debian 12+) with GTK4 runtime packages installed:
- **Ubuntu/Debian**: `sudo apt install libgtk-4-1`
- **Fedora/RHEL**: `sudo dnf install gtk4`
- **Arch Linux**: `sudo pacman -S gtk4`

### Building from Source
If building from source, ensure your system has C++20 compiler tools:
- **C++ Compiler**: GCC 13+ or Clang 16+ supporting C++20.
- **Build Tools**: CMake 3.20+ and Ninja (or Make).
- **GUI Toolkit**: `gtkmm-4.0` development packages.
- **IPC / D-Bus**: `gio-2.0` (included with GTK4/glib).

#### Installing Build Dependencies

##### Ubuntu / Debian (24.04+)
```bash
sudo apt update
sudo apt install build-essential cmake ninja-build libgtkmm-4.0-dev libjsoncpp-dev
```

##### Fedora / RHEL
```bash
sudo dnf install gcc-c++ cmake ninja-build gtkmm4.0-devel
```

##### Arch Linux
```bash
sudo pacman -S base-devel cmake ninja gtkmm4
```

---

## 📦 Building from Source

```bash
# Clone the repository
git clone https://github.com/AbdulKadir-22/emotion-expressor.git
cd emotion-expressor

# Configure the project with CMake
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Compile the application daemon, CLI tool, and tests
cmake --build build
```

### 2. Run the Application

```bash
# Start the background daemon / toggle the emoji picker window
./build/emotion_expressor --toggle
```

> **Note**: Press `Ctrl+.` (or run `./build/emotion_expressor --toggle`) at any time to open or close the picker window.

---

## 🧪 Testing & CLI Harness

### Running Unit Tests

Execute the test suite via CTest:

```bash
ctest --test-dir build --output-on-failure
```

Or run the Catch2 executable directly:

```bash
./build/emoji_tests
```

### Using the CLI Harness (`emoji_cli`)

Emotion Expressor includes a lightweight CLI binary for searching emojis from your terminal.

#### Query Mode
```bash
./build/emoji_cli smile
./build/emoji_cli fire
./build/emoji_cli cat
./build/emoji_cli rocket
```

#### Interactive REPL Mode
```bash
./build/emoji_cli
```

---

## 📁 Project Structure

```text
├── CMakeLists.txt                  # CMake build configuration
├── data/
│   ├── emojis.json                 # 1,870 Unicode CLDR / Gemoji dataset
│   └── README.md                   # Dataset licensing and details
├── docs/
│   ├── architecture.md             # System architecture & component interaction
│   └── files_and_dataflow.md       # Comprehensive file index & data flow guide
├── src/
│   ├── main.cpp                    # Application entry point
│   ├── cli_main.cpp                # Terminal CLI search harness
│   ├── app/                        # Gtk::Application daemon setup & CSS styling
│   ├── core/                       # Decoupled core logic (Emoji, Database, Search)
│   ├── ui/                         # GTK4 / gtkmm UI components (Window, SearchBar)
│   ├── platform/                   # System integration (Shortcuts, WindowManager, Clipboard)
│   └── utils/                      # Utilities (Config, Leveled Logger)
└── tests/                          # Catch2 unit test suite
```

For detailed architectural diagrams, file breakdowns, and data flow walkthroughs, refer to [`docs/architecture.md`](docs/architecture.md) and [`docs/files_and_dataflow.md`](docs/files_and_dataflow.md).

---

## 🏷️ Publishing a Release (Maintainers)

To release a new version of Emotion Expressor:

1. Tag a release commit with a version tag:
   ```bash
   git tag -a v0.1.0 -m "Release v0.1.0"
   git push origin v0.1.0
   ```
2. The **GitHub Actions release workflow** (`.github/workflows/release.yml`) will automatically:
   - Compile production binaries in `Release` mode on Ubuntu 24.04 runner.
   - Run Catch2 unit tests.
   - Package release archives (`emotion-expressor-v0.1.0-linux-x86_64.tar.gz`).
   - Generate SHA-256 checksums (`emotion-expressor-v0.1.0-linux-x86_64.tar.gz.sha256`).
   - Create a GitHub Release attached with assets.

---

## 📄 License

This project is licensed under the **GNU General Public License v3.0 (GPL-3.0)** - see the [LICENSE](LICENSE) file for details.

The emoji dataset (`data/emojis.json`) is derived from Unicode CLDR / GitHub Gemoji and is licensed under the **MIT License**.

