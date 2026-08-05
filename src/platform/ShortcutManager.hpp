#pragma once

#include <functional>
#include <memory>
#include <string>
#include <giomm.h>

enum class ShortcutBackend {
    XdgPortal,
    GSettings,
    X11,
    None
};

class ShortcutManager {
public:
    ShortcutManager();
    ~ShortcutManager();

    bool registerShortcut(const std::string& accelerator, std::function<void()> onTrigger);
    void unregisterShortcut();
    ShortcutBackend getActiveBackend() const { return activeBackend_; }

    static std::string normalizeAcceleratorForGSettings(const std::string& accel);
    static std::string normalizeAcceleratorForPortal(const std::string& accel);

private:
    bool tryRegisterPortal(const std::string& accel);
    bool tryRegisterGSettings(const std::string& accel);
    bool tryRegisterX11(const std::string& accel);

    std::function<void()> callback_;
    ShortcutBackend activeBackend_{ShortcutBackend::None};
    std::string registeredAccelerator_;

    Glib::RefPtr<Gio::DBus::Connection> dbusConn_;
    guint portalSignalSubscriptionId_{0};
    std::string portalSessionHandle_;
    std::string gsettingsPath_;
};
