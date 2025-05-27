#include "RenderingSystem.hpp"
#include "Player.hpp"
#include "Enemy.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cstdint>

namespace fs = std::filesystem;

RenderingSystem::RenderingSystem() {
    // Initialize logging
    logFile.open(logFileName, std::ios::out | std::ios::app);
    if (logFile.is_open()) {
        logInfo("RenderingSystem initialized");
    }
    
    // Initialize placeholder shapes
    backgroundPlaceholder.setSize(sf::Vector2f(3000, 600)); // LEVEL_WIDTH x WINDOW_HEIGHT
    backgroundPlaceholder.setFillColor(sf::Color(100, 180, 100)); // Green
    backgroundPlaceholder.setPosition(sf::Vector2f(0, 0));
    
    playerPlaceholder.setSize(sf::Vector2f(32, 32));
    playerPlaceholder.setFillColor(sf::Color::Blue);
    
    enemyPlaceholder.setSize(sf::Vector2f(32, 32));
    enemyPlaceholder.setFillColor(sf::Color::Red);
    
    // Initialize random engine for tile rendering
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    randomEngine.seed(static_cast<unsigned int>(seed));
    
    logInfo("Placeholder shapes and tile system initialized");
}

RenderingSystem::~RenderingSystem() {
    if (logFile.is_open()) {
        logInfo("RenderingSystem shutting down");
        logFile.close();
    }
}

void RenderingSystem::renderFrame() {
    // Main rendering orchestration method
    // This will be the central method that coordinates all rendering
    // Note: This method requires a render target to be set
    
    if (!renderTarget) {
        logError("No render target set for renderFrame()");
        return;
    }
    
    // Clear the window
    renderTarget->clear(sf::Color::Black);
    
    // Render background
    renderBackground();
    
    // Render debug grid if enabled
    if (showDebugGrid) {
        renderDebugGrid();
    }
    
    // Render debug boxes if enabled
    if (showBoundingBoxes) {
        renderDebugBoxes();
    }
    

    
    // Render UI elements
    renderUI();
    
    // Render FPS
    renderFPS();
    
    // Render mini-map if enabled
    if (showMiniMap) {
        renderMiniMap();
    }
}

void RenderingSystem::renderBackground() {
    if (!renderTarget) return;
    
    if (useBackgroundPlaceholder) {
        renderTarget->draw(backgroundPlaceholder);
        logDebug("Rendered background placeholder");
    } else {
        renderBackgroundLayers();
        logDebug("Rendered background layers");
    }
}

