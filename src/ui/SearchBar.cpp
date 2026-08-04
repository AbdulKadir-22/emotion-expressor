#include "SearchBar.hpp"

SearchBar::SearchBar()
    : Gtk::Box(Gtk::Orientation::HORIZONTAL, 0) {
    add_css_class("search-bar-container");
    searchEntry_.set_placeholder_text("Search emojis...");
    searchEntry_.set_hexpand(true);
    searchEntry_.add_css_class("search-entry");

    searchEntry_.signal_search_changed().connect([this]() {
        signal_text_changed_.emit(searchEntry_.get_text());
    });

    append(searchEntry_);
}

sigc::signal<void(const std::string&)> SearchBar::signal_text_changed() {
    return signal_text_changed_;
}

void SearchBar::grab_search_focus() {
    searchEntry_.grab_focus();
}

void SearchBar::clear() {
    searchEntry_.set_text("");
}

std::string SearchBar::get_text() const {
    return searchEntry_.get_text();
}

void SearchBar::set_text(const std::string& text) {
    searchEntry_.set_text(text);
}
