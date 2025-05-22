cmake_minimum_required(VERSION 3.10)
project(TestTexture)

# Find SFML
find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)

# Create executable
add_executable(TestTexture TestTexture.cpp)

# Link SFML libraries
target_link_libraries(TestTexture PRIVATE SFML::Graphics SFML::Window SFML::System) 