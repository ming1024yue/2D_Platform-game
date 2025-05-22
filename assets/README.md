# Game Assets Directory

This directory contains all assets for the game, organized into subdirectories:

## Directory Structure

```
assets/
├── images/
│   ├── backgrounds/   # Background images for game scenes
│   ├── characters/    # Player and NPC character sprites
│   └── enemies/       # Enemy sprites
├── audio/
│   ├── music/         # Background music tracks
│   └── sfx/           # Sound effects
└── fonts/             # Text fonts
```

## Asset Naming Convention

Use descriptive names with underscores:
- `background_forest.png`
- `character_hero_idle.png`
- `enemy_slime_move.png`
- `music_battle.ogg`
- `sfx_jump.wav`

## File Formats

- Images: PNG (preferred for transparency) or JPG
- Audio: OGG (music) or WAV (sound effects)
- Fonts: TTF

## Loading Assets

Use the AssetManager class to load and access assets in your game:

```cpp
// Example of loading and using assets
AssetManager assets;

// Load assets
assets.loadTexture("background", "assets/images/backgrounds/background_forest.png");
assets.loadTexture("player", "assets/images/characters/character_hero_idle.png");
assets.loadTexture("enemy", "assets/images/enemies/enemy_slime_move.png");
assets.loadFont("main_font", "assets/fonts/game_font.ttf");
assets.loadSoundBuffer("jump_sound", "assets/audio/sfx/sfx_jump.wav");

// Use assets
sf::Sprite backgroundSprite(assets.getTexture("background"));
sf::Sprite playerSprite(assets.getTexture("player"));
sf::Sprite enemySprite(assets.getTexture("enemy"));

sf::Text text;
text.setFont(assets.getFont("main_font"));

sf::Sound jumpSound;
jumpSound.setBuffer(assets.getSoundBuffer("jump_sound"));
``` 