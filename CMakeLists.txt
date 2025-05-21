cmake_minimum_required(VERSION 3.10)
project(PlatformPuzzleGame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SFML
find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)

# Add executable
add_executable(game 
    src/main.cpp
    src/Game.cpp
    src/Player.cpp
    src/Enemy.cpp
)

# Include directories
target_include_directories(game PRIVATE include /opt/homebrew/include)

# Link SFML libraries
target_link_libraries(game PRIVATE SFML::Graphics SFML::Window SFML::System) 