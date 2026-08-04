#include "Application.hpp"
#include "utils/Logger.hpp"

#include <filesystem>

Glib::RefPtr<Application> Application::create() {
    return Glib::make_refptr_for_instance<Application>(new Application());
}

Application::Application()
    : Gtk::Application("dev.emotion_expressor.EmojiPicker", Gio::Application::Flags::NONE) {
}

Application::~Application() {
    if (window_) {
        delete window_;
        window_ = nullptr;
    }
}

void Application::on_startup() {
    Gtk::Application::on_startup();

    config_.load();
    load_database();
    load_styles();
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
    if (!window_) {
        window_ = new MainWindow(db_, config_);
        add_window(*window_);
    }
    window_->prepare_for_show();
    window_->present();
}
