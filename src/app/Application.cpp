#include "Application.hpp"
#include "utils/Logger.hpp"

#include <filesystem>

Glib::RefPtr<Application> Application::create() {
    return Glib::make_refptr_for_instance<Application>(new Application());
}

Application::Application()
    : Gtk::Application("dev.emotion_expressor.EmojiPicker", Gio::Application::Flags::HANDLES_COMMAND_LINE) {
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

void Application::load_database() {
    std::filesystem::path dbPath = "data/emojis.json";
    if (!std::filesystem::exists(dbPath) && std::filesystem::exists("../data/emojis.json")) {
        dbPath = "../data/emojis.json";
    }

    if (!db_.load(dbPath)) {
        Logger::error("Application: Failed to load emoji database from path {}", dbPath.string());
    } else {
        Logger::info("Application: Successfully loaded {} emojis from {}", db_.size(), dbPath.string());
    }
}

void Application::load_styles() {
    std::filesystem::path cssPath = "assets/styles/style.css";
    if (!std::filesystem::exists(cssPath) && std::filesystem::exists("../assets/styles/style.css")) {
        cssPath = "../assets/styles/style.css";
    }

    if (std::filesystem::exists(cssPath)) {
        auto provider = Gtk::CssProvider::create();
        try {
            provider->load_from_path(cssPath.string());
            Gtk::StyleContext::add_provider_for_display(
                Gdk::Display::get_default(),
                provider,
                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
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
