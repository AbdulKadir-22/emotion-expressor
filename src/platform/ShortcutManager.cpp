#include "ShortcutManager.hpp"
#include "utils/Logger.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <gio/gio.h>
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

namespace {
struct PortalResponse {
    uint32_t responseCode{1};
    std::map<Glib::ustring, Glib::VariantBase> results;
    bool received{false};
};

bool waitForPortalResponse(
    const Glib::RefPtr<Gio::DBus::Connection>& conn,
    const Glib::ustring& requestPath,
    PortalResponse& responseOut,
    guint timeoutMs = 15000)
{
    if (!conn || requestPath.empty()) {
        return false;
    }

    auto loop = Glib::MainLoop::create();
    guint signalSubId = 0;

    signalSubId = conn->signal_subscribe(
        [&](const Glib::RefPtr<Gio::DBus::Connection>& /*conn*/,
            const Glib::ustring& /*sender_name*/,
            const Glib::ustring& /*object_path*/,
            const Glib::ustring& /*interface_name*/,
            const Glib::ustring& signal_name,
            const Glib::VariantContainerBase& parameters) {
            if (signal_name == "Response") {
                try {
                    Glib::Variant<uint32_t> codeVar;
                    parameters.get_child(codeVar, 0);
                    responseOut.responseCode = codeVar.get();

                    Glib::Variant<std::map<Glib::ustring, Glib::VariantBase>> resultsVar;
                    parameters.get_child(resultsVar, 1);
                    responseOut.results = resultsVar.get();

                    responseOut.received = true;
                } catch (const std::exception& ex) {
                    Logger::warn("ShortcutManager: Error parsing Portal Response signal: {}", ex.what());
                }
                if (loop && loop->is_running()) {
                    loop->quit();
                }
            }
        },
        "org.freedesktop.portal.Desktop",
        "org.freedesktop.portal.Request",
        "Response",
        requestPath
    );

    auto timeoutConn = Glib::MainContext::get_default()->signal_timeout().connect(
        [&]() -> bool {
            if (loop && loop->is_running()) {
                Logger::warn("ShortcutManager: Portal Response timed out after {} ms", timeoutMs);
                loop->quit();
            }
            return false;
        },
        timeoutMs
    );

    loop->run();

    if (signalSubId > 0) {
        conn->signal_unsubscribe(signalSubId);
    }
    timeoutConn.disconnect();

    return responseOut.received && (responseOut.responseCode == 0);
}
} // anonymous namespace

