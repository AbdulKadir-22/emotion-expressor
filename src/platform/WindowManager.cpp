#include "WindowManager.hpp"
#include "utils/Logger.hpp"

#include <gdkmm.h>

WindowManager::WindowManager(MainWindow& window)
    : window_(window) {
    // Focus loss dismissal logic
    focusLossConnection_ = window_.property_is_active().signal_changed().connect([this]() {
        if (!window_.property_is_active().get_value()) {
            Logger::info("WindowManager: Window lost active focus, hiding.");
            hide();
        }
    });
}

void WindowManager::show() {
    centerOnActiveMonitor();
    window_.prepare_for_show();
    window_.present();
    Logger::info("WindowManager: Window presented and focused.");
}

void WindowManager::hide() {
    if (window_.get_visible()) {
        window_.hide();
        Logger::info("WindowManager: Window hidden.");
    }
}

void WindowManager::toggle() {
    if (window_.get_visible() && window_.property_is_active().get_value()) {
        hide();
    } else {
        show();
    }
}

bool WindowManager::isVisible() const {
    return window_.get_visible();
}

void WindowManager::centerOnActiveMonitor() {
    // GTK4 on Wayland relies on compositor window placement rules.
    // Calling present() centers top-level popup windows on active display monitor.
    auto display = Gdk::Display::get_default();
    if (display) {
        // Monitor hints / geometry verification if needed
    }
}
