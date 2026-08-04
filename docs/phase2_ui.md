# Phase 2: GTK4 / gtkmm-4.0 UI Documentation

## 1. Overview

Phase 2 introduces the native GTK4 GUI layer for **Emotion Expressor**, built with `gtkmm-4.0`. The UI allows users to search emojis interactively as they type, navigate results using keyboard shortcuts or mouse selection, and instantly copy selected emojis to the Wayland/X11 system clipboard.

---

## 2. Architecture & Components

### Application (`src/app/Application.hpp` / `.cpp`)
- **Base Class**: `Gtk::Application`
- **Application ID**: `dev.emotion_expressor.EmojiPicker`
- **Responsibilities**:
  - Initializes logging and loads `Config` at startup.
  - Loads `EmojiDatabase` from `data/emojis.json`.
  - Injects custom CSS styling from `assets/styles/style.css`.
  - Instantiates and manages `MainWindow`.

---

### MainWindow (`src/ui/MainWindow.hpp` / `.cpp`)
- **Base Class**: `Gtk::ApplicationWindow`
- **Key Responsibilities**:
  - Contains `SearchBar` and a `Gtk::ListView` wrapped in a `Gtk::ScrolledWindow`.
  - Uses `Gio::ListStore<EmojiItem>` and `Gtk::SingleSelection` for low-latency reactive list rendering.
  - Performs live ranked search filtering on keystroke changes.
  - Handles keyboard navigation (`Up`, `Down`, `Return`, `Escape`).
  - Triggers clipboard copying upon selection and saves recently used emojis to configuration.

---

### SearchBar (`src/ui/SearchBar.hpp` / `.cpp`)
- **Base Class**: `Gtk::SearchEntry`
- **Key Responsibilities**:
  - Captures user query input with custom placeholder text (`"Search emojis (e.g. smile, fire)..."`).
  - Emits search signal updates to update results in real time.

---

### Platform Clipboard (`src/platform/Clipboard.hpp` / `.cpp`)
- **Key Methods**:
  ```cpp
  static bool copyText(const std::string& text);
  ```
- **Responsibilities**:
  - Accesses standard `Gdk::Display` clipboard object.
  - Copies selected UTF-8 emoji string directly to system clipboard.
