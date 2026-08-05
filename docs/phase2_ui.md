# Phase 2: GTK4 / gtkmm-4.0 UI Documentation

## 1. Overview

Phase 2 introduces the native GTK4 GUI layer for **Emotion Expressor**, built with `gtkmm-4.0`. The UI features a high-fidelity dark slate interface matching modern design standards. Users can search emojis interactively, navigate a 7-column tile grid using 2D keyboard controls (`Left`, `Right`, `Up`, `Down`), inspect live emoji metadata in the dynamic preview footer card, and instantly copy selected emojis to the Wayland/X11 system clipboard.

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
  - Contains `SearchBar`, `Gtk::GridView` (7-column grid layout), and a dynamic `footerCard_`.
  - Uses `Gio::ListStore<EmojiItem>` and `Gtk::SingleSelection` for reactive grid tile rendering.
  - Highlights active emoji selection with a bright royal blue ring (`#2563eb`).
  - Dynamically updates footer preview card with selected emoji glyph, title name, and alias (`:smile:`).
  - Performs live ranked search filtering on keystroke changes.
  - Handles 2D keyboard navigation (`Left`, `Right`, `Up`, `Down`, `Return`, `Escape`).
  - Triggers clipboard copying upon selection and saves recently used emojis to configuration.

---

### SearchBar (`src/ui/SearchBar.hpp` / `.cpp`)
- **Base Class**: `Gtk::SearchEntry`
- **Key Responsibilities**:
  - Captures user query input with styled slate background (`#1b2131`), blue focus ring, and clear button.
  - Emits search signal updates to update results in real time.

---

### Footer Preview Card (`src/ui/MainWindow.cpp`)
- **Key Responsibilities**:
  - Displays selected emoji glyph (32px preview font).
  - Displays full emoji name (e.g. *Grinning Face With Smiling Eyes*) and alias shortcode (e.g. `:smile:`).
  - Displays styled key command badges (`Esc Close`, `Enter Copy`).

---

### Platform Clipboard (`src/platform/Clipboard.hpp` / `.cpp`)
- **Key Methods**:
  ```cpp
  static bool copyText(const std::string& text);
  ```
- **Responsibilities**:
  - Accesses standard `Gdk::Display` clipboard object.
  - Copies selected UTF-8 emoji string directly to system clipboard.
