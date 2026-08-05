#pragma once

#include <gtkmm.h>
#include <sigc++/sigc++.h>
#include <string>

class SearchBar : public Gtk::Box {
public:
    SearchBar();

    sigc::signal<void(const std::string&)> signal_text_changed();

    void grab_search_focus();
    void clear();
    std::string get_text() const;
    void set_text(const std::string& text);

private:
    Gtk::SearchEntry searchEntry_;
    sigc::signal<void(const std::string&)> signal_text_changed_;
};