void RenderingSystem::renderBackgroundLayers() {
    if (!renderTarget) return;
    
    // Get the current view from the render target
    sf::View currentView = renderTarget->getView();
    sf::Vector2f viewCenter = currentView.getCenter();
    sf::Vector2f viewSize = currentView.getSize();
    
    // Calculate the visible area
    float leftX = viewCenter.x - viewSize.x / 2.0f;
    float rightX = viewCenter.x + viewSize.x / 2.0f;
    float topY = viewCenter.y - viewSize.y / 2.0f;
    float bottomY = viewCenter.y + viewSize.y / 2.0f;
    
    int layersRendered = 0;
    
    // Draw layers from back to front (background1 -> background2 -> background3 -> background4)
    for (auto& layer : backgroundLayers) {
        if (!layer.isLoaded || !layer.sprite) continue;
        
        // Calculate parallax offset
        // For parallax, we want layers to move slower than the camera
        // A speed of 0.0 means static (no movement), 1.0 means moves with camera
        float parallaxOffsetX = (viewCenter.x - WINDOW_WIDTH / 2.0f) * layer.parallaxSpeed;
        float parallaxOffsetY = (viewCenter.y - WINDOW_HEIGHT / 2.0f) * layer.parallaxSpeed;
        
        // Calculate uniform scale to maintain aspect ratio
        float scaleX = viewSize.x / layer.textureSize.x;
        float scaleY = viewSize.y / layer.textureSize.y;
        
        // Choose scaling strategy based on layer type
        float uniformScale;
        if (layer.name == "background1") {
            // Background1 should completely fill the screen
            uniformScale = std::max(scaleX, scaleY);
        } else {
            // Other layers can use different strategies
            uniformScale = std::max(scaleX, scaleY); // Fill screen completely
        }
        
        // Apply uniform scaling
        layer.sprite->setScale(sf::Vector2f(uniformScale, uniformScale));
        
        // Calculate scaled texture dimensions
        float scaledWidth = layer.textureSize.x * uniformScale;
        float scaledHeight = layer.textureSize.y * uniformScale;
        
        if (layer.tileHorizontally) {
            // Calculate starting positions for tiling
            float startX = std::floor((leftX + parallaxOffsetX) / scaledWidth) * scaledWidth - parallaxOffsetX;
            float startY;
            
            // Special positioning for background4 layer to align with actual ground platforms
            if (layer.name == "background4") {
                // Position background4 layer to align with the actual ground platforms
                // The ground platforms are positioned at WINDOW_HEIGHT - GROUND_HEIGHT (y=540)
                float groundLevel = WINDOW_HEIGHT - GROUND_HEIGHT; // This is where the platforms are (y=540)
                
                // Position the background4 texture to cover the ground platforms area
                // We want the background4 texture to be positioned so it covers the platform area
                // The platforms are 60px high starting at y=540, so we need to cover y=540 to y=600
                startY = groundLevel - (scaledHeight * 0.8f); // Start 80% of texture height above ground level to ensure coverage
                
                // Apply parallax effect for background4 layer (it should move with camera)
                startY += parallaxOffsetY;
            } else {
                // Normal positioning for other layers
                startY = layer.tileVertically ? 
                        std::floor((topY + parallaxOffsetY) / scaledHeight) * scaledHeight - parallaxOffsetY :
                        topY + parallaxOffsetY;
            }
            
            // Draw tiles
            if (layer.tileVertically) {
                // Tile both horizontally and vertically
                for (float y = startY; y < bottomY + scaledHeight; y += scaledHeight) {
                    for (float x = startX; x < rightX + scaledWidth; x += scaledWidth) {
                        layer.sprite->setPosition(sf::Vector2f(x, y));
                        renderTarget->draw(*layer.sprite);
                    }
                }
            } else {
                // Tile only horizontally
                for (float x = startX; x < rightX + scaledWidth; x += scaledWidth) {
                    layer.sprite->setPosition(sf::Vector2f(x, startY));
                    renderTarget->draw(*layer.sprite);
                }
            }
        } else {
            // Single image, positioned with parallax
            layer.sprite->setPosition(sf::Vector2f(
                leftX + parallaxOffsetX,
                topY + parallaxOffsetY
            ));
            renderTarget->draw(*layer.sprite);
        }
        
        layersRendered++;
    }
    
    logDebug("Rendered " + std::to_string(layersRendered) + " background layers with parallax");
}

void RenderingSystem::setBackgroundLayers(std::vector<BackgroundLayer>&& layers) {
    backgroundLayers = std::move(layers);
    logInfo("Background layers updated, count: " + std::to_string(backgroundLayers.size()));
}

void RenderingSystem::setBackgroundLayersRef(const std::vector<BackgroundLayer>& layers) {
    // Clear existing layers
    backgroundLayers.clear();
    
    // Copy the layers (note: this creates new BackgroundLayer objects)
    for (const auto& layer : layers) {
        // Create a new layer with the same properties
        BackgroundLayer newLayer(layer.name, layer.parallaxSpeed, layer.tileHorizontally, layer.tileVertically);
        newLayer.isLoaded = layer.isLoaded;
        newLayer.textureSize = layer.textureSize;
        
        // Copy the sprite if it exists
        if (layer.sprite && layer.isLoaded) {
            newLayer.sprite = std::make_unique<sf::Sprite>(*layer.sprite);
        }
        
        backgroundLayers.push_back(std::move(newLayer));
    }
    
    logInfo("Background layers copied from reference, count: " + std::to_string(backgroundLayers.size()));
}

void RenderingSystem::renderPlatforms(const std::vector<sf::RectangleShape>& platforms) {
    if (!renderTarget) return;
    
    for (const auto& platform : platforms) {
        renderTarget->draw(platform);
    }
    logDebug("Rendered " + std::to_string(platforms.size()) + " platforms");
}



