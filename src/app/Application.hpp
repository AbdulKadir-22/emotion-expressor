#pragma once

#include "core/EmojiDatabase.hpp"
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

private:
    void load_styles();
    void load_database();

    EmojiDatabase db_;
    Config config_;
    MainWindow* window_{nullptr};
};
