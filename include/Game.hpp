#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include "Player.hpp"
#include "Enemy.hpp"
#include "LightingSystem.hpp"
#include "AssetManager.hpp"

// Game states enum
enum class GameState {
    Playing,
    GameOver,
    LevelTransition,
    DebugPanel // New state for showing debug/settings UI
};

class Game {
public:
    Game();
    ~Game() = default;
    void run();

private:
    void handleEvents();
    void update();
    void draw();
    void initializePlatforms();
    void initializeLadders();
    void initializeEnemies();
    void initializeLights();
    void initializeUI();
    void initializeMiniMap();
    void updateMiniMap();
    void updateLights();
    void checkPlayerEnemyCollision();
    void updateUI();
    void checkGameOver();
    void resetGame();
    void nextLevel();
    void checkLevelCompletion();
    void loadAssets();
    
    // Debug panel methods
    void drawDebugPanel();
    void initializeDebugPanel();
    bool drawSettingsButton(const std::string& label, bool* value, float x, float y, float width = 150.0f);
    bool drawSlider(const std::string& label, float* value, float min, float max, float x, float y, float width = 150.0f);
    void drawColorPicker(const std::string& label, sf::Color* color, float x, float y);
    void drawTabButton(const std::string& label, int tabId, int* currentTabId, float x, float y, float width = 120.0f);
    
    // Helper to create a heart shape
    sf::RectangleShape createHeartIcon(float x, float y, bool filled);

    // FPS counter methods
    void updateFPS();
    void drawFPS();

    sf::RenderWindow window;
    sf::View gameView;
    sf::View uiView; // Separate view for UI elements that don't scroll
    sf::View miniMapView; // View for the mini-map
    sf::Clock clock;
    Player player;
    std::vector<sf::RectangleShape> platforms;
    std::vector<sf::RectangleShape> ladders;
    std::vector<Enemy> enemies;
    bool playerHit;
    float playerHitCooldown;
    
    // Level system
    int currentLevel;
    float transitionTimer;
    sf::Text levelText;
    
    // Lighting system
    LightingSystem lightingSystem;
    int playerLightIndex; // Index of the player's light
    bool showLighting; // Flag to toggle lighting visibility
    
    // Mini-map elements
    sf::RectangleShape miniMapBorder;
    sf::RectangleShape miniMapPlayerIcon;
    std::vector<sf::RectangleShape> miniMapPlatforms;
    std::vector<sf::RectangleShape> miniMapLadders;
    std::vector<sf::RectangleShape> miniMapEnemies;
    bool showMiniMap; // Flag to toggle minimap visibility
    
    // Game state
    GameState currentState;
    GameState previousState; // For returning from debug panel
    
    // UI elements
    sf::Font defaultFont; // Default font for initialization
    sf::Font font;        // Game font that will be loaded
    sf::Text healthText;
    sf::Text gameOverText;
    sf::Text restartText;
    std::vector<sf::RectangleShape> heartIcons; // Heart icons for health display
    
    // Debug panel elements
    sf::RectangleShape debugPanelBackground;
    sf::Text debugPanelTitle;
    int currentDebugTab;
    bool showBoundingBoxes;
    float gameSpeed;
    sf::Color platformColor;
    sf::Color playerBorderColor;
    sf::Color enemyBorderColor;
    float spriteScale;
    float boundaryBoxHeight;
    
    // FPS tracking variables
    sf::Clock fpsClock;
    sf::Text fpsText;
    sf::RectangleShape fpsBackground;
    float fpsUpdateTime;
    int frameCount;
    float currentFPS;
    
    static constexpr int WINDOW_WIDTH = 800;
    static constexpr int WINDOW_HEIGHT = 600;
    static constexpr int LEVEL_WIDTH = 3000; // Extended level width for scrolling
    static constexpr int FPS = 60;
    static constexpr float HIT_COOLDOWN = 1.5f; // 1.5 seconds invulnerability
    static constexpr float LEVEL_TRANSITION_DURATION = 2.0f; // Duration of level transition in seconds
    
    // Mini-map constants
    static constexpr int MINI_MAP_WIDTH = 200;
    static constexpr int MINI_MAP_HEIGHT = 100;
    static constexpr int MINI_MAP_MARGIN = 10;

    AssetManager assets;
    // Use pointers for sprites to avoid constructor issues
    std::unique_ptr<sf::Sprite> backgroundSprite;
    std::unique_ptr<sf::Sprite> playerSprite;
    std::unique_ptr<sf::Sprite> enemySprite;
    
    // Store background texture size for tiling
    sf::Vector2u backgroundTextureSize;
    
    // Placeholder shapes to draw when textures are missing
    sf::RectangleShape backgroundPlaceholder;
    sf::RectangleShape playerPlaceholder;
    sf::RectangleShape enemyPlaceholder;
    
    // Flags to indicate when to use placeholders
    bool useBackgroundPlaceholder = true;
    bool usePlayerPlaceholder = true;
    bool useEnemyPlaceholder = true;
    
    sf::Vector2f playerPosition;
    float playerSpeed;
    bool isRunning;
}; 