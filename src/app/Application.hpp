#pragma once

#include "core/EmojiDatabase.hpp"
#include "platform/ShortcutManager.hpp"
#include "platform/WindowManager.hpp"
#include "ui/MainWindow.hpp"
#include "utils/Config.hpp"

#include <gtkmm.h>
#include <memory>

class Application : public Gtk::Application {
public:
    static Glib::RefPtr<Application> create();

    Application();
    ~Application() override;

protected:
    void on_startup() override;
    void on_activate() override;
    int on_command_line(const Glib::RefPtr<Gio::ApplicationCommandLine>& command_line) override;

private:
    void load_styles();
    void load_database();

    EmojiDatabase db_;
    Config config_;
    MainWindow* window_{nullptr};
    std::unique_ptr<WindowManager> windowManager_;
    std::unique_ptr<ShortcutManager> shortcutManager_;
};
