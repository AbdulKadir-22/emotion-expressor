#pragma once

#include "ui/MainWindow.hpp"
#include <sigc++/sigc++.h>

class WindowManager {
public:
    explicit WindowManager(MainWindow& window);
    ~WindowManager() = default;

    void show();
    void hide();
    void toggle();
    bool isVisible() const;

private:
    void centerOnActiveMonitor();

    MainWindow& window_;
    sigc::connection focusLossConnection_;
};
