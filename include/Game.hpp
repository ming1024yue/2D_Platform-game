#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include "Player.hpp"
#include "Enemy.hpp"
#include "LightingSystem.hpp"
#include "AssetManager.hpp"
#include "Physics.hpp"
#include "TileRenderer.hpp"
#include "imgui.h"
#include "imgui-SFML.h"

// Game states enum
enum class GameState {
    Playing,
    GameOver,
    LevelTransition,
    DebugPanel // New state for showing debug/settings UI
};

// Image asset info structure
struct ImageAssetInfo {
    std::string path;
    std::string name;
    sf::Vector2u dimensions;
    size_t fileSize;
    bool isLoaded;
    sf::Time loadTime;
};

// Background layer structure for parallax backgrounds
struct BackgroundLayer {
    std::unique_ptr<sf::Sprite> sprite;
    sf::Vector2u textureSize;
    float parallaxSpeed;    // 0.0 = static, 1.0 = moves with camera
    std::string name;       // Layer name (e.g., "sky", "clouds", "mountains", "ground")
    bool isLoaded;
    bool tileHorizontally;  // Whether to tile this layer horizontally
    bool tileVertically;    // Whether to tile this layer vertically
    
    BackgroundLayer(const std::string& layerName, float speed = 1.0f, 
                   bool tileH = true, bool tileV = false) 
        : parallaxSpeed(speed), name(layerName), isLoaded(false),
          tileHorizontally(tileH), tileVertically(tileV) {}
};

class Game {
public:
    Game();
    ~Game();
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
    void syncPlatformsWithPhysics();
    void checkPlayerEnemyCollision();
    void updateUI();
    void checkGameOver();
    void resetGame();
    void nextLevel();
    void jumpToLevel(int level);  // New method for jumping to specific levels
    void checkLevelCompletion();
    void loadAssets();
    void drawDebugBoxes();
    void drawDebugGrid();  // New method for drawing the canonical coordinate grid
    
    // Background layer methods
    void initializeBackgroundLayers();
    void loadBackgroundLayers();
    void drawBackgroundLayers();
    
    // Level-specific layouts
    void forestLevelPlatforms();
    void desertLevelPlatforms();
    void snowLevelPlatforms();
    
    // Level-specific enemy patterns
    void initializeForestEnemies();
    void initializeDesertEnemies();
    void initializeSnowEnemies();
    
    // ImGui methods
    void initializeImGui();
    void updateImGui();
    void renderImGui();
    void shutdownImGui();
    
    // Asset manager window
    void showAssetManagerWindow();
    void scanAssetDirectory(const std::string& directory);
    
    // FPS counter methods
    void updateFPS();
    void drawFPS();
    
    // Helper to create a heart shape
    sf::RectangleShape createHeartIcon(float x, float y, bool filled);
    
    // Logging methods
    void logDebug(const std::string& message);
    void logInfo(const std::string& message);
    void logWarning(const std::string& message);
    void logError(const std::string& message);
    void setLoggingEnabled(bool enabled) { loggingEnabled = enabled; }
    bool isLoggingEnabled() const { return loggingEnabled; }
    void clearGameLogFile();
    std::string getCurrentTimestamp();

    sf::RenderWindow window;
    sf::View gameView;
    sf::View uiView; // Separate view for UI elements that don't scroll
    sf::View miniMapView; // View for the mini-map
    sf::Clock clock;
    sf::Clock imguiClock; // Clock for ImGui updates
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
    
    // Physics system
    PhysicsSystem physicsSystem;
    
    // Tile rendering system
    TileRenderer tileRenderer;
    
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
    
    // Settings variables
    bool showBoundingBoxes;
    float gameSpeed;
    sf::Color platformColor;
    sf::Color playerBorderColor;
    sf::Color enemyBorderColor;
    float spriteScale;
    float boundaryBoxHeight;
    bool showEnemies;  // Toggle to show/hide enemies for testing
    
    // Debug grid variables
    bool showDebugGrid;
    float gridSize;
    sf::Color gridColor;
    sf::Color gridOriginColor;
    sf::Color gridAxesColor;
    
    // ImGui UI state variables
    bool showImGuiDemo;
    bool useImGuiInterface;
    bool showAssetManager; // Flag to show/hide the asset manager window
    
    // Asset manager variables
    std::vector<ImageAssetInfo> imageAssets;
    std::string assetRootDir = "assets";
    ImageAssetInfo* selectedAsset = nullptr;
    sf::Texture previewTexture;
    bool previewAvailable = false;
    
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
    static constexpr float GROUND_HEIGHT = 60.f; // Height of the ground platform from bottom of screen
    
    // Mini-map constants
    static constexpr int MINI_MAP_WIDTH = 200;
    static constexpr int MINI_MAP_HEIGHT = 100;
    static constexpr int MINI_MAP_MARGIN = 10;

    AssetManager assets;
    // Use pointers for sprites to avoid constructor issues
    std::unique_ptr<sf::Sprite> playerSprite;
    std::unique_ptr<sf::Sprite> enemySprite;
    
    // Layered background system
    std::vector<BackgroundLayer> backgroundLayers;
    
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
    
    // Logging system
    std::ofstream gameLogFile;
    bool loggingEnabled = true;
    std::string gameLogFileName = "game_debug.log";
}; 