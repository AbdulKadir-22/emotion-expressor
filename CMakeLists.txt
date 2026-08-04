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

# Core & Utils library target
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

# CLI Smoke Test Executable
add_executable(emoji_cli src/main.cpp)
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
