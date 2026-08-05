#include "MainWindow.hpp"
#include "core/EmojiSearch.hpp"
#include "platform/Clipboard.hpp"
#include "utils/Logger.hpp"

#include <algorithm>
#include <cctype>

namespace {
std::string to_title_case(const std::string& input) {
    if (input.empty()) return input;
    std::string result = input;
    bool capitalize_next = true;
    for (char& c : result) {
        if (std::isspace(static_cast<unsigned char>(c)) || c == '_' || c == '-') {
            capitalize_next = true;
            if (c == '_' || c == '-') c = ' ';
        } else if (capitalize_next) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            capitalize_next = false;
        } else {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    }
    return result;
}
} // namespace

MainWindow::MainWindow(const EmojiDatabase& db, Config& config)
    : db_(db), config_(config) {
    set_title("Emotion Expressor");
    set_decorated(false);
    set_default_size(440, 520);
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

    selectionModel_->property_selected().signal_changed().connect([this]() {
        update_preview();
    });

    // 7-Column Grid Item Factory
    auto factory = Gtk::SignalListItemFactory::create();
    factory->signal_setup().connect([this](const Glib::RefPtr<Gtk::ListItem>& list_item) {
        list_item->set_selectable(true);

        auto tileBox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 0);
        tileBox->add_css_class("emoji-tile");
        tileBox->set_halign(Gtk::Align::FILL);
        tileBox->set_valign(Gtk::Align::FILL);
        tileBox->set_hexpand(true);
        tileBox->set_vexpand(true);

        auto emojiLabel = Gtk::make_managed<Gtk::Label>();
        emojiLabel->add_css_class("emoji-tile-glyph");
        emojiLabel->set_halign(Gtk::Align::CENTER);
        emojiLabel->set_valign(Gtk::Align::CENTER);
        emojiLabel->set_hexpand(true);
        emojiLabel->set_vexpand(true);

        tileBox->append(*emojiLabel);
        list_item->set_child(*tileBox);
    });

    factory->signal_bind().connect([this](const Glib::RefPtr<Gtk::ListItem>& list_item) {
        auto item = std::dynamic_pointer_cast<EmojiItem>(list_item->get_item());
        if (!item || !item->emoji) return;

        auto tileBox = dynamic_cast<Gtk::Box*>(list_item->get_child());
        if (!tileBox) return;

        auto emojiLabel = dynamic_cast<Gtk::Label*>(tileBox->get_first_child());
        if (emojiLabel) {
            emojiLabel->set_text(item->emoji->character);
        }
    });

    // GridView Configuration
    gridView_.set_model(selectionModel_);
    gridView_.set_factory(factory);
    gridView_.set_min_columns(GRID_COLUMNS);
    gridView_.set_max_columns(GRID_COLUMNS);
    gridView_.set_single_click_activate(true);
    gridView_.add_css_class("emoji-grid-view");
    gridView_.signal_activate().connect(
        sigc::mem_fun(*this, &MainWindow::on_row_activated));

    scrolledWindow_.set_child(gridView_);
    scrolledWindow_.set_vexpand(true);
    scrolledWindow_.set_hexpand(true);
    scrolledWindow_.set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    scrolledWindow_.add_css_class("emoji-scrolled");
    rootBox_.append(scrolledWindow_);

    // Dynamic Footer Preview Card
    footerCard_.add_css_class("footer-preview-card");

    previewGlyph_.add_css_class("preview-glyph-large");
    previewName_.add_css_class("preview-title-text");
    previewName_.set_xalign(0.0);
    previewAlias_.add_css_class("preview-alias-text");
    previewAlias_.set_xalign(0.0);

    previewDetailsBox_.append(previewName_);
    previewDetailsBox_.append(previewAlias_);

    previewBox_.append(previewGlyph_);
    previewBox_.append(previewDetailsBox_);
    previewBox_.set_hexpand(true);
    footerCard_.append(previewBox_);

    // Hotkey Action Badges
    auto escBadge = Gtk::make_managed<Gtk::Label>("Esc");
    escBadge->add_css_class("kbd-badge");
    auto escLabel = Gtk::make_managed<Gtk::Label>("Close");
    escLabel->add_css_class("kbd-label");
    auto escUnit = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 6);
    escUnit->append(*escBadge);
    escUnit->append(*escLabel);

    auto sep = Gtk::make_managed<Gtk::Label>("│");
    sep->add_css_class("footer-sep");

    auto enterBadge = Gtk::make_managed<Gtk::Label>("Enter");
    enterBadge->add_css_class("kbd-badge");
    auto enterLabel = Gtk::make_managed<Gtk::Label>("Copy");
    enterLabel->add_css_class("kbd-label");
    auto enterUnit = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 6);
    enterUnit->append(*enterBadge);
    enterUnit->append(*enterLabel);

    hotkeyBox_.append(*escUnit);
    hotkeyBox_.append(*sep);
    hotkeyBox_.append(*enterUnit);
    hotkeyBox_.set_valign(Gtk::Align::CENTER);
    footerCard_.append(hotkeyBox_);

    rootBox_.append(footerCard_);

    // Key Event Controller with CAPTURE phase so Arrow keys (Left, Right, Up, Down) work globally
    keyController_ = Gtk::EventControllerKey::create();
    keyController_->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    keyController_->signal_key_pressed().connect(
        sigc::mem_fun(*this, &MainWindow::on_key_pressed), false);
    add_controller(keyController_);

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
            const auto& all = db_.all();
            size_t count = std::min<size_t>(42, all.size());
            for (size_t i = 0; i < count; ++i) {
                listStore_->append(EmojiItem::create(&all[i]));
            }
        }
    } else {
        auto results = EmojiSearch::search(db_, query, 70);
        for (const auto& res : results) {
            if (res.emoji) {
                listStore_->append(EmojiItem::create(res.emoji));
            }
        }
    }

    if (listStore_->get_n_items() > 0) {
        selectionModel_->set_selected(0);
        gridView_.scroll_to(0);
    }
    update_preview();
}

