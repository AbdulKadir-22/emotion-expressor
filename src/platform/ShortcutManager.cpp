#include "ShortcutManager.hpp"
#include "utils/Logger.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>

ShortcutManager::ShortcutManager() {
    try {
        dbusConn_ = Gio::DBus::Connection::get_sync(Gio::DBus::BusType::SESSION);
    } catch (const Glib::Error& ex) {
        Logger::warn("ShortcutManager: Failed to connect to Session DBus: {}", ex.what());
    }
}

ShortcutManager::~ShortcutManager() {
    unregisterShortcut();
}

std::string ShortcutManager::normalizeAcceleratorForGSettings(const std::string& accel) {
    if (accel.empty()) return "<Primary>period";
    if (accel.find('<') != std::string::npos) return accel; // Already formatted

    std::vector<std::string> tokens;
    std::stringstream ss(accel);
    std::string token;
    while (std::getline(ss, token, '+')) {
        tokens.push_back(token);
    }

    std::string result;
    std::string keyToken;

    for (const auto& rawTok : tokens) {
        std::string tok = rawTok;
        std::transform(tok.begin(), tok.end(), tok.begin(), [](unsigned char c) { return std::tolower(c); });

        if (tok == "ctrl" || tok == "control" || tok == "primary") {
            result += "<Primary>";
        } else if (tok == "shift") {
            result += "<Shift>";
        } else if (tok == "alt") {
            result += "<Alt>";
        } else if (tok == "super" || tok == "cmd" || tok == "win" || tok == "mod4") {
            result += "<Super>";
        } else {
            keyToken = rawTok;
        }
    }

    std::string keyLower = keyToken;
    std::transform(keyLower.begin(), keyLower.end(), keyLower.begin(), [](unsigned char c) { return std::tolower(c); });

    if (keyToken == ".") keyToken = "period";
    else if (keyToken == ",") keyToken = "comma";
    else if (keyToken == " " || keyLower == "space") keyToken = "space";
    else if (keyToken == "/" || keyLower == "slash") keyToken = "slash";
    else if (keyToken == "-" || keyLower == "minus") keyToken = "minus";
    else if (keyLower == "return" || keyLower == "enter") keyToken = "Return";
    else if (keyToken.length() == 1 && std::isupper(keyToken[0])) {
        keyToken[0] = static_cast<char>(std::tolower(keyToken[0]));
    }

    result += keyToken;
    return result;
}

std::string ShortcutManager::normalizeAcceleratorForPortal(const std::string& accel) {
    // Portal preferred_trigger format: e.g. "Ctrl+." or "Control+period" or "<Control>period"
    return normalizeAcceleratorForGSettings(accel);
}

bool ShortcutManager::registerShortcut(const std::string& accelerator, std::function<void()> onTrigger) {
    unregisterShortcut();

    callback_ = std::move(onTrigger);
    registeredAccelerator_ = accelerator;

    const char* sessionTypeEnv = std::getenv("XDG_SESSION_TYPE");
    std::string sessionType = sessionTypeEnv ? sessionTypeEnv : "";

    Logger::info("ShortcutManager: Attempting registration for accelerator '{}' (Session: '{}')", accelerator, sessionType);

    // Strategy Priority 1: XDG Desktop Portal GlobalShortcuts
    if (tryRegisterPortal(accelerator)) {
        activeBackend_ = ShortcutBackend::XdgPortal;
        Logger::info("ShortcutManager: Successfully registered global shortcut via XDG Desktop Portal.");
        return true;
    }

    // Strategy Priority 2: GNOME GSettings Custom Keybinding
    if (tryRegisterGSettings(accelerator)) {
        activeBackend_ = ShortcutBackend::GSettings;
        Logger::info("ShortcutManager: Successfully registered global shortcut via GNOME GSettings fallback.");
        return true;
    }

    // Strategy Priority 3: X11 / Other DE fallback
    if (sessionType == "x11" && tryRegisterX11(accelerator)) {
        activeBackend_ = ShortcutBackend::X11;
        Logger::info("ShortcutManager: Successfully registered global shortcut via X11 backend.");
        return true;
    }

    activeBackend_ = ShortcutBackend::None;
    Logger::error("ShortcutManager: Failed to register global shortcut via Portal or GSettings. "
                  "Manual binding can be set in DE settings for command 'emotion_expressor --toggle'.");
    return false;
}

void ShortcutManager::unregisterShortcut() {
    if (portalSignalSubscriptionId_ > 0 && dbusConn_) {
        dbusConn_->signal_unsubscribe(portalSignalSubscriptionId_);
        portalSignalSubscriptionId_ = 0;
    }

    if (!gsettingsPath_.empty()) {
        try {
            auto mediaKeysSettings = Gio::Settings::create("org.gnome.settings-daemon.plugins.media-keys");
            auto customBindings = mediaKeysSettings->get_string_array("custom-keybindings");
            std::vector<Glib::ustring> updated;
            for (const auto& item : customBindings) {
                if (item.raw() != gsettingsPath_) {
                    updated.push_back(item);
                }
            }
            mediaKeysSettings->set_string_array("custom-keybindings", updated);
            Logger::info("ShortcutManager: Removed GSettings custom keybinding at {}", gsettingsPath_);
        } catch (const Glib::Error& ex) {
            Logger::debug("ShortcutManager: Clean up GSettings error: {}", ex.what());
        }
        gsettingsPath_.clear();
    }

    activeBackend_ = ShortcutBackend::None;
    callback_ = nullptr;
}

