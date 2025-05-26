#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <random>
#include <memory>

class TileRenderer {
public:
    TileRenderer();
    ~TileRenderer();
    
    // Initialize the tile system
    bool loadTiles(const std::string& tilesDirectory);
    
    // Render a platform using tiles
    void renderPlatform(sf::RenderWindow& window, const sf::RectangleShape& platform, bool randomize = true);
    
    // Render all platforms with tiles
    void renderPlatforms(sf::RenderWindow& window, const std::vector<sf::RectangleShape>& platforms, bool randomize = true);
    
    // Settings
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
    
    // Debug
    void renderTileGrid(sf::RenderWindow& window, const sf::Vector2f& position, const sf::Vector2f& size);

private:
    // Tile management
    std::vector<std::unique_ptr<sf::Texture>> tileTextures;
    std::vector<std::unique_ptr<sf::Sprite>> tileSprites;
    
    // Settings
    int tileSize = 32;  // Size of each tile in pixels
    float tileScale = 2.0f;  // Scale factor for tiles
    bool randomizationEnabled = true;
    
    // Random number generation
    std::mt19937 randomEngine;
    std::uniform_int_distribution<int> tileDistribution;
    
    // Helper methods
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