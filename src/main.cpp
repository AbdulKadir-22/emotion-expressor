#include "app/Application.hpp"
#include "utils/Logger.hpp"

int main(int argc, char* argv[]) {
    Logger::info("Starting Emotion Expressor GUI (Phase 2)...");
    auto app = Application::create();
    return app->run(argc, argv);
}
