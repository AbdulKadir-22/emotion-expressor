#include "Clipboard.hpp"
#include "utils/Logger.hpp"

#include <gdkmm.h>

bool Clipboard::copyText(const std::string& text) {
    auto display = Gdk::Display::get_default();
    if (!display) {
        Logger::error("Clipboard copy failed: Default Gdk::Display is null");
        return false;
    }

    auto clipboard = display->get_clipboard();
    if (!clipboard) {
        Logger::error("Clipboard copy failed: Gdk::Clipboard is null");
        return false;
    }

    clipboard->set_text(text);
    Logger::info("Copied emoji to clipboard: {}", text);
    return true;
}