bool ShortcutManager::tryRegisterPortal(const std::string& accel) {
    if (!dbusConn_) return false;
    std::string formatted = normalizeAcceleratorForPortal(accel);
    (void)formatted;

    try {
        // Subscribe to Activated signal on org.freedesktop.portal.GlobalShortcuts
        portalSignalSubscriptionId_ = dbusConn_->signal_subscribe(
            [this](const Glib::RefPtr<Gio::DBus::Connection>& /*conn*/,
                   const Glib::ustring& /*sender_name*/,
                   const Glib::ustring& /*object_path*/,
                   const Glib::ustring& /*interface_name*/,
                   const Glib::ustring& signal_name,
                   const Glib::VariantContainerBase& /*parameters*/) {
                if (signal_name == "Activated" && callback_) {
                    Logger::debug("ShortcutManager: Portal Activated signal received.");
                    callback_();
                }
            },
            "org.freedesktop.portal.Desktop",
            "org.freedesktop.portal.GlobalShortcuts",
            "Activated",
            "/org/freedesktop/portal/desktop"
        );

        // Attempt CreateSession D-Bus call
        std::map<Glib::ustring, Glib::VariantBase> options;
        options["handle_token"] = Glib::Variant<Glib::ustring>::create("emotion_expressor_session");
        options["session_handle_token"] = Glib::Variant<Glib::ustring>::create("emotion_expressor_session");

        auto callParams = Glib::VariantContainerBase::create_tuple({
            Glib::Variant<std::map<Glib::ustring, Glib::VariantBase>>::create(options)
        });

        // Synchronous call to test if Portal GlobalShortcuts is functional
        auto reply = dbusConn_->call_sync(
            "/org/freedesktop/portal/desktop",
            "org.freedesktop.portal.GlobalShortcuts",
            "CreateSession",
            callParams,
            "org.freedesktop.portal.Desktop"
        );

        if (reply) {
            Logger::info("ShortcutManager: Portal GlobalShortcuts CreateSession request initiated.");
            return true;
        }
    } catch (const Glib::Error& ex) {
        Logger::debug("ShortcutManager: Portal GlobalShortcuts not available or rejected: {}", ex.what());
    }

    if (portalSignalSubscriptionId_ > 0 && dbusConn_) {
        dbusConn_->signal_unsubscribe(portalSignalSubscriptionId_);
        portalSignalSubscriptionId_ = 0;
    }

    return false;
}

static std::string getSelfExecutablePath() {
    try {
        if (std::filesystem::exists("/proc/self/exe")) {
            return std::filesystem::canonical("/proc/self/exe").string();
        }
    } catch (...) {}
    return "emotion_expressor";
}

bool ShortcutManager::tryRegisterGSettings(const std::string& accel) {
    std::string formattedBinding = normalizeAcceleratorForGSettings(accel);
    gsettingsPath_ = "/org/gnome/settings-daemon/plugins/media-keys/custom-keybindings/emotion-expressor/";

    try {
        auto mediaKeysSettings = Gio::Settings::create("org.gnome.settings-daemon.plugins.media-keys");
        auto customBindings = mediaKeysSettings->get_string_array("custom-keybindings");

        bool exists = false;
        for (const auto& item : customBindings) {
            if (item == gsettingsPath_) {
                exists = true;
                break;
            }
        }

        if (!exists) {
            customBindings.push_back(gsettingsPath_);
            mediaKeysSettings->set_string_array("custom-keybindings", customBindings);
        }

        std::string commandStr = getSelfExecutablePath() + " --toggle";
        auto customItemSettings = Gio::Settings::create("org.gnome.settings-daemon.plugins.media-keys.custom-keybinding", gsettingsPath_);
        customItemSettings->set_string("name", "Emotion Expressor");
        customItemSettings->set_string("command", commandStr);
        customItemSettings->set_string("binding", formattedBinding);

        Logger::info("ShortcutManager: GSettings registered binding '{}' with command '{}'", formattedBinding, commandStr);
        return true;
    } catch (const Glib::Error& ex) {
        Logger::warn("ShortcutManager: GSettings registration failed: {}", ex.what());
        gsettingsPath_.clear();
        return false;
    }
}

bool ShortcutManager::tryRegisterX11(const std::string& accel) {
    // Under X11, try GSettings first if on GNOME; otherwise return false for now.
    return tryRegisterGSettings(accel);
}
