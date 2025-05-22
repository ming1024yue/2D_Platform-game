# Adding Assets to Your Game

This guide explains how to add and use different types of assets in your game.

## Asset Types and Folder Structure

Assets are organized in the following structure:

```
assets/
├── images/
│   ├── backgrounds/   # Background images
│   ├── characters/    # Player and NPC sprites
│   └── enemies/       # Enemy sprites
├── audio/
│   ├── music/         # Background music
│   └── sfx/           # Sound effects
└── fonts/             # Text fonts
```

## File Formats

- **Images**: Use PNG (recommended for transparency) or JPG
- **Audio**: Use OGG for music and WAV for sound effects
- **Fonts**: Use TTF (TrueType Font)

## How to Add New Assets

### 1. Adding Images

1. Place your image files in the appropriate subdirectory:
   - `assets/images/backgrounds/` - for background images
   - `assets/images/characters/` - for player and NPC sprites
   - `assets/images/enemies/` - for enemy sprites

2. Load the images in the `Game::loadAssets()` method:

```cpp
// In Game.cpp - loadAssets() method
assets.loadTexture("my_background", "assets/images/backgrounds/my_background.png");
assets.loadTexture("my_character", "assets/images/characters/my_character.png");
```

3. Use the images in your game:

```cpp
// Create sprite and set texture
sf::Sprite mySprite;
mySprite.setTexture(assets.getTexture("my_background"));

// In your draw method
window.draw(mySprite);
```

### 2. Adding Sounds

1. Place audio files in the appropriate subdirectory:
   - `assets/audio/music/` - for background music
   - `assets/audio/sfx/` - for sound effects

2. Load the audio in the `Game::loadAssets()` method:

```cpp
assets.loadSoundBuffer("jump_sound", "assets/audio/sfx/jump.wav");
```

3. Use the sounds in your game:

```cpp
sf::Sound jumpSound;
jumpSound.setBuffer(assets.getSoundBuffer("jump_sound"));
jumpSound.play();
```

### 3. Adding Fonts

1. Place font files in the `assets/fonts/` directory

2. Load the font in the `Game::loadAssets()` method:

```cpp
assets.loadFont("main_font", "assets/fonts/myfont.ttf");
```

3. Use the font in your game:

```cpp
sf::Text myText;
myText.setFont(assets.getFont("main_font"));
myText.setString("Hello World");
myText.setCharacterSize(24);
myText.setFillColor(sf::Color::White);

// In your draw method
window.draw(myText);
```

## Creating Sprite Sheets

For animated characters, consider using sprite sheets:

1. Create a single image with multiple frames of animation
2. Load the entire sprite sheet as a texture
3. Use `sf::IntRect` to select portions of the texture for animation:

```cpp
sf::Sprite playerSprite;
playerSprite.setTexture(assets.getTexture("player_sheet"));

// Select a specific frame from the sprite sheet
// Parameters: x position, y position, width, height
playerSprite.setTextureRect(sf::IntRect(0, 0, 32, 32)); // First frame
playerSprite.setTextureRect(sf::IntRect(32, 0, 32, 32)); // Second frame
```

## Tips for Game Art

- Keep a consistent pixel size and style across your assets
- Use power-of-two textures for better performance (e.g., 256×256, 512×512)
- Consider using free assets from sites like:
  - OpenGameArt.org
  - Kenney.nl
  - Itch.io (free assets section)

## Creating Your Own Assets

You can create your own pixel art using:
- Aseprite
- GIMP
- Piskel (web-based)
- PyxelEdit

For sound effects:
- BFXR/SFXR
- Audacity