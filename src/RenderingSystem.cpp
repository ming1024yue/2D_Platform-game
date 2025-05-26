#include "RenderingSystem.hpp"
#include "Player.hpp"
#include "Enemy.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

RenderingSystem::RenderingSystem(sf::RenderWindow& window) : window(window) {
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
    
    logInfo("Placeholder shapes initialized");
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
    
    // Clear the window
    window.clear(sf::Color::Black);
    
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
    
    // Render lighting if enabled
    if (showLighting) {
        renderLighting();
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
    if (useBackgroundPlaceholder) {
        window.draw(backgroundPlaceholder);
        logDebug("Rendered background placeholder");
    } else {
        renderBackgroundLayers();
        logDebug("Rendered background layers");
    }
}

void RenderingSystem::renderBackgroundLayers() {
    int layersRendered = 0;
    for (auto& layer : backgroundLayers) {
        if (layer.isLoaded && layer.sprite) {
            window.draw(*layer.sprite);
            layersRendered++;
        }
    }
    logDebug("Rendered " + std::to_string(layersRendered) + " background layers");
}

void RenderingSystem::setBackgroundLayers(std::vector<BackgroundLayer>&& layers) {
    backgroundLayers = std::move(layers);
    logInfo("Background layers updated, count: " + std::to_string(backgroundLayers.size()));
}

void RenderingSystem::renderPlatforms(const std::vector<sf::RectangleShape>& platforms) {
    for (const auto& platform : platforms) {
        window.draw(platform);
    }
    logDebug("Rendered " + std::to_string(platforms.size()) + " platforms");
}

void RenderingSystem::renderLadders(const std::vector<sf::RectangleShape>& ladders) {
    for (const auto& ladder : ladders) {
        window.draw(ladder);
    }
    logDebug("Rendered " + std::to_string(ladders.size()) + " ladders");
}

void RenderingSystem::renderPlayer(const Player& player) {
    if (usePlayerPlaceholder) {
        playerPlaceholder.setPosition(player.getPosition());
        window.draw(playerPlaceholder);
        logDebug("Rendered player placeholder at position (" + 
                std::to_string(player.getPosition().x) + ", " + 
                std::to_string(player.getPosition().y) + ")");
    } else if (playerSprite) {
        playerSprite->setPosition(player.getPosition());
        playerSprite->setScale(sf::Vector2f(spriteScale, spriteScale));
        window.draw(*playerSprite);
        logDebug("Rendered player sprite at position (" + 
                std::to_string(player.getPosition().x) + ", " + 
                std::to_string(player.getPosition().y) + ")");
    } else {
        logWarning("Player rendering failed: no sprite or placeholder available");
    }
}

void RenderingSystem::renderEnemies(const std::vector<Enemy>& enemies) {
    if (!showEnemies) {
        logDebug("Enemy rendering skipped (showEnemies = false)");
        return;
    }
    
    int enemiesRendered = 0;
    for (const auto& enemy : enemies) {
        if (useEnemyPlaceholder) {
            enemyPlaceholder.setPosition(enemy.getPosition());
            window.draw(enemyPlaceholder);
            enemiesRendered++;
        } else if (enemySprite) {
            enemySprite->setPosition(enemy.getPosition());
            enemySprite->setScale(sf::Vector2f(spriteScale, spriteScale));
            window.draw(*enemySprite);
            enemiesRendered++;
        }
    }
    logDebug("Rendered " + std::to_string(enemiesRendered) + " enemies");
}

void RenderingSystem::renderDebugGrid() {
    // Stub for debug grid rendering
    // This would implement the grid drawing logic
    logDebug("Debug grid rendering called (grid size: " + std::to_string(gridSize) + ")");
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

void RenderingSystem::renderLighting() {
    // Stub for lighting rendering
    // This would apply lighting effects
    logDebug("Lighting rendering called");
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
    // Helper method for rendering sprites with direction
    sf::Sprite tempSprite = sprite;
    tempSprite.setPosition(position);
    
    if (facingLeft) {
        // Flip sprite horizontally
        sf::Vector2f scale = tempSprite.getScale();
        scale.x = -std::abs(scale.x);
        tempSprite.setScale(scale);
    }
    
    window.draw(tempSprite);
    logDebug("Rendered sprite with direction at (" + 
            std::to_string(position.x) + ", " + 
            std::to_string(position.y) + "), facing " + 
            (facingLeft ? "left" : "right"));
}

void RenderingSystem::renderPlaceholderWithDirection(const sf::RectangleShape& placeholder, const sf::Vector2f& position, bool facingLeft) {
    // Helper method for rendering placeholders with direction
    sf::RectangleShape tempPlaceholder = placeholder;
    tempPlaceholder.setPosition(position);
    
    // Placeholders don't need direction changes, but this method maintains consistency
    window.draw(tempPlaceholder);
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