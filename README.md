# 2D Platform Puzzle Game

A 2D platformer game with puzzle elements, inspired by games like Animal Well and Braid. The game features basic shapes as platforms and characters, with mechanics including jumping and puzzle-solving elements.

## Requirements

- C++17 or later
- CMake 3.10 or later
- SFML 2.5 or later

## Building the Game

1. Create a build directory:
   ```bash
   mkdir build
   cd build
   ```

2. Generate build files:
   ```bash
   cmake ..
   ```

3. Build the game:
   ```bash
   cmake --build .
   ```

4. Run the game:
   ```bash
   ./PlatformPuzzleGame
   ```

## Controls

- Left Arrow: Move left
- Right Arrow: Move right
- Space: Jump
- ESC: Quit game

## Features

- Basic platforming mechanics
- Puzzle-solving elements
- Simple physics-based movement
- Clean, minimalist visual style 

target_link_libraries(PlatformPuzzleGame PRIVATE sfml-Graphics sfml-Window sfml-System) 