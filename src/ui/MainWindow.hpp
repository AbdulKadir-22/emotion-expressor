#pragma once

#include "SearchBar.hpp"
#include "core/Emoji.hpp"
#include "core/EmojiDatabase.hpp"
#include "utils/Config.hpp"

#include <gdkmm.h>
#include <giomm.h>
#include <gtkmm.h>

class EmojiItem : public Glib::Object {
public:
    const Emoji* emoji{nullptr};

    static Glib::RefPtr<EmojiItem> create(const Emoji* emoji) {
        return Glib::make_refptr_for_instance<EmojiItem>(new EmojiItem(emoji));
    }

protected:
    explicit EmojiItem(const Emoji* e) : emoji(e) {}
};

class MainWindow : public Gtk::ApplicationWindow {
public:
    MainWindow(const EmojiDatabase& db, Config& config);
    ~MainWindow() override = default;

    void prepare_for_show();

private:
    void on_search_changed(const std::string& query);
    void on_row_activated(guint position);
    bool on_key_pressed(guint keyval, guint keycode, Gdk::ModifierType state);
    void move_selection(int delta);
    void activate_current_selection();
    void select_and_copy_at(guint position);

    const EmojiDatabase& db_;
    Config& config_;

    Gtk::Box rootBox_{Gtk::Orientation::VERTICAL, 0};
    SearchBar searchBar_;
    Gtk::ScrolledWindow scrolledWindow_;
    Gtk::ListView listView_;
    Glib::RefPtr<Gio::ListStore<EmojiItem>> listStore_;
    Glib::RefPtr<Gtk::SingleSelection> selectionModel_;
    Gtk::Box footerBox_{Gtk::Orientation::HORIZONTAL, 8};
    Glib::RefPtr<Gtk::EventControllerKey> keyController_;
};
