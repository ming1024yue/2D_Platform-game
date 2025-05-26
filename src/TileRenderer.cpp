#include "TileRenderer.hpp"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <chrono>
#include <cstdint>

namespace fs = std::filesystem;

TileRenderer::TileRenderer() {
    // Initialize random engine with current time
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    randomEngine.seed(static_cast<unsigned int>(seed));
}

TileRenderer::~TileRenderer() {
    // Cleanup is handled by smart pointers
}

bool TileRenderer::loadTiles(const std::string& tilesDirectory) {
    tileTextures.clear();
    tileSprites.clear();
    
    std::cout << "Loading tiles from: " << tilesDirectory << std::endl;
    
    if (!fs::exists(tilesDirectory)) {
        std::cerr << "Tiles directory does not exist: " << tilesDirectory << std::endl;
        return false;
    }
    
    // Load all PNG files from the tiles directory
    std::vector<std::string> tileFiles;
    
    try {
        for (const auto& entry : fs::directory_iterator(tilesDirectory)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (filename.size() >= 4 && filename.substr(filename.size() - 4) == ".png") {
                    tileFiles.push_back(entry.path().string());
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error reading tiles directory: " << e.what() << std::endl;
        return false;
    }
    
    // Sort files to ensure consistent loading order
    std::sort(tileFiles.begin(), tileFiles.end());
    
    if (tileFiles.empty()) {
        std::cerr << "No PNG files found in tiles directory" << std::endl;
        return false;
    }
    
    // Load each tile texture
    for (const auto& filePath : tileFiles) {
        auto texture = std::make_unique<sf::Texture>();
        
        if (texture->loadFromFile(filePath)) {
            auto sprite = std::make_unique<sf::Sprite>(*texture);
            sprite->setScale(sf::Vector2f(tileScale, tileScale));
            
            tileTextures.push_back(std::move(texture));
            tileSprites.push_back(std::move(sprite));
            
            std::cout << "Loaded tile: " << fs::path(filePath).filename().string() << std::endl;
        } else {
            std::cerr << "Failed to load tile: " << filePath << std::endl;
        }
    }
    
    if (tileTextures.empty()) {
        std::cerr << "No tiles were successfully loaded" << std::endl;
        return false;
    }
    
    // Update the distribution for random tile selection
    updateTileDistribution();
    
    std::cout << "Successfully loaded " << tileTextures.size() << " tiles" << std::endl;
    return true;
}

void TileRenderer::renderPlatform(sf::RenderWindow& window, const sf::RectangleShape& platform, bool randomize) {
    if (tileTextures.empty()) {
        // Fallback to original platform rendering if no tiles loaded
        window.draw(platform);
        return;
    }
    
    sf::Vector2f platformPos = platform.getPosition();
    sf::Vector2f platformSize = platform.getSize();
    
    // Generate tile layout for this platform
    auto tileLayout = generateTileLayout(platformPos, platformSize, randomize);
    
    // Render each tile
    for (const auto& tilePos : tileLayout) {
        sf::Sprite& sprite = getTileSprite(tilePos.tileIndex);
        
        float scaledTileSize = tileSize * tileScale;
        sprite.setPosition(sf::Vector2f(
            platformPos.x + tilePos.x * scaledTileSize,
            platformPos.y + tilePos.y * scaledTileSize
        ));
        
        window.draw(sprite);
    }
}

void TileRenderer::renderPlatforms(sf::RenderWindow& window, const std::vector<sf::RectangleShape>& platforms, bool randomize) {
    for (const auto& platform : platforms) {
        renderPlatform(window, platform, randomize);
    }
}

void TileRenderer::renderTileGrid(sf::RenderWindow& window, const sf::Vector2f& position, const sf::Vector2f& size) {
    if (tileTextures.empty()) return;
    
    float scaledTileSize = tileSize * tileScale;
    int tilesX = static_cast<int>(std::ceil(size.x / scaledTileSize));
    int tilesY = static_cast<int>(std::ceil(size.y / scaledTileSize));
    
    for (int y = 0; y < tilesY; y++) {
        for (int x = 0; x < tilesX; x++) {
            int tileIndex = (x + y) % tileTextures.size(); // Simple pattern for debug
            sf::Sprite& sprite = getTileSprite(tileIndex);
            
            sprite.setPosition(sf::Vector2f(
                position.x + x * scaledTileSize,
                position.y + y * scaledTileSize
            ));
            
            window.draw(sprite);
        }
    }
}

int TileRenderer::getRandomTileIndex() {
    if (tileTextures.empty()) return 0;
    return tileDistribution(randomEngine);
}

void TileRenderer::updateTileDistribution() {
    if (!tileTextures.empty()) {
        tileDistribution = std::uniform_int_distribution<int>(0, static_cast<int>(tileTextures.size() - 1));
    }
}

sf::Sprite& TileRenderer::getTileSprite(int index) {
    // Ensure index is within bounds
    index = std::max(0, std::min(index, static_cast<int>(tileSprites.size() - 1)));
    return *tileSprites[index];
}

std::vector<TileRenderer::TilePosition> TileRenderer::generateTileLayout(
    const sf::Vector2f& platformPos, 
    const sf::Vector2f& platformSize, 
    bool randomize) {
    
    std::vector<TilePosition> layout;
    
    float scaledTileSize = tileSize * tileScale;
    int tilesX = static_cast<int>(std::ceil(platformSize.x / scaledTileSize));
    int tilesY = static_cast<int>(std::ceil(platformSize.y / scaledTileSize));
    
    // Create a deterministic seed based on platform position for consistent randomization
    // This ensures the same platform always gets the same random pattern
    std::mt19937 localRandom;
    if (randomize && randomizationEnabled) {
        // Use platform position as seed to ensure consistent randomization per platform
        uint32_t seed = static_cast<uint32_t>(platformPos.x * 1000 + platformPos.y * 1000);
        localRandom.seed(seed);
    }
    
    for (int y = 0; y < tilesY; y++) {
        for (int x = 0; x < tilesX; x++) {
            TilePosition tilePos;
            tilePos.x = x;
            tilePos.y = y;
            
            if (randomize && randomizationEnabled) {
                // Use local random generator with deterministic seed for consistent results
                std::uniform_int_distribution<int> localDist(0, static_cast<int>(tileTextures.size() - 1));
                tilePos.tileIndex = localDist(localRandom);
            } else {
                // Use a deterministic pattern based on position
                // This creates a consistent but varied pattern
                int patternIndex = (x * 3 + y * 7) % tileTextures.size();
                tilePos.tileIndex = patternIndex;
            }
            
            layout.push_back(tilePos);
        }
    }
    
    return layout;
} 