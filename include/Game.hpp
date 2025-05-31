#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include "Player.hpp"
#include "Enemy.hpp"
#include "NPC.hpp"
#include "SoundSystem.h"

#include "AssetManager.hpp"
#include "Physics.hpp"
#include "RenderingSystem.hpp"
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

// BackgroundLayer is now defined in RenderingSystem.hpp

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
    void initializeNPCs();  // New method
    void initializeEnemies();
    void initializeUI();
    void initializeMiniMap();
    void initializeAudio(); // New method for audio initialization
    void updateMiniMap();
    void syncPlatformsWithPhysics();
    void checkPlayerEnemyCollision();
    void checkPlayerNPCCollision();  // New method for NPC collision detection
    void updateUI();
    void checkGameOver();
    void resetGame();
    void nextLevel();
    void previousLevel();  // Method to go back to the previous level
    void jumpToLevel(int level);  // New method for jumping to specific levels
    void checkLevelCompletion();
    void loadAssets();
    void drawDebugBoxes();

    
    // Background layer methods
    void initializeBackgroundLayers();
    void loadBackgroundLayers();
    
    // Level-specific layouts (removed for puzzle-focused gameplay)
    
    // Level-specific enemy patterns
    void initializeSnowEnemies();
    void initializeSnowForestEnemies();
    
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
    std::unique_ptr<NPC> npcManager;  // NPC manager
    bool playerHit;
    float playerHitCooldown;
    
    // Level system
    int currentLevel;
    float transitionTimer;
    sf::Text levelText;
    

    
    // Physics system
    PhysicsSystem physicsSystem;
    
    // Rendering system (includes tile rendering)
    RenderingSystem renderingSystem;
    
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
    sf::Text gameOverText;
    sf::Text restartText;
    
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
    static constexpr float LEVEL_TRANSITION_DURATION = 1.0f; // Duration of level transition in seconds
    static constexpr float GROUND_HEIGHT = 100.f; // Height of the ground platform from bottom of screen
    
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

    // Debug properties
    bool showPlayerDebug = false;
    
    // Fullscreen properties
    bool isFullscreen = false;
    sf::VideoMode previousVideoMode;  // Store previous window size/mode
    sf::Vector2i previousPosition;    // Store previous window position

    // Sound system
    SoundSystem soundSystem;
    bool isMusicEnabled;
    bool isSoundEffectsEnabled;
    float musicVolume;
    float soundEffectVolume;
}; 