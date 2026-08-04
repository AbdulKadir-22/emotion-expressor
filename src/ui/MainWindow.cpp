#include "MainWindow.hpp"
#include "core/EmojiSearch.hpp"
#include "platform/Clipboard.hpp"
#include "utils/Logger.hpp"

#include <algorithm>

MainWindow::MainWindow(const EmojiDatabase& db, Config& config)
    : db_(db), config_(config) {
    set_title("Emotion Expressor");
    set_decorated(false);
    set_default_size(420, 480);
    add_css_class("popup-window");

    rootBox_.add_css_class("app-container");
    set_child(rootBox_);

    // Search bar
    searchBar_.signal_text_changed().connect(
        sigc::mem_fun(*this, &MainWindow::on_search_changed));
    rootBox_.append(searchBar_);

    // Model and Selection
    listStore_ = Gio::ListStore<EmojiItem>::create();
    selectionModel_ = Gtk::SingleSelection::create(listStore_);
    selectionModel_->set_autoselect(true);
    selectionModel_->set_can_unselect(false);

    // Row Factory
    auto factory = Gtk::SignalListItemFactory::create();
    factory->signal_setup().connect([](const Glib::RefPtr<Gtk::ListItem>& list_item) {
        auto rowBox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 12);
        rowBox->add_css_class("emoji-row");

        auto emojiGlyph = Gtk::make_managed<Gtk::Label>();
        emojiGlyph->add_css_class("emoji-glyph");

        auto textBox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 2);
        textBox->set_hexpand(true);

        auto nameLabel = Gtk::make_managed<Gtk::Label>();
        nameLabel->add_css_class("emoji-name");
        nameLabel->set_xalign(0.0);

        auto detailLabel = Gtk::make_managed<Gtk::Label>();
        detailLabel->add_css_class("emoji-detail");
        detailLabel->set_xalign(0.0);

        textBox->append(*nameLabel);
        textBox->append(*detailLabel);

        rowBox->append(*emojiGlyph);
        rowBox->append(*textBox);

        list_item->set_child(*rowBox);
    });

    factory->signal_bind().connect([](const Glib::RefPtr<Gtk::ListItem>& list_item) {
        auto item = std::dynamic_pointer_cast<EmojiItem>(list_item->get_item());
        if (!item || !item->emoji) return;

        auto rowBox = dynamic_cast<Gtk::Box*>(list_item->get_child());
        if (!rowBox) return;

        auto emojiGlyph = dynamic_cast<Gtk::Label*>(rowBox->get_first_child());
        auto textBox = dynamic_cast<Gtk::Box*>(emojiGlyph ? emojiGlyph->get_next_sibling() : nullptr);
        if (!emojiGlyph || !textBox) return;

        auto nameLabel = dynamic_cast<Gtk::Label*>(textBox->get_first_child());
        auto detailLabel = dynamic_cast<Gtk::Label*>(nameLabel ? nameLabel->get_next_sibling() : nullptr);
        if (!nameLabel || !detailLabel) return;

        emojiGlyph->set_text(item->emoji->character);
        nameLabel->set_text(item->emoji->name);

        std::string detail;
        if (!item->emoji->aliases.empty()) {
            detail = ":" + item->emoji->aliases[0] + ":";
        } else if (!item->emoji->keywords.empty()) {
            detail = item->emoji->keywords[0];
            for (size_t k = 1; k < item->emoji->keywords.size() && k < 3; ++k) {
                detail += ", " + item->emoji->keywords[k];
            }
        }
        detailLabel->set_text(detail);
    });

    // ListView & ScrolledWindow
    listView_.set_model(selectionModel_);
    listView_.set_factory(factory);
    listView_.set_single_click_activate(true);
    listView_.add_css_class("emoji-list");
    listView_.signal_activate().connect(
        sigc::mem_fun(*this, &MainWindow::on_row_activated));

    scrolledWindow_.set_child(listView_);
    scrolledWindow_.set_vexpand(true);
    scrolledWindow_.set_hexpand(true);
    scrolledWindow_.set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    scrolledWindow_.add_css_class("emoji-scrolled");
    rootBox_.append(scrolledWindow_);

    // Footer
    footerBox_.add_css_class("footer-bar");
    auto hintEsc = Gtk::make_managed<Gtk::Label>("Esc Close");
    hintEsc->add_css_class("footer-hint");
    auto hintSep1 = Gtk::make_managed<Gtk::Label>("·");
    hintSep1->add_css_class("footer-sep");
    auto hintNav = Gtk::make_managed<Gtk::Label>("↑↓ Navigate");
    hintNav->add_css_class("footer-hint");
    auto hintSep2 = Gtk::make_managed<Gtk::Label>("·");
    hintSep2->add_css_class("footer-sep");
    auto hintEnter = Gtk::make_managed<Gtk::Label>("Enter Copy");
    hintEnter->add_css_class("footer-hint");

    footerBox_.append(*hintEsc);
    footerBox_.append(*hintSep1);
    footerBox_.append(*hintNav);
    footerBox_.append(*hintSep2);
    footerBox_.append(*hintEnter);
    rootBox_.append(footerBox_);

    // Key Event Controller
    keyController_ = Gtk::EventControllerKey::create();
    keyController_->signal_key_pressed().connect(
        sigc::mem_fun(*this, &MainWindow::on_key_pressed), false);
    add_controller(keyController_);

    // Focus Loss Hide (best effort for GTK window focus state)
    // TODO (Phase 3): Compositor-level focus-loss window manager closing on Wayland.
    property_is_active().signal_changed().connect([this]() {
        if (!property_is_active().get_value()) {
            hide();
        }
    });

    prepare_for_show();
}