void RenderingSystem::renderPlayer(const Player& player) {
    if (!renderTarget) return;
    
    if (usePlayerPlaceholder) {
        playerPlaceholder.setPosition(player.getPosition());
        renderTarget->draw(playerPlaceholder);
        logDebug("Rendered player placeholder at position (" + 
                std::to_string(player.getPosition().x) + ", " + 
                std::to_string(player.getPosition().y) + ")");
    } else if (playerSprite) {
        playerSprite->setPosition(player.getPosition());
        playerSprite->setScale(sf::Vector2f(spriteScale, spriteScale));
        renderTarget->draw(*playerSprite);
        logDebug("Rendered player sprite at position (" + 
                std::to_string(player.getPosition().x) + ", " + 
                std::to_string(player.getPosition().y) + ")");
    } else {
        logWarning("Player rendering failed: no sprite or placeholder available");
    }
}

void RenderingSystem::renderEnemies(const std::vector<Enemy>& enemies) {
    if (!renderTarget) return;
    
    if (!showEnemies) {
        logDebug("Enemy rendering skipped (showEnemies = false)");
        return;
    }
    
    int enemiesRendered = 0;
    for (const auto& enemy : enemies) {
        if (useEnemyPlaceholder) {
            enemyPlaceholder.setPosition(enemy.getPosition());
            renderTarget->draw(enemyPlaceholder);
            enemiesRendered++;
        } else if (enemySprite) {
            enemySprite->setPosition(enemy.getPosition());
            enemySprite->setScale(sf::Vector2f(spriteScale, spriteScale));
            renderTarget->draw(*enemySprite);
            enemiesRendered++;
        }
    }
    logDebug("Rendered " + std::to_string(enemiesRendered) + " enemies");
}

void RenderingSystem::renderDebugGrid() {
    if (!renderTarget) {
        logError("No render target set for renderDebugGrid()");
        return;
    }
    
    if (!showDebugGrid) {
        logDebug("Debug grid rendering skipped (showDebugGrid = false)");
        return;
    }
    
    // Get the current view from the render target
    sf::View currentView = renderTarget->getView();
    sf::Vector2f viewCenter = currentView.getCenter();
    sf::Vector2f viewSize = currentView.getSize();
    
    float leftX = viewCenter.x - viewSize.x / 2.0f;
    float rightX = viewCenter.x + viewSize.x / 2.0f;
    float topY = viewCenter.y - viewSize.y / 2.0f;
    float bottomY = viewCenter.y + viewSize.y / 2.0f;
    
    // Calculate grid line start and end positions
    int startGridX = static_cast<int>(std::floor(leftX / gridSize));
    int endGridX = static_cast<int>(std::ceil(rightX / gridSize));
    int startGridY = static_cast<int>(std::floor(topY / gridSize));
    int endGridY = static_cast<int>(std::ceil(bottomY / gridSize));
    
    // Create vertex arrays for efficient rendering
    sf::VertexArray gridLines(sf::PrimitiveType::Lines);
    sf::VertexArray axisLines(sf::PrimitiveType::Lines);
    sf::VertexArray originLines(sf::PrimitiveType::Lines);
    
    // Draw vertical grid lines
    for (int x = startGridX; x <= endGridX; x++) {
        float worldX = x * gridSize;
        if (worldX >= leftX && worldX <= rightX) {
            sf::Color lineColor = gridColor;
            
            // Use axis color for major axes (every 10th line or at coordinate 0)
            if (x == 0) {
                // This is the Y-axis (x=0)
                originLines.append(sf::Vertex{{worldX, topY}, gridOriginColor});
                originLines.append(sf::Vertex{{worldX, bottomY}, gridOriginColor});
            } else if (x % 10 == 0) {
                // Major grid line
                axisLines.append(sf::Vertex{{worldX, topY}, gridAxesColor});
                axisLines.append(sf::Vertex{{worldX, bottomY}, gridAxesColor});
            } else {
                // Regular grid line
                gridLines.append(sf::Vertex{{worldX, topY}, lineColor});
                gridLines.append(sf::Vertex{{worldX, bottomY}, lineColor});
            }
        }
    }
    
    // Draw horizontal grid lines
    for (int y = startGridY; y <= endGridY; y++) {
        float worldY = y * gridSize;
        if (worldY >= topY && worldY <= bottomY) {
            sf::Color lineColor = gridColor;
            
            // Use axis color for major axes (every 10th line or at coordinate 0)
            if (y == 0) {
                // This is the X-axis (y=0)
                originLines.append(sf::Vertex{{leftX, worldY}, gridOriginColor});
                originLines.append(sf::Vertex{{rightX, worldY}, gridOriginColor});
            } else if (y % 10 == 0) {
                // Major grid line  
                axisLines.append(sf::Vertex{{leftX, worldY}, gridAxesColor});
                axisLines.append(sf::Vertex{{rightX, worldY}, gridAxesColor});
            } else {
                // Regular grid line
                gridLines.append(sf::Vertex{{leftX, worldY}, lineColor});
                gridLines.append(sf::Vertex{{rightX, worldY}, lineColor});
            }
        }
    }
    
    // Draw the grid (in order: grid, major axes, then origin)
    if (gridLines.getVertexCount() > 0) {
        renderTarget->draw(gridLines);
    }
    if (axisLines.getVertexCount() > 0) {
        renderTarget->draw(axisLines);
    }
    if (originLines.getVertexCount() > 0) {
        renderTarget->draw(originLines);
    }
    
    logDebug("Debug grid rendered with " + std::to_string(gridLines.getVertexCount() + axisLines.getVertexCount() + originLines.getVertexCount()) + " vertices");
}