bool ShortcutManager::tryRegisterPortal(const std::string& accel) {
    if (!dbusConn_) return false;
    std::string formatted = normalizeAcceleratorForPortal(accel);

    try {
        // 1. CreateSession
        std::map<Glib::ustring, Glib::VariantBase> createOptions;
        static int sessionCounter = 0;
        std::string token = "clipmoji_session_" + std::to_string(++sessionCounter);
        createOptions["handle_token"] = Glib::Variant<Glib::ustring>::create(token);
        createOptions["session_handle_token"] = Glib::Variant<Glib::ustring>::create(token);

        std::vector<Glib::VariantBase> createArgs;
        createArgs.push_back(Glib::Variant<std::map<Glib::ustring, Glib::VariantBase>>::create(createOptions));
        auto createParams = Glib::VariantContainerBase::create_tuple(createArgs);

        auto createReply = dbusConn_->call_sync(
            "/org/freedesktop/portal/desktop",
            "org.freedesktop.portal.GlobalShortcuts",
            "CreateSession",
            createParams,
            "org.freedesktop.portal.Desktop"
        );

        if (!createReply) {
            Logger::warn("ShortcutManager: CreateSession D-Bus call returned null");
            return false;
        }

        Glib::Variant<Glib::ustring> createReqPathVar;
        createReply.get_child(createReqPathVar, 0);
        Glib::ustring createRequestPath = createReqPathVar.get();

        PortalResponse createResp;
        if (!waitForPortalResponse(dbusConn_, createRequestPath, createResp, 15000)) {
            Logger::warn("ShortcutManager: CreateSession failed or timed out (responseCode={})", createResp.responseCode);
            return false;
        }

        auto it = createResp.results.find("session_handle");
        if (it == createResp.results.end()) {
            Logger::warn("ShortcutManager: CreateSession response missing session_handle");
            return false;
        }

        Glib::ustring sessionHandle;
        if (it->second.is_of_type(Glib::VariantType(G_VARIANT_TYPE_STRING)) ||
            it->second.is_of_type(Glib::VariantType(G_VARIANT_TYPE_OBJECT_PATH))) {
            const char* str = g_variant_get_string(it->second.gobj(), nullptr);
            if (str) {
                sessionHandle = str;
            }
        }

        if (sessionHandle.empty()) {
            Logger::warn("ShortcutManager: Session handle extracted was empty");
            return false;
        }

        Logger::info("ShortcutManager: Portal CreateSession succeeded. Session handle: {}", sessionHandle.raw());

        // 2. BindShortcuts
        GVariantBuilder propsBuilder;
        g_variant_builder_init(&propsBuilder, G_VARIANT_TYPE("a{sv}"));
        g_variant_builder_add(&propsBuilder, "{sv}", "description", g_variant_new_string("Toggle Emoji Picker"));
        g_variant_builder_add(&propsBuilder, "{sv}", "preferred_trigger", g_variant_new_string(formatted.c_str()));

        GVariantBuilder shortcutsBuilder;
        g_variant_builder_init(&shortcutsBuilder, G_VARIANT_TYPE("a(sa{sv})"));
        g_variant_builder_add(&shortcutsBuilder, "(sa{sv})", "toggle", g_variant_builder_end(&propsBuilder));

        GVariant* shortcutsGVar = g_variant_builder_end(&shortcutsBuilder);

        std::vector<Glib::VariantBase> bindArgs;
        bindArgs.push_back(Glib::VariantBase(g_variant_new_object_path(sessionHandle.c_str())));
        bindArgs.push_back(Glib::VariantBase(shortcutsGVar));
        bindArgs.push_back(Glib::Variant<Glib::ustring>::create("")); // parent_window

        std::map<Glib::ustring, Glib::VariantBase> bindOptions;
        static int bindCounter = 0;
        bindOptions["handle_token"] = Glib::Variant<Glib::ustring>::create("clipmoji_bind_" + std::to_string(++bindCounter));
        bindArgs.push_back(Glib::Variant<std::map<Glib::ustring, Glib::VariantBase>>::create(bindOptions));

        auto bindParams = Glib::VariantContainerBase::create_tuple(bindArgs);

        auto bindReply = dbusConn_->call_sync(
            "/org/freedesktop/portal/desktop",
            "org.freedesktop.portal.GlobalShortcuts",
            "BindShortcuts",
            bindParams,
            "org.freedesktop.portal.Desktop"
        );

        if (!bindReply) {
            Logger::warn("ShortcutManager: BindShortcuts D-Bus call returned null");
            return false;
        }

        Glib::Variant<Glib::ustring> bindReqPathVar;
        bindReply.get_child(bindReqPathVar, 0);
        Glib::ustring bindRequestPath = bindReqPathVar.get();

        PortalResponse bindResp;
        if (!waitForPortalResponse(dbusConn_, bindRequestPath, bindResp, 15000)) {
            Logger::warn("ShortcutManager: BindShortcuts failed or timed out (responseCode={})", bindResp.responseCode);
            return false;
        }

        Logger::info("ShortcutManager: Portal BindShortcuts succeeded for shortcut 'toggle' with trigger '{}'.", formatted);

        // 3. Activated Signal Subscription filtered by sessionHandle
        portalSessionHandle_ = sessionHandle;

        portalSignalSubscriptionId_ = dbusConn_->signal_subscribe(
            [this](const Glib::RefPtr<Gio::DBus::Connection>& /*conn*/,
                   const Glib::ustring& /*sender_name*/,
                   const Glib::ustring& /*object_path*/,
                   const Glib::ustring& /*interface_name*/,
                   const Glib::ustring& signal_name,
                   const Glib::VariantContainerBase& parameters) {
                if (signal_name == "Activated" && callback_) {
                    try {
                        Glib::Variant<Glib::ustring> sessVar;
                        parameters.get_child(sessVar, 0);

                        Glib::Variant<Glib::ustring> shortcutVar;
                        parameters.get_child(shortcutVar, 1);

                        if (sessVar.get() == portalSessionHandle_ && shortcutVar.get() == "toggle") {
                            Logger::debug("ShortcutManager: Portal Activated signal received for session.");
                            callback_();
                        }
                    } catch (const std::exception& ex) {
                        Logger::warn("ShortcutManager: Error handling Activated signal: {}", ex.what());
                    }
                }
            },
            "org.freedesktop.portal.Desktop",
            "org.freedesktop.portal.GlobalShortcuts",
            "Activated",
            "/org/freedesktop/portal/desktop"
        );

        return true;

    } catch (const Glib::Error& ex) {
        Logger::debug("ShortcutManager: Portal GlobalShortcuts registration error: {}", ex.what());
    } catch (const std::exception& ex) {
        Logger::debug("ShortcutManager: Portal GlobalShortcuts exception: {}", ex.what());
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
            if (item.raw() == gsettingsPath_) {
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
        customItemSettings->set_string("name", "Clipmoji");
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
