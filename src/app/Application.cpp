#include "Application.hpp"
#include "utils/Logger.hpp"

#include <filesystem>

Glib::RefPtr<Application> Application::create() {
    return Glib::make_refptr_for_instance<Application>(new Application());
}

Application::Application()
    : Gtk::Application("in.abdulkadir.clipmoji", Gio::Application::Flags::HANDLES_COMMAND_LINE) {
    add_main_option_entry(
        Gio::Application::OptionType::BOOL,
        "toggle",
        't',
        "Toggle the emoji picker window visibility"
    );
}

Application::~Application() {
    if (window_) {
        delete window_;
        window_ = nullptr;
    }
}

void Application::on_startup() {
    Gtk::Application::on_startup();

    // Keep daemon running continuously in background even when window is hidden
    hold();

    config_.load();
    load_database();
    load_styles();

    if (!window_) {
        window_ = new MainWindow(db_, config_);
        add_window(*window_);
    }

    windowManager_ = std::make_unique<WindowManager>(*window_);
    shortcutManager_ = std::make_unique<ShortcutManager>();

    std::string shortcut = config_.getShortcut();
    if (shortcut.empty()) {
        shortcut = "Ctrl+.";
    }

    Logger::info("Application: Registering configured global shortcut: '{}'", shortcut);
    bool registered = shortcutManager_->registerShortcut(shortcut, [this]() {
        if (windowManager_) {
            windowManager_->toggle();
        }
    });

    if (!registered) {
        Logger::warn("Application: Global shortcut registration failed; application remains accessible via terminal 'emotion_expressor --toggle'");
    }
}

namespace {
std::filesystem::path resolve_resource_path(const std::string& relativePath) {
    // 1. Try CWD relative
    if (std::filesystem::exists(relativePath)) {
        return relativePath;
    }
    if (std::filesystem::exists("../" + relativePath)) {
        return "../" + relativePath;
    }

    // 2. Try executable directory relative (/proc/self/exe on Linux)
    try {
        if (std::filesystem::exists("/proc/self/exe")) {
            std::filesystem::path exePath = std::filesystem::canonical("/proc/self/exe");
            std::filesystem::path exeDir = exePath.parent_path();

            if (std::filesystem::exists(exeDir / relativePath)) {
                return exeDir / relativePath;
            }
            if (std::filesystem::exists(exeDir / ".." / relativePath)) {
                return exeDir / ".." / relativePath;
            }
            if (std::filesystem::exists(exeDir / ".." / "share" / "emotion-expressor" / relativePath)) {
                return exeDir / ".." / "share" / "emotion-expressor" / relativePath;
            }
        }
    } catch (...) {
        // Ignore resolution errors if /proc/self/exe is unavailable
    }

    return relativePath;
}
} // namespace

void Application::load_database() {
    std::filesystem::path dbPath = resolve_resource_path("data/emojis.json");

    if (!db_.load(dbPath)) {
        Logger::error("Application: Failed to load emoji database from path {}", dbPath.string());
    } else {
        Logger::info("Application: Successfully loaded {} emojis from {}", db_.size(), dbPath.string());
    }
}

void Application::load_styles() {
    std::filesystem::path cssPath = resolve_resource_path("assets/styles/style.css");

    if (std::filesystem::exists(cssPath)) {
        cssProvider_ = Gtk::CssProvider::create();
        try {
            cssProvider_->load_from_path(cssPath.string());
            Gtk::StyleContext::add_provider_for_display(
                Gdk::Display::get_default(),
                cssProvider_,
                GTK_STYLE_PROVIDER_PRIORITY_USER
            );
            Logger::info("Loaded CSS styles from {}", cssPath.string());
        } catch (const Glib::Error& ex) {
            Logger::error("Failed to load CSS styles: {}", ex.what());
        }
    } else {
        Logger::warn("CSS stylesheet not found at {}", cssPath.string());
    }
}

void Application::on_activate() {
    if (windowManager_) {
        windowManager_->show();
    }
}

int Application::on_command_line(const Glib::RefPtr<Gio::ApplicationCommandLine>& command_line) {
    auto options = command_line->get_options_dict();
    bool isToggle = options->contains("toggle");

    Logger::info("Application: Received command line invocation (isToggle: {})", isToggle);

    if (windowManager_) {
        if (isToggle) {
            windowManager_->toggle();
        } else {
            windowManager_->show();
        }
    }

    return 0;
}
