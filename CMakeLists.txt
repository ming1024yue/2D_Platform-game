cmake_minimum_required(VERSION 3.10)
project(PlatformPuzzleGame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SFML
find_package(SFML 3 COMPONENTS Graphics Window System Audio REQUIRED)

# Find OpenGL (needed for ImGui-SFML)
find_package(OpenGL REQUIRED)

# ImGui sources
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)
set(IMGUI_SFML_DIR ${CMAKE_SOURCE_DIR}/external/imgui-sfml)

# ImGui source files
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_SFML_DIR}/imgui-SFML.cpp
)

# Define core game sources
set(GAME_SOURCES
    src/main.cpp
    src/Game.cpp
    src/GameImGui.cpp
    src/Player.cpp
    src/Enemy.cpp
    src/LightingSystem.cpp
    src/AssetManager.cpp
    src/Physics.cpp
    src/RenderingSystem.cpp
    src/TileRenderer.cpp
)

# Add executable
add_executable(game 
    ${GAME_SOURCES}
    ${IMGUI_SOURCES}
)

# Include directories
target_include_directories(game PRIVATE 
    include 
    /opt/homebrew/include
    /opt/homebrew/Cellar/sfml/3.0.1/include
    ${IMGUI_DIR}
    ${IMGUI_SFML_DIR}
    ${OPENGL_INCLUDE_DIR}
)

# Link SFML libraries and OpenGL
target_link_libraries(game PRIVATE 
    SFML::Graphics 
    SFML::Window 
    SFML::System 
    SFML::Audio
    ${OPENGL_LIBRARIES}
)

# Add debug subdirectory
add_subdirectory(src/debug) 