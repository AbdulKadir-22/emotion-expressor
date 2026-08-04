cmake_minimum_required(VERSION 3.20)
project(emotion_expressor VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler Warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include(FetchContent)

# Fetch nlohmann_json via release tarball
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# Fetch Catch2 test framework via release tarball
FetchContent_Declare(
    Catch2
    URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.5.2.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(Catch2)

# GTK4 / gtkmm-4.0 Dependency
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET gtkmm-4.0)

# Core & Utils library target (GTK-free)
add_library(emoji_core STATIC
    src/core/Emoji.cpp
    src/core/Emoji.hpp
    src/core/EmojiDatabase.cpp
    src/core/EmojiDatabase.hpp
    src/core/EmojiSearch.cpp
    src/core/EmojiSearch.hpp
    src/utils/Config.cpp
    src/utils/Config.hpp
    src/utils/Logger.cpp
    src/utils/Logger.hpp
)

target_include_directories(emoji_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(emoji_core PUBLIC nlohmann_json::nlohmann_json)

# UI & Platform library target
add_library(emoji_ui STATIC
    src/app/Application.cpp
    src/app/Application.hpp
    src/ui/MainWindow.cpp
    src/ui/MainWindow.hpp
    src/ui/SearchBar.cpp
    src/ui/SearchBar.hpp
    src/platform/Clipboard.cpp
    src/platform/Clipboard.hpp
    src/platform/ShortcutManager.cpp
    src/platform/ShortcutManager.hpp
    src/platform/WindowManager.cpp
    src/platform/WindowManager.hpp
)

target_include_directories(emoji_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(emoji_ui PUBLIC emoji_core PkgConfig::GTKMM)

# Main GTK Application Executable Target
add_executable(emotion_expressor src/main.cpp)
target_link_libraries(emotion_expressor PRIVATE emoji_ui)

# CLI Smoke Test Executable Target (Phase 1 regression checking)
add_executable(emoji_picker_cli src/cli_main.cpp)
target_link_libraries(emoji_picker_cli PRIVATE emoji_core)

add_executable(emoji_cli src/cli_main.cpp)
target_link_libraries(emoji_cli PRIVATE emoji_core)

# Unit Tests Executable
enable_testing()

add_executable(emoji_tests
    tests/test_database.cpp
    tests/test_search.cpp
    tests/test_config.cpp
)
target_link_libraries(emoji_tests PRIVATE emoji_core Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(emoji_tests)
