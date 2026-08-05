# Phase 3: Background Daemon & Global Shortcuts Documentation

## 1. Overview

Phase 3 implements persistent background daemon operation, system-wide global keyboard shortcuts, and instant window show/hide toggling for **Emotion Expressor**.

The app runs in the background as a lightweight single-instance process (`dev.emotion_expressor.EmojiPicker`) and pops up on demand when triggered by the global keyboard shortcut (default: `Ctrl+.`) or command-line activation (`emotion_expressor --toggle`).

---

## 2. Platform Components & Architecture

### `ShortcutManager` (`src/platform/ShortcutManager.hpp` / `.cpp`)

Responsible for registering configured shortcuts and handling global trigger callbacks.

#### Backend Priority Strategy:
1. **XDG Desktop Portal `GlobalShortcuts` (`org.freedesktop.portal.GlobalShortcuts`)**:
   - Modern, compositor-agnostic, sandbox/Flatpak-friendly Wayland hotkey registration over session D-Bus.
   - Listens for in-process `Activated` signals for sub-10ms response latency.
2. **GNOME `gsettings` Fallback (`org.gnome.settings-daemon.plugins.media-keys`)**:
   - Automatically resolves the canonical absolute path of the running executable via `/proc/self/exe`.
   - Registers custom keybindings under `/org/gnome/settings-daemon/plugins/media-keys/custom-keybindings/emotion-expressor/`.
   - Executes `/path/to/emotion_expressor --toggle` on key press, which D-Bus-activates the primary background instance cleanly even when uninstalled/running from a build folder.
3. **X11 Fallback (`XGrabKey`)**:
   - Active under X11 (`XDG_SESSION_TYPE == "x11"`).
4. **Graceful Degraded Mode**:
   - Logs diagnostic warnings if registration is unsupported on host DE; app remains operational via manual CLI binding (`emotion_expressor --toggle`).

#### Key Methods:
```cpp
bool registerShortcut(const std::string& accelerator, std::function<void()> onTrigger);
void unregisterShortcut();
ShortcutBackend getActiveBackend() const;

static std::string normalizeAcceleratorForGSettings(const std::string& accel);
static std::string normalizeAcceleratorForPortal(const std::string& accel);
```

---

### `WindowManager` (`src/platform/WindowManager.hpp` / `.cpp`)

Manages `MainWindow` visibility, keyboard focus grab, and focus-loss dismissal.

#### Key Responsibilities:
- **`show()`**: Resets search query, focuses search input, and presents window on active monitor.
- **`hide()`**: Hides `MainWindow` without terminating the background process.
- **`toggle()`**: Toggles between `show()` and `hide()`. Pressing shortcut while window is active hides it.
- **Focus Loss Dismissal**: Listens to `Gtk::Window::property_is_active().signal_changed()`. Automatically hides window when user clicks outside or switches application focus.

---

### `Application` Daemon Lifecycle (`src/app/Application.hpp` / `.cpp`)

- **Flags**: `Gio::Application::Flags::HANDLES_COMMAND_LINE`
- **Background Persistence**: Calls `hold()` on application startup to ensure process remains alive continuously when window is hidden.
- **D-Bus Single Instance Activation**: Invoking `emotion_expressor --toggle` sends command line arguments over D-Bus to the running primary process, executing `on_command_line()`.

---

## 3. Usage & Command Line Options

```bash
# Start background daemon (or toggle window if already running)
emotion_expressor --toggle

# Short flag alias
emotion_expressor -t
```