void RenderingSystem::renderDebugBoxes() {
    // Stub for debug box rendering
    // This would implement collision box visualization
    logDebug("Debug boxes rendering called");
}

void RenderingSystem::renderUI() {
    // Stub for UI rendering
    // This would render health, level text, game over screens, etc.
    logDebug("UI rendering called");
}

void RenderingSystem::renderFPS() {
    // Stub for FPS rendering
    // This would render the FPS counter
    logDebug("FPS rendering called");
}

void RenderingSystem::renderMiniMap() {
    // Stub for mini-map rendering
    // This would render the mini-map with player and enemy positions
    logDebug("Mini-map rendering called");
}



void RenderingSystem::setPlayerSprite(std::unique_ptr<sf::Sprite> sprite) {
    playerSprite = std::move(sprite);
    usePlayerPlaceholder = false;
    logInfo("Player sprite set, placeholder disabled");
}

void RenderingSystem::setEnemySprite(std::unique_ptr<sf::Sprite> sprite) {
    enemySprite = std::move(sprite);
    useEnemyPlaceholder = false;
    logInfo("Enemy sprite set, placeholder disabled");
}

void RenderingSystem::setBackgroundPlaceholder(const sf::RectangleShape& placeholder) {
    backgroundPlaceholder = placeholder;
    logInfo("Background placeholder updated");
}

void RenderingSystem::clearLogFile() {
    if (logFile.is_open()) {
        logFile.close();
    }
    logFile.open(logFileName, std::ios::out | std::ios::trunc);
    if (logFile.is_open()) {
        logInfo("Log file cleared");
    }
}

void RenderingSystem::renderSpriteWithDirection(const sf::Sprite& sprite, const sf::Vector2f& position, bool facingLeft) {
    if (!renderTarget) return;
    
    // Helper method for rendering sprites with direction
    sf::Sprite tempSprite = sprite;
    tempSprite.setPosition(position);
    
    if (facingLeft) {
        // Flip sprite horizontally
        sf::Vector2f scale = tempSprite.getScale();
        scale.x = -std::abs(scale.x);
        tempSprite.setScale(scale);
    }
    
    renderTarget->draw(tempSprite);
    logDebug("Rendered sprite with direction at (" + 
            std::to_string(position.x) + ", " + 
            std::to_string(position.y) + "), facing " + 
            (facingLeft ? "left" : "right"));
}

void RenderingSystem::renderPlaceholderWithDirection(const sf::RectangleShape& placeholder, const sf::Vector2f& position, bool facingLeft) {
    if (!renderTarget) return;
    
    // Helper method for rendering placeholders with direction
    sf::RectangleShape tempPlaceholder = placeholder;
    tempPlaceholder.setPosition(position);
    
    // Placeholders don't need direction changes, but this method maintains consistency
    renderTarget->draw(tempPlaceholder);
    logDebug("Rendered placeholder at (" + 
            std::to_string(position.x) + ", " + 
            std::to_string(position.y) + ")");
}

// Logging helper methods
void RenderingSystem::logDebug(const std::string& message) {
    if (loggingEnabled && logFile.is_open()) {
        logFile << "[" << getCurrentTimestamp() << "] [DEBUG] " << message << std::endl;
        logFile.flush();
    }
}

