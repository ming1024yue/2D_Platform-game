#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <fstream>
#include <string>
#include <random>

// Forward declarations
class Player;
class Enemy;
class PhysicsSystem;


struct BackgroundLayer {
    std::string name;
    float parallaxSpeed;
    bool tileHorizontally;
    bool tileVertically;
    bool isLoaded = false;
    std::unique_ptr<sf::Sprite> sprite;
    sf::Vector2u textureSize;
    
    BackgroundLayer(const std::string& layerName, float speed, bool tileH, bool tileV)
        : name(layerName), parallaxSpeed(speed), tileHorizontally(tileH), tileVertically(tileV) {}
    
    // Move constructor
    BackgroundLayer(BackgroundLayer&& other) noexcept
        : name(std::move(other.name)), parallaxSpeed(other.parallaxSpeed),
          tileHorizontally(other.tileHorizontally), tileVertically(other.tileVertically),
          isLoaded(other.isLoaded), sprite(std::move(other.sprite)), textureSize(other.textureSize) {}
    
    // Move assignment operator
    BackgroundLayer& operator=(BackgroundLayer&& other) noexcept {
        if (this != &other) {
            name = std::move(other.name);
            parallaxSpeed = other.parallaxSpeed;
            tileHorizontally = other.tileHorizontally;
            tileVertically = other.tileVertically;
            isLoaded = other.isLoaded;
            sprite = std::move(other.sprite);
            textureSize = other.textureSize;
        }
        return *this;
    }
    
    // Delete copy constructor and copy assignment operator
    BackgroundLayer(const BackgroundLayer&) = delete;
    BackgroundLayer& operator=(const BackgroundLayer&) = delete;
};

class RenderingSystem {
public:
    RenderingSystem();
    ~RenderingSystem();

    // Main rendering method
    void renderFrame();
    
    // Background rendering
    void renderBackground();
    void renderBackgroundLayers();
    void setBackgroundLayers(std::vector<BackgroundLayer>&& layers);
    void setBackgroundLayersRef(const std::vector<BackgroundLayer>& layers);
    
    // Game object rendering
    void renderPlatforms(const std::vector<sf::RectangleShape>& platforms);

    void renderPlayer(const Player& player);
    void renderEnemies(const std::vector<Enemy>& enemies);
    
    // Debug rendering
    void renderDebugGrid();
    void renderDebugBoxes();
    
    // UI rendering
    void renderUI();
    void renderFPS();
    void renderMiniMap();
    

    
    // Logging controls
    void setLoggingEnabled(bool enabled) { loggingEnabled = enabled; }
    bool isLoggingEnabled() const { return loggingEnabled; }
    void clearLogFile();
    
    // Settings
    void setShowBoundingBoxes(bool show) { showBoundingBoxes = show; }
    void setShowDebugGrid(bool show) { showDebugGrid = show; }
    void setShowMiniMap(bool show) { showMiniMap = show; }

    void setShowEnemies(bool show) { showEnemies = show; }
    void setSpriteScale(float scale) { spriteScale = scale; }
    void setGridSize(float size) { gridSize = size; }
    void setGridColor(const sf::Color& color) { gridColor = color; }
    void setGridOriginColor(const sf::Color& color) { gridOriginColor = color; }
    void setGridAxesColor(const sf::Color& color) { gridAxesColor = color; }
    
    // Getters
    bool getShowBoundingBoxes() const { return showBoundingBoxes; }
    bool getShowDebugGrid() const { return showDebugGrid; }
    bool getShowMiniMap() const { return showMiniMap; }

    bool getShowEnemies() const { return showEnemies; }
    float getSpriteScale() const { return spriteScale; }
    float getGridSize() const { return gridSize; }
    const sf::Color& getGridColor() const { return gridColor; }
    const sf::Color& getGridOriginColor() const { return gridOriginColor; }
    const sf::Color& getGridAxesColor() const { return gridAxesColor; }
    
    // Sprite management
    void setPlayerSprite(std::unique_ptr<sf::Sprite> sprite);
    void setEnemySprite(std::unique_ptr<sf::Sprite> sprite);
    void setUsePlayerPlaceholder(bool use) { usePlayerPlaceholder = use; }
    void setUseEnemyPlaceholder(bool use) { useEnemyPlaceholder = use; }
    
    // Background management
    void setUseBackgroundPlaceholder(bool use) { useBackgroundPlaceholder = use; }
    void setBackgroundPlaceholder(const sf::RectangleShape& placeholder);
    