void MainWindow::prepare_for_show() {
    searchBar_.clear();
    searchBar_.grab_search_focus();
    on_search_changed("");
}

void MainWindow::on_search_changed(const std::string& query) {
    listStore_->remove_all();

    if (query.empty()) {
        const auto& recents = config_.getRecentEmojis();
        if (!recents.empty()) {
            for (const auto& rChar : recents) {
                const Emoji* e = db_.findByChar(rChar);
                if (e) {
                    listStore_->append(EmojiItem::create(e));
                }
            }
        } else {
            // Show default top emojis when no recents exist
            const auto& all = db_.all();
            size_t count = std::min<size_t>(30, all.size());
            for (size_t i = 0; i < count; ++i) {
                listStore_->append(EmojiItem::create(&all[i]));
            }
        }
    } else {
        auto results = EmojiSearch::search(db_, query, 50);
        for (const auto& res : results) {
            if (res.emoji) {
                listStore_->append(EmojiItem::create(res.emoji));
            }
        }
    }

    if (listStore_->get_n_items() > 0) {
        selectionModel_->set_selected(0);
        listView_.scroll_to(0);
    }
}

void MainWindow::on_row_activated(guint position) {
    select_and_copy_at(position);
}

bool MainWindow::on_key_pressed(guint keyval, guint /*keycode*/, Gdk::ModifierType /*state*/) {
    if (keyval == GDK_KEY_Escape) {
        hide();
        return true;
    }
    if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter) {
        activate_current_selection();
        return true;
    }
    if (keyval == GDK_KEY_Up) {
        move_selection(-1);
        return true;
    }
    if (keyval == GDK_KEY_Down) {
        move_selection(1);
        return true;
    }
    return false;
}

void MainWindow::move_selection(int delta) {
    guint total = listStore_->get_n_items();
    if (total == 0) return;

    guint current = selectionModel_->get_selected();
    guint next = current;
    if (delta < 0) {
        if (current > 0) next = current - 1;
    } else if (delta > 0) {
        if (current + 1 < total) next = current + 1;
    }

    if (next != current) {
        selectionModel_->set_selected(next);
        listView_.scroll_to(next);
    }
}

void MainWindow::activate_current_selection() {
    guint selected = selectionModel_->get_selected();
    select_and_copy_at(selected);
}

void MainWindow::select_and_copy_at(guint position) {
    if (position >= listStore_->get_n_items()) return;

    auto item = listStore_->get_item(position);
    if (!item || !item->emoji) return;

    std::string charToCopy = item->emoji->character;
    Clipboard::copyText(charToCopy);
    config_.addRecentEmoji(charToCopy, 20);
    config_.save();

    hide();
}