void MainWindow::update_preview() {
    guint selected = selectionModel_->get_selected();
    if (selected < listStore_->get_n_items()) {
        auto item = listStore_->get_item(selected);
        if (item && item->emoji) {
            previewGlyph_.set_text(item->emoji->character);
            previewName_.set_text(to_title_case(item->emoji->name));
            if (!item->emoji->aliases.empty()) {
                previewAlias_.set_text(":" + item->emoji->aliases[0] + ":");
            } else if (!item->emoji->keywords.empty()) {
                previewAlias_.set_text(item->emoji->keywords[0]);
            } else {
                previewAlias_.set_text("");
            }
            return;
        }
    }
    previewGlyph_.set_text("🔍");
    previewName_.set_text("No matching emoji");
    previewAlias_.set_text("");
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
    if (keyval == GDK_KEY_Left || keyval == GDK_KEY_KP_Left) {
        move_selection(-1);
        return true;
    }
    if (keyval == GDK_KEY_Right || keyval == GDK_KEY_KP_Right) {
        move_selection(1);
        return true;
    }
    if (keyval == GDK_KEY_Up || keyval == GDK_KEY_KP_Up) {
        move_selection(-static_cast<int>(GRID_COLUMNS));
        return true;
    }
    if (keyval == GDK_KEY_Down || keyval == GDK_KEY_KP_Down) {
        move_selection(static_cast<int>(GRID_COLUMNS));
        return true;
    }
    return false;
}

void MainWindow::move_selection(int delta) {
    guint total = listStore_->get_n_items();
    if (total == 0) return;

    int current = static_cast<int>(selectionModel_->get_selected());
    int next = current + delta;
    next = std::clamp(next, 0, static_cast<int>(total) - 1);

    if (next != current) {
        selectionModel_->set_selected(static_cast<guint>(next));
        gridView_.scroll_to(static_cast<guint>(next));
        update_preview();
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