    // Tile rendering functionality (moved from TileRenderer)
    bool loadTiles(const std::string& tilesDirectory);
    void renderPlatform(sf::RenderWindow& window, const sf::RectangleShape& platform, bool randomize = true);
    void renderPlatforms(sf::RenderWindow& window, const std::vector<sf::RectangleShape>& platforms, bool randomize = true);
    void renderTileGrid(sf::RenderWindow& window, const sf::Vector2f& position, const sf::Vector2f& size);
    
    // General rendering utilities
    void renderBackground(sf::RenderWindow& window, const sf::Sprite& background);
    void renderEntity(sf::RenderWindow& window, const sf::Sprite& sprite, const sf::Vector2f& position);
    void renderShape(sf::RenderWindow& window, const sf::Shape& shape);
    
    // Tile settings
    void setTileSize(int size) { tileSize = size; }
    void setRandomSeed(unsigned int seed) { randomEngine.seed(seed); }
    void setRandomizationEnabled(bool enabled) { randomizationEnabled = enabled; }
    void setTileScale(float scale) { tileScale = scale; }
    
    // Getters
    int getTileSize() const { return tileSize; }
    bool isRandomizationEnabled() const { return randomizationEnabled; }
    float getTileScale() const { return tileScale; }
    int getTileCount() const { return tileTextures.size(); }
    bool isLoaded() const { return !tileTextures.empty(); }
    
    // Rendering state management
    void setRenderTarget(sf::RenderWindow* window) { renderTarget = window; }
    sf::RenderWindow* getRenderTarget() const { return renderTarget; }
    
    // Batch rendering for performance
    void beginBatch();
    void endBatch();
    void addToBatch(const sf::Sprite& sprite, const sf::Vector2f& position);

private:
    
    // Logging system
    std::ofstream logFile;
    bool loggingEnabled = true;
    std::string logFileName = "rendering_debug.log";
    
    // Background system
    std::vector<BackgroundLayer> backgroundLayers;
    bool useBackgroundPlaceholder = true;
    sf::RectangleShape backgroundPlaceholder;
    
    // Sprite management
    std::unique_ptr<sf::Sprite> playerSprite;
    std::unique_ptr<sf::Sprite> enemySprite;
    bool usePlayerPlaceholder = true;
    bool useEnemyPlaceholder = true;
    sf::RectangleShape playerPlaceholder;
    sf::RectangleShape enemyPlaceholder;
    
    // Rendering settings
    bool showBoundingBoxes = true;
    bool showDebugGrid = false;
    bool showMiniMap = true;

    bool showEnemies = true;
    float spriteScale = 4.0f;
    
    // Debug grid settings
    float gridSize = 50.0f;
    sf::Color gridColor = sf::Color(128, 128, 128, 64);
    sf::Color gridOriginColor = sf::Color(255, 255, 0, 128);
    sf::Color gridAxesColor = sf::Color(255, 255, 255, 96);
    
    // Tile management (from TileRenderer)
    std::vector<std::unique_ptr<sf::Texture>> tileTextures;
    std::vector<std::unique_ptr<sf::Sprite>> tileSprites;
    
    // Tile settings
    int tileSize = 16;
    float tileScale = 2.0f;
    bool randomizationEnabled = true;
    
    // Random number generation
    std::mt19937 randomEngine;
    std::uniform_int_distribution<int> tileDistribution;
    
    // Rendering state
    sf::RenderWindow* renderTarget = nullptr;
    
    // Batch rendering
    std::vector<std::pair<sf::Sprite, sf::Vector2f>> spriteBatch;
    bool batchMode = false;
    
    // Constants for background rendering (moved from Game class)
    static constexpr int WINDOW_WIDTH = 800;
    static constexpr int WINDOW_HEIGHT = 600;
    static constexpr float GROUND_HEIGHT = 60.f;
    
    // Helper methods
    void renderSpriteWithDirection(const sf::Sprite& sprite, const sf::Vector2f& position, bool facingLeft = false);
    void renderPlaceholderWithDirection(const sf::RectangleShape& placeholder, const sf::Vector2f& position, bool facingLeft = false);
    
    // Logging helper methods
    void logDebug(const std::string& message);
    void logInfo(const std::string& message);
    void logWarning(const std::string& message);
    void logError(const std::string& message);
    std::string getCurrentTimestamp();
    
    // Helper methods (from TileRenderer)
    int getRandomTileIndex();
    void updateTileDistribution();
    sf::Sprite& getTileSprite(int index);
    
    // Tile positioning
    struct TilePosition {
        int x, y;
        int tileIndex;
    };
    
    std::vector<TilePosition> generateTileLayout(const sf::Vector2f& platformPos, 
                                                const sf::Vector2f& platformSize, 
                                                bool randomize);
};