void RenderingSystem::logInfo(const std::string& message) {
    if (loggingEnabled && logFile.is_open()) {
        logFile << "[" << getCurrentTimestamp() << "] [INFO] " << message << std::endl;
        logFile.flush();
    }
}

void RenderingSystem::logWarning(const std::string& message) {
    if (loggingEnabled && logFile.is_open()) {
        logFile << "[" << getCurrentTimestamp() << "] [WARNING] " << message << std::endl;
        logFile.flush();
    }
}

void RenderingSystem::logError(const std::string& message) {
    if (loggingEnabled && logFile.is_open()) {
        logFile << "[" << getCurrentTimestamp() << "] [ERROR] " << message << std::endl;
        logFile.flush();
    }
}

std::string RenderingSystem::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

// ============================================================================
// Tile Rendering System (moved from TileRenderer)
// ============================================================================

bool RenderingSystem::loadTiles(const std::string& tilesDirectory) {
    tileTextures.clear();
    tileSprites.clear();
    
    logInfo("Loading tiles from: " + tilesDirectory);
    
    if (!fs::exists(tilesDirectory)) {
        logError("Tiles directory does not exist: " + tilesDirectory);
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
        logError("Error reading tiles directory: " + std::string(e.what()));
        return false;
    }
    
    // Sort files to ensure consistent loading order
    std::sort(tileFiles.begin(), tileFiles.end());
    
    if (tileFiles.empty()) {
        logError("No PNG files found in tiles directory");
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
            
            logInfo("Loaded tile: " + fs::path(filePath).filename().string());
        } else {
            logError("Failed to load tile: " + filePath);
        }
    }
    
    if (tileTextures.empty()) {
        logError("No tiles were successfully loaded");
        return false;
    }
    
    // Update the distribution for random tile selection
    updateTileDistribution();
    
    logInfo("Successfully loaded " + std::to_string(tileTextures.size()) + " tiles");
    return true;
}

void RenderingSystem::renderPlatform(sf::RenderWindow& window, const sf::RectangleShape& platform, bool randomize) {
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

void RenderingSystem::renderPlatforms(sf::RenderWindow& window, const std::vector<sf::RectangleShape>& platforms, bool randomize) {
    for (const auto& platform : platforms) {
        renderPlatform(window, platform, randomize);
    }
}

void RenderingSystem::renderTileGrid(sf::RenderWindow& window, const sf::Vector2f& position, const sf::Vector2f& size) {
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

void RenderingSystem::renderBackground(sf::RenderWindow& window, const sf::Sprite& background) {
    window.draw(background);
}

void RenderingSystem::renderEntity(sf::RenderWindow& window, const sf::Sprite& sprite, const sf::Vector2f& position) {
    sf::Sprite tempSprite = sprite;
    tempSprite.setPosition(position);
    window.draw(tempSprite);
}

void RenderingSystem::renderShape(sf::RenderWindow& window, const sf::Shape& shape) {
    window.draw(shape);
}

void RenderingSystem::beginBatch() {
    spriteBatch.clear();
    batchMode = true;
}

void RenderingSystem::endBatch() {
    if (renderTarget) {
        for (const auto& [sprite, position] : spriteBatch) {
            sf::Sprite tempSprite = sprite;
            tempSprite.setPosition(position);
            renderTarget->draw(tempSprite);
        }
    }
    spriteBatch.clear();
    batchMode = false;
}

void RenderingSystem::addToBatch(const sf::Sprite& sprite, const sf::Vector2f& position) {
    if (batchMode) {
        spriteBatch.emplace_back(sprite, position);
    }
}

int RenderingSystem::getRandomTileIndex() {
    if (tileTextures.empty()) return 0;
    return tileDistribution(randomEngine);
}

void RenderingSystem::updateTileDistribution() {
    if (!tileTextures.empty()) {
        tileDistribution = std::uniform_int_distribution<int>(0, static_cast<int>(tileTextures.size() - 1));
    }
}

sf::Sprite& RenderingSystem::getTileSprite(int index) {
    // Ensure index is within bounds
    index = std::max(0, std::min(index, static_cast<int>(tileSprites.size() - 1)));
    return *tileSprites[index];
}

std::vector<RenderingSystem::TilePosition> RenderingSystem::generateTileLayout(
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