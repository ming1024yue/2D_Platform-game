#include "Game.hpp"
#include <iostream>
#include <cstdint> // For uint8_t
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <chrono>
#include <ctime>

namespace fs = std::filesystem;

// Helper function for rectangle intersection (for SFML 3.x compatibility)
static bool rectsIntersect(const sf::FloatRect& a, const sf::FloatRect& b) {
    return a.position.x < b.position.x + b.size.x &&
           a.position.x + a.size.x > b.position.x &&
           a.position.y < b.position.y + b.size.y &&
           a.position.y + a.size.y > b.position.y;
}

Game::Game() : window(sf::VideoMode(sf::Vector2u(WINDOW_WIDTH, WINDOW_HEIGHT)), "2D Platform Puzzle Game"),
               player(50.f, WINDOW_HEIGHT - GROUND_HEIGHT - 80.f, physicsSystem), // Pass physicsSystem reference
               playerHit(false),
               playerHitCooldown(0.f),
               currentState(GameState::Playing),
               gameOverText(defaultFont, sf::String("GAME OVER"), 48),
               restartText(defaultFont, sf::String("Press ENTER to restart"), 24),
               fpsText(defaultFont, sf::String("FPS: 0"), 16),
               showMiniMap(true),
               currentLevel(1),
               transitionTimer(0.f),
               levelText(defaultFont, sf::String("Level 1"), 36),
               playerPosition(50.f, WINDOW_HEIGHT / 2.f),
               playerSpeed(200.f),
               isRunning(true),
               previousState(GameState::Playing),
               showBoundingBoxes(false),
               gameSpeed(1.0f),
               platformColor(34, 139, 34),
               playerBorderColor(0, 255, 0),
               enemyBorderColor(255, 0, 0),
               spriteScale(4.0f),
               boundaryBoxHeight(0.67f),
               showEnemies(false),  // Show enemies by default
               showDebugGrid(false),
               gridSize(50.0f),
               gridColor(128, 128, 128, 64),  // Semi-transparent gray
               gridOriginColor(255, 255, 0, 128),  // Semi-transparent yellow
               gridAxesColor(255, 255, 255, 96),   // Semi-transparent white for axes
               fpsUpdateTime(0.0f),
               frameCount(0),
               currentFPS(0.0f),
               showImGuiDemo(false),
               useImGuiInterface(true),
               showAssetManager(false),
               previewAvailable(false) {
    
    // Initialize logging system
    gameLogFile.open(gameLogFileName, std::ios::out | std::ios::app);
    if (gameLogFile.is_open()) {
        logInfo("Game initialized - starting new session");
    }
    
    // Initialize sprite pointers with shared empty texture (created in loadAssets)
    // This is required because sf::Sprite has no default constructor in SFML 3.x
               
    window.setFramerateLimit(FPS);
    

    
    // Initialize view for scrolling
    gameView.setSize(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
    gameView.setCenter(sf::Vector2f(WINDOW_WIDTH / 2.f, WINDOW_HEIGHT / 2.f));
    
    // Initialize UI view
    uiView.setSize(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
    uiView.setCenter(sf::Vector2f(WINDOW_WIDTH / 2.f, WINDOW_HEIGHT / 2.f));
    
    // Initialize mini-map view
    miniMapView.setSize(sf::Vector2f(LEVEL_WIDTH, WINDOW_HEIGHT));
    miniMapView.setCenter(sf::Vector2f(LEVEL_WIDTH / 2.f, WINDOW_HEIGHT / 2.f));
    
    // Set viewport for mini-map view (fixed for SFML 3.x)
    sf::Vector2f viewportPos(
        1.0f - (float)(MINI_MAP_WIDTH + MINI_MAP_MARGIN) / WINDOW_WIDTH,
        1.0f - (float)(MINI_MAP_HEIGHT + MINI_MAP_MARGIN) / WINDOW_HEIGHT
    );
    sf::Vector2f viewportSize(
        (float)MINI_MAP_WIDTH / WINDOW_WIDTH,
        (float)MINI_MAP_HEIGHT / WINDOW_HEIGHT
    );
    miniMapView.setViewport(sf::FloatRect(viewportPos, viewportSize));
    
    window.setView(gameView);
    
    // Initialize NPC manager
    npcManager = std::make_unique<NPC>(assets, renderingSystem);
    
    // Load game assets
    loadAssets();
    
    initializePlatforms();
    initializeNPCs();  // Initialize NPCs after loading assets
    initializeEnemies();
    initializeUI();
    initializeMiniMap();
    
    // Initialize ImGui
    initializeImGui();
    
    // Initialize FPS text
    fpsText.setFillColor(sf::Color::White);
    fpsText.setOutlineColor(sf::Color::Black);
    fpsText.setOutlineThickness(2.0f);
    fpsText.setCharacterSize(24); // Larger text
    fpsText.setPosition(sf::Vector2f(WINDOW_WIDTH - 115, 8)); // Better centered within the background

    // Initialize FPS background
    fpsBackground.setSize(sf::Vector2f(120, 35)); // Larger background
    fpsBackground.setPosition(sf::Vector2f(WINDOW_WIDTH - 125, 5));
    fpsBackground.setFillColor(sf::Color(0, 0, 0, 200)); // More opaque background
    fpsBackground.setOutlineColor(sf::Color(200, 200, 200)); // Lighter outline
    fpsBackground.setOutlineThickness(2.0f); // Thicker outline
    
    // Initialize physics system
    physicsSystem.initialize();
    physicsSystem.initializePlayer(player);
    physicsSystem.initializePlatforms(platforms);
    physicsSystem.initializeEnemies(enemies);
}

void Game::update() {
    if (!window.isOpen()) {
        return;
    }
    
    float deltaTime = clock.restart().asSeconds() * gameSpeed;
    
    // Process ImGui
    if (useImGuiInterface) {
        updateImGui();
    }
    
    // Calculate FPS
    updateFPS();
    
    // Skip game updates when in debug panel mode
    if (currentState == GameState::DebugPanel) {
        return;
    }
    
    // Update view position regardless of game state
    float viewX = std::max(WINDOW_WIDTH / 2.f, 
                      std::min(player.getPosition().x, LEVEL_WIDTH - WINDOW_WIDTH / 2.f));
    gameView.setCenter(sf::Vector2f(viewX, gameView.getCenter().y));
    
    if (currentState == GameState::Playing) {
        // Update player
        player.update(deltaTime, platforms, ladders);
        
        // Update NPCs
        npcManager->updateAll(deltaTime);
        // Update NPC physics
        physicsSystem.updateNPCs(const_cast<std::vector<NPC::NPCData>&>(npcManager->getAllNPCs()));
        
        // Update enemies only if they're visible
        if (showEnemies) {
            for (auto& enemy : enemies) {
                enemy.update(platforms);
            }
        }
        
        // Update physics system
        physicsSystem.update(deltaTime, player, enemies);
        
        // Check for player-enemy collisions only if enemies are visible
        if (showEnemies) {
            checkPlayerEnemyCollision();
        }
        
        // Check for player-NPC collisions
        checkPlayerNPCCollision();
        
        // Check if game is over
        checkGameOver();
        
        // Check if level is complete (reached right edge)
        checkLevelCompletion();
        
        // Check if player should go to previous level (reached left edge)
        if (currentLevel > 1 && player.getPosition().x <= 10.f && player.isOnGround()) {
            // Player has reached the left edge of the level
            currentState = GameState::LevelTransition;
            transitionTimer = LEVEL_TRANSITION_DURATION;
            
            // Set up level transition text
            levelText.setString("Going to Level " + std::to_string(currentLevel - 1));
            
            // Center the text
            sf::FloatRect levelBounds = levelText.getGlobalBounds();
            levelText.setPosition(sf::Vector2f(
                WINDOW_WIDTH / 2.f - levelBounds.size.x / 2.f,
                WINDOW_HEIGHT / 2.f - levelBounds.size.y / 2.f
            ));
        }
        
        // Update UI
        updateUI();
        
        // Update mini-map
        updateMiniMap();
        
        window.setView(gameView);
    } else if (currentState == GameState::LevelTransition) {
        // Handle level transition timer
        transitionTimer -= deltaTime;
        if (transitionTimer <= 0) {
            // Check if we're going forward or backward
            if (player.getPosition().x >= LEVEL_WIDTH - player.getSize().x - 50.f) {
                nextLevel();
            } else if (player.getPosition().x <= 10.f) {
                previousLevel();
            }
        }
    } else if (currentState == GameState::GameOver) {
        // In game over state (either from death or completion)
        // Keep updating certain elements for visual continuity
        updateFPS();
        updateMiniMap();
        
        // Keep the view centered on the final position
        window.setView(gameView);
        
        // Update UI elements
        updateUI();
        
        // Update game over text position to stay centered in view
        sf::Vector2f viewCenter = gameView.getCenter();
        sf::FloatRect gameOverBounds = gameOverText.getGlobalBounds();
        gameOverText.setPosition(sf::Vector2f(
            viewCenter.x - gameOverBounds.size.x / 2.f,
            viewCenter.y - gameOverBounds.size.y / 2.f - 40.f
        ));
        
        sf::FloatRect restartBounds = restartText.getGlobalBounds();
    }
}

void Game::loadAssets() {
    try {
        // Set placeholder flags to true initially
        useBackgroundPlaceholder = true;
        usePlayerPlaceholder = true;
        useEnemyPlaceholder = true;
        
        // Initialize placeholder shapes for drawing - make sure it covers the full screen
        backgroundPlaceholder.setSize(sf::Vector2f(LEVEL_WIDTH, WINDOW_HEIGHT));
        backgroundPlaceholder.setFillColor(sf::Color(100, 180, 100)); // Green
        backgroundPlaceholder.setPosition(sf::Vector2f(0, 0));
        
        playerPlaceholder.setSize(sf::Vector2f(32, 32));
        playerPlaceholder.setFillColor(sf::Color::Blue);
        
        enemyPlaceholder.setSize(sf::Vector2f(32, 32));
        enemyPlaceholder.setFillColor(sf::Color::Red);
        
        // Initialize and load layered background system
        try {
            initializeBackgroundLayers();
            loadBackgroundLayers();
        } catch (const std::exception& e) {
            logError("Failed to load layered backgrounds: " + std::string(e.what()));
            // We'll use the background placeholder instead
        }
        
        // Load character sprites
        try {
            // Try multiple paths to find the player sprite
            std::vector<std::string> playerPaths = {
                "../assets/images/characters/player.png",
                "assets/images/characters/player.png",
                "./assets/images/characters/player.png"
            };
            
            bool playerLoaded = false;
            for (const auto& path : playerPaths) {
                try {
                    assets.loadTexture("player", path);
                    playerLoaded = true;
                    logInfo("Successfully loaded player sprite from: " + path);
                    break;
                } catch (const std::exception& e) {
                    logWarning("Failed to load player sprite from " + path + ": " + std::string(e.what()));
                }
            }
            
            if (playerLoaded) {
                playerSprite = std::make_unique<sf::Sprite>(assets.getTexture("player"));
                usePlayerPlaceholder = false;
            } else {
                logError("Failed to load player sprite from any path");
                // We'll use the player placeholder instead
            }
        } catch (const std::exception& e) {
            logError("Failed to load player sprite: " + std::string(e.what()));
            // We'll use the player placeholder instead
        }
        
        // Load enemy sprites
        try {
            // Try multiple paths to find the enemy sprite
            std::vector<std::string> enemyPaths = {
                "../assets/images/enemies/enemy.png",
                "assets/images/enemies/enemy.png",
                "./assets/images/enemies/enemy.png"
            };
            
            bool enemyLoaded = false;
            for (const auto& path : enemyPaths) {
                try {
                    assets.loadTexture("enemy", path);
                    enemyLoaded = true;
                    logInfo("Successfully loaded enemy sprite from: " + path);
                    break;
                } catch (const std::exception& e) {
                    logWarning("Failed to load enemy sprite from " + path + ": " + std::string(e.what()));
                }
            }
            
            if (enemyLoaded) {
                enemySprite = std::make_unique<sf::Sprite>(assets.getTexture("enemy"));
                useEnemyPlaceholder = false;
            } else {
                logError("Failed to load enemy sprite from any path");
                // We'll use the enemy placeholder instead
            }
        } catch (const std::exception& e) {
            logError("Failed to load enemy sprite: " + std::string(e.what()));
            // We'll use the enemy placeholder instead
        }
        
        // Load fonts
        try {
            assets.loadFont("pixel_font", "assets/fonts/pixel.ttf");
        } catch (const std::exception& e) {
            logWarning("Failed to load font: " + std::string(e.what()));
            // Font errors are handled in initializeUI with fallbacks
        }
        
        // Load sounds
        try {
            assets.loadSoundBuffer("jump", "assets/audio/sfx/jump.wav");
            assets.loadSoundBuffer("hit", "assets/audio/sfx/hit.wav");
        } catch (const std::exception& e) {
            logWarning("Failed to load sound: " + std::string(e.what()));
            // Game can continue without sounds
        }
        
        // Load platform tiles
        try {
            std::vector<std::string> tilePaths = {
                "assets/images/platformer/tiles",
                "./assets/images/platformer/tiles",
                "../assets/images/platformer/tiles"
            };
            
            bool tilesLoaded = false;
            for (const auto& path : tilePaths) {
                            if (renderingSystem.loadTiles(path)) {
                logInfo("Successfully loaded platform tiles from: " + path);
                tilesLoaded = true;
                break;
            } else {
                logWarning("Failed to load tiles from: " + path);
            }
            }
            
            if (!tilesLoaded) {
                logWarning("No platform tiles loaded - platforms will use solid colors");
            }
        } catch (const std::exception& e) {
            logWarning("Failed to load platform tiles: " + std::string(e.what()));
            // Game can continue without tiles
        }
        
    } catch (const std::exception& e) {
        logError("Asset loading error: " + std::string(e.what()));
        logWarning("Game will continue without some assets.");
    }
}

void Game::initializeUI() {
    // Try to load the pixel art font first
    bool fontLoaded = false;
    
    if (font.openFromFile("assets/fonts/pixel.ttf")) {
        fontLoaded = true;
    } else {
        // Fallback to other fonts if pixel font not found
        if (font.openFromFile("assets/fonts/arial.ttf")) {
            fontLoaded = true;
        } else if (font.openFromFile("assets/fonts/roboto.ttf")) {
            fontLoaded = true;
        } else if (font.openFromFile("/Library/Fonts/arial.ttf")) {
            fontLoaded = true;
        } else if (font.openFromFile("/System/Library/Fonts/Supplemental/arial.ttf")) {
            fontLoaded = true;
        }
    }
    
    if (fontLoaded) {
        // Set the font for all text elements
        gameOverText.setFont(font);
        restartText.setFont(font);
        levelText.setFont(font); 
        fpsText.setFont(font);
    }
    
    // Make FPS text more visible - use larger size and bright color
    fpsText.setFillColor(sf::Color::White);  // White is more visible
    fpsText.setOutlineColor(sf::Color::Black);
    fpsText.setOutlineThickness(2.0f);
    fpsText.setCharacterSize(24); // Larger text
    
    // Configure level text
    levelText.setString("Level " + std::to_string(currentLevel));
    levelText.setFillColor(sf::Color::White);
    levelText.setOutlineColor(sf::Color::Black);
    levelText.setOutlineThickness(2.0f);
    
    // Center level text at the top of the screen
    sf::FloatRect levelBounds = levelText.getGlobalBounds();
    levelText.setPosition(sf::Vector2f(
        WINDOW_WIDTH / 2.f - levelBounds.size.x / 2.f,
        20.f
    ));
    
    // Configure game over text
    gameOverText.setFillColor(sf::Color::Red);
    gameOverText.setOutlineColor(sf::Color::Black);
    gameOverText.setOutlineThickness(2.0f);
    gameOverText.setLetterSpacing(2.0f);
    
    // Center game over text
    sf::FloatRect gameOverBounds = gameOverText.getGlobalBounds();
    gameOverText.setPosition(sf::Vector2f(
        WINDOW_WIDTH / 2.f - gameOverBounds.size.x / 2.f,
        WINDOW_HEIGHT / 2.f - gameOverBounds.size.y / 2.f - 40.f
    ));
    
    // Configure restart text
    restartText.setFillColor(sf::Color::White);
    restartText.setOutlineColor(sf::Color::Black);
    restartText.setOutlineThickness(1.0f);
    restartText.setLetterSpacing(1.5f);
    
    // Position restart text below game over text
    sf::FloatRect restartBounds = restartText.getGlobalBounds();
    restartText.setPosition(sf::Vector2f(
        WINDOW_WIDTH / 2.f - restartBounds.size.x / 2.f,
        WINDOW_HEIGHT / 2.f - restartBounds.size.y / 2.f + 40.f
    ));
}

void Game::updateUI() {
    // No UI updates needed since health system is removed
}

void Game::initializePlatforms() {
    // Clear existing platforms
    platforms.clear();
    
    // Create only the ground platform for puzzle-based gameplay
    sf::RectangleShape ground;
    ground.setSize(sf::Vector2f(LEVEL_WIDTH, GROUND_HEIGHT));
    ground.setPosition(sf::Vector2f(0, WINDOW_HEIGHT - GROUND_HEIGHT));
    ground.setFillColor(platformColor);
    platforms.push_back(ground);
    
    // Reinitialize physics system with just the ground platform
    physicsSystem.initialize();
    physicsSystem.initializePlatforms(platforms);
}







void Game::initializeEnemies() {
    // Clear existing enemies first
    enemies.clear();
    
    // Different enemy patterns based on level theme
    if (currentLevel == 1) {
        // Level 1 - Snow Mountain - enemies concentrated in middle section
        initializeSnowEnemies();
    } else {
        // Level 2+ - Snow Forest - forest-like enemy pattern adapted for snow
        initializeSnowForestEnemies();
    }
    
    // Apply level difficulty scaling - enemies get faster in higher levels
    float speedMultiplier = 1.0f + (currentLevel * 0.1f); // 10% faster per level
    
    // Force enemies to start moving right and scale speed
    for (auto& enemy : enemies) {
        // Set a positive initial velocity and reset position if too close to left edge
        sf::Vector2f pos = enemy.getPosition();
        if (pos.x < 50.0f) {
            pos.x = 100.0f; // Move away from edge
            enemy.setPosition(pos);
        }
        
        // Start velocity to the right and scale by level
        enemy.setVelocity(sf::Vector2f(2.0f * speedMultiplier, 0.0f));
    }
    
    // Debug info
    logDebug("Total enemies created: " + std::to_string(enemies.size()));
}



void Game::initializeSnowEnemies() {
    // Snow level has fewer ground enemies due to the cold
    enemies.push_back(Enemy(500.f, WINDOW_HEIGHT - GROUND_HEIGHT - 30.f, 150.f));
    
    // First section climbing area has few enemies
    enemies.push_back(Enemy(280.f, 370.f, 60.f));
    
    // Middle section - icy platforms have more enemies
    for (int i = 0; i < 4; i++) {
        float x = 850.f + i * 350.f;
        enemies.push_back(Enemy(x, 220.f, 100.f));
    }
    
    // Final descending section
    enemies.push_back(Enemy(2300.f, 170.f, 80.f));
    enemies.push_back(Enemy(2500.f, 220.f, 80.f));
}

void Game::initializeSnowForestEnemies() {
    // Snow Forest enemies - similar to forest but adapted for snow theme
    
    // Enemy on ground
    enemies.push_back(Enemy(400.f, WINDOW_HEIGHT - GROUND_HEIGHT - 30.f, 180.f));
    
    // Enemy on first platform
    enemies.push_back(Enemy(380.f, 370.f, 80.f));
    
    // Enemy on second platform
    enemies.push_back(Enemy(180.f, 270.f, 80.f));
    
    // Middle section enemies - slightly different positions than forest
    enemies.push_back(Enemy(1000.f, 320.f, 100.f));
    enemies.push_back(Enemy(1420.f, 220.f, 120.f));  // Adjusted for new platform positions
    enemies.push_back(Enemy(1820.f, 350.f, 120.f));
    
    // End section enemy
    enemies.push_back(Enemy(2620.f, 290.f, 120.f));  // Adjusted for new platform height
}

void Game::checkGameOver() {
    // Only check for game over if player is not jumping
    if (player.isJumping()) {
        return;
    }
    
    // Only reset game when player falls off the level completely and is not jumping
    float fallThreshold = WINDOW_HEIGHT + 200.0f; // Give extra room below window
    
    if (player.getPosition().y > fallThreshold && !player.isJumping()) {
        currentState = GameState::GameOver;
        
        // Position game over text
        sf::FloatRect gameOverBounds = gameOverText.getGlobalBounds();
        gameOverText.setPosition(sf::Vector2f(
            WINDOW_WIDTH / 2.f - gameOverBounds.size.x / 2.f,
            WINDOW_HEIGHT / 2.f - gameOverBounds.size.y / 2.f - 40.f
        ));
        
        // Position restart text
        sf::FloatRect restartBounds = restartText.getGlobalBounds();
        restartText.setPosition(sf::Vector2f(
            WINDOW_WIDTH / 2.f - restartBounds.size.x / 2.f,
            WINDOW_HEIGHT / 2.f - restartBounds.size.y / 2.f + 40.f
        ));
    }
}

void Game::resetGame() {
    // Reset player
    player.reset(50.f, WINDOW_HEIGHT - GROUND_HEIGHT - 40.f); // Start player higher above the ground
    
    // Set player collision box
    player.setCollisionBoxSize(sf::Vector2f(56.f, 56.f)); // Scaled up from 28x28 to match 4x scale
    
    // Center the collision box on the sprite (64x64 with 4x scale from 32x32)
    player.setCollisionBoxOffset(sf::Vector2f(
        (64.f - 56.f) / 2.f,  // Center horizontally (64-56)/2 = 4
        (64.f - 56.f) / 2.f   // Center vertically (64-56)/2 = 4
    ));
    
    // Reset view
    gameView.setCenter(sf::Vector2f(WINDOW_WIDTH / 2.f, WINDOW_HEIGHT / 2.f));
    
    // Reset game state
    playerHit = false;
    playerHitCooldown = 0.f;
    currentState = GameState::Playing;
    
    // Reinitialize everything except NPCs
    initializePlatforms();
    initializeEnemies();
    initializeUI();
    initializeMiniMap();
    
    // Initialize physics system with centered collision box
    physicsSystem.initialize();
    physicsSystem.setPlayerCollisionSize(0.875f, 0.875f); // 28/32 = 0.875 (collision box is 28x28 on 32x32 sprite)
    physicsSystem.setPlayerCollisionOffset(0.0625f, 0.0625f); // 2/32 = 0.0625 (offset by 2 pixels on each side)
    physicsSystem.initializePlayer(player);
    physicsSystem.initializePlatforms(platforms);
    physicsSystem.initializeEnemies(enemies);
    
    // Make sure NPCs are properly initialized in physics system
    if (npcManager) {
        physicsSystem.initializeNPCs(npcManager->getAllNPCs());
    }
}

void Game::checkPlayerEnemyCollision() {
    if (playerHit) {
        playerHitCooldown -= 1.0f / FPS; // Decrement cooldown
        if (playerHitCooldown <= 0) {
            playerHit = false;
        }
        // Blink player during invulnerability
        if (static_cast<int>(playerHitCooldown * 10) % 2 == 0) {
            // This makes the player periodically invisible for blinking effect
            return;
        }
    }
    
    // Check for collisions with enemies
    for (const auto& enemy : enemies) {
        if (!playerHit && rectsIntersect(player.getGlobalBounds(), enemy.getGlobalBounds())) {
            // Player hit by enemy
            playerHit = true;
            playerHitCooldown = HIT_COOLDOWN;
            
            // Push player away from enemy
            if (player.getPosition().x < enemy.getGlobalBounds().position.x) {
                // Push player left
                player.setPosition(sf::Vector2f(player.getPosition().x - 50.f, player.getPosition().y - 30.f));
            } else {
                // Push player right
                player.setPosition(sf::Vector2f(player.getPosition().x + 50.f, player.getPosition().y - 30.f));
            }
            
            break;
        }
    }
}

void Game::updateMiniMap() {
    // Calculate scaling factors to properly fill mini-map
    // Use slightly more aggressive scaling to fill the mini-map completely
    float scaleX = (float)(MINI_MAP_WIDTH - 8) / LEVEL_WIDTH;
    float scaleY = (float)(MINI_MAP_HEIGHT - 8) / WINDOW_HEIGHT;
    
    // Update player icon position
    sf::Vector2f playerPos = player.getPosition();
    sf::Vector2f miniPlayerPos(
        playerPos.x * scaleX,
        playerPos.y * scaleY
    );
    miniMapPlayerIcon.setPosition(miniPlayerPos);
    
    // Update enemy positions
    for (size_t i = 0; i < enemies.size() && i < miniMapEnemies.size(); i++) {
        sf::FloatRect bounds = enemies[i].getGlobalBounds();
        sf::Vector2f miniEnemyPos(
            bounds.position.x * scaleX,
            bounds.position.y * scaleY
        );
        miniMapEnemies[i].setPosition(miniEnemyPos);
    }
}





void Game::updateFPS() {
    // Increment frame counter
    frameCount++;
    
    // Accumulate time
    fpsUpdateTime += fpsClock.restart().asSeconds();
    
    // Update FPS counter approximately once per quarter second for more frequent updates
    if (fpsUpdateTime >= 0.25f) {
        // Calculate FPS
        currentFPS = static_cast<float>(frameCount) / fpsUpdateTime;
        
        // Update FPS text with one decimal place precision
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << "FPS: " << currentFPS;
        fpsText.setString(ss.str());
        
        // Reset counters
        frameCount = 0;
        fpsUpdateTime = 0.0f;
    }
}

void Game::drawFPS() {
    // Make sure we're in UI view
    window.setView(uiView);
    
    // Draw the background first
    window.draw(fpsBackground);
    
    // Draw FPS text in the top-right corner
    window.draw(fpsText);
}

void Game::initializeMiniMap() {
    // Create mini-map border
    miniMapBorder.setSize(sf::Vector2f(MINI_MAP_WIDTH, MINI_MAP_HEIGHT));
    miniMapBorder.setPosition(sf::Vector2f(
        WINDOW_WIDTH - MINI_MAP_WIDTH - MINI_MAP_MARGIN,
        WINDOW_HEIGHT - MINI_MAP_HEIGHT - MINI_MAP_MARGIN
    ));
    miniMapBorder.setFillColor(sf::Color(0, 0, 0, 100)); // Semi-transparent black
    miniMapBorder.setOutlineColor(sf::Color::White);
    miniMapBorder.setOutlineThickness(2.f);
    
    // Create player icon for mini-map - larger for better visibility
    miniMapPlayerIcon.setSize(sf::Vector2f(8.f, 8.f));
    miniMapPlayerIcon.setFillColor(sf::Color::Yellow);
    
    // Calculate scaling factors to properly fill mini-map
    // Use slightly more aggressive scaling to fill the mini-map completely
    float scaleX = (float)(MINI_MAP_WIDTH - 8) / LEVEL_WIDTH;
    float scaleY = (float)(MINI_MAP_HEIGHT - 8) / WINDOW_HEIGHT;
    
    // Create platform representations for mini-map
    miniMapPlatforms.clear();
    for (const auto& platform : platforms) {
        sf::RectangleShape miniPlatform;
        
        miniPlatform.setSize(sf::Vector2f(
            platform.getSize().x * scaleX,
            platform.getSize().y * scaleY
        ));
        
        // Fix position setting for SFML 3.x
        sf::Vector2f platformPos(
            platform.getPosition().x * scaleX,
            platform.getPosition().y * scaleY
        );
        miniPlatform.setPosition(platformPos);
        
        miniPlatform.setFillColor(sf::Color::Green);
        miniMapPlatforms.push_back(miniPlatform);
    }
    
    // Create ladder representations for mini-map
    miniMapLadders.clear();
    for (const auto& ladder : ladders) {
        sf::RectangleShape miniLadder;
        
        miniLadder.setSize(sf::Vector2f(
            ladder.getSize().x * scaleX,
            ladder.getSize().y * scaleY
        ));
        
        // Fix position setting for SFML 3.x
        sf::Vector2f ladderPos(
            ladder.getPosition().x * scaleX,
            ladder.getPosition().y * scaleY
        );
        miniLadder.setPosition(ladderPos);
        
        miniLadder.setFillColor(sf::Color(139, 69, 19)); // Brown
        miniMapLadders.push_back(miniLadder);
    }
    
    // Create enemy representations for mini-map
    miniMapEnemies.clear();
    for (const auto& enemy : enemies) {
        sf::RectangleShape miniEnemy;
        
        sf::FloatRect bounds = enemy.getGlobalBounds();
        miniEnemy.setSize(sf::Vector2f(8.f, 8.f)); // Larger for better visibility
        
        // Fix position setting for SFML 3.x
        sf::Vector2f enemyPos(
            bounds.position.x * scaleX,
            bounds.position.y * scaleY
        );
        miniEnemy.setPosition(enemyPos);
        
        miniEnemy.setFillColor(sf::Color::Red);
        miniMapEnemies.push_back(miniEnemy);
    }
}

void Game::checkLevelCompletion() {
    // Only check for level completion if player is on ground
    if (!player.isOnGround()) {
        return;
    }
    
    // Check if player has reached the end of the level (right edge)
    if (player.getPosition().x >= LEVEL_WIDTH - player.getSize().x - 50.f) {
        // Handle differently based on current level
        if (currentLevel == 1) {
            // Player has reached the end of level 1
            currentState = GameState::LevelTransition;
            transitionTimer = LEVEL_TRANSITION_DURATION;
            
            // Set up level transition text
            levelText.setString("Level " + std::to_string(currentLevel) + " Completed!");
            
            // Center the text
            sf::FloatRect levelBounds = levelText.getGlobalBounds();
            levelText.setPosition(sf::Vector2f(
                WINDOW_WIDTH / 2.f - levelBounds.size.x / 2.f,
                WINDOW_HEIGHT / 2.f - levelBounds.size.y / 2.f
            ));
        } else if (currentLevel == 2) {
            // Player has completed the final level
            currentState = GameState::GameOver;
            
            // Update game over text to show victory
            gameOverText.setString("CONGRATULATIONS!");
            gameOverText.setFillColor(sf::Color::Green);
            
            // Position game over text
            sf::FloatRect gameOverBounds = gameOverText.getGlobalBounds();
            gameOverText.setPosition(sf::Vector2f(
                WINDOW_WIDTH / 2.f - gameOverBounds.size.x / 2.f,
                WINDOW_HEIGHT / 2.f - gameOverBounds.size.y / 2.f - 40.f
            ));
            
            // Update restart text
            restartText.setString("Press ENTER to play again");
            sf::FloatRect restartBounds = restartText.getGlobalBounds();
            restartText.setPosition(sf::Vector2f(
                WINDOW_WIDTH / 2.f - restartBounds.size.x / 2.f,
                WINDOW_HEIGHT / 2.f - restartBounds.size.y / 2.f + 40.f
            ));
            
            // Log completion
            logInfo("Player completed the final level!");
        }
    }
}

void Game::nextLevel() {
    // Only go to level 2 if we're on level 1
    if (currentLevel >= 2) {
        logWarning("Already at final level (2), cannot go to next level");
        return;
    }
    
    // Increment level (will be 2)
    currentLevel++;
    
    // Reset player position to left side of level
    player.setPosition(sf::Vector2f(50.f, WINDOW_HEIGHT / 2.f));
    
    // Reset view
    gameView.setCenter(sf::Vector2f(WINDOW_WIDTH / 2.f, WINDOW_HEIGHT / 2.f));
    
    // Reset game state
    currentState = GameState::Playing;
    
    // Update level text
    levelText.setString("Level " + std::to_string(currentLevel));
    sf::FloatRect levelBounds = levelText.getGlobalBounds();
    levelText.setPosition(sf::Vector2f(
        WINDOW_WIDTH / 2.f - levelBounds.size.x / 2.f,
        20.f
    ));

    // Initialize level-specific NPCs
    initializeNPCs();
    
    // Change background and theme for Snow Forest (Level 2)
    try {
        std::string levelBackgroundPath = "assets/images/backgrounds/snow_forest/snow_forest_background.png";
        platformColor = sf::Color(180, 200, 240); // Slightly different blue for snow forest
        std::vector<std::string> alternativePaths = {
            "assets/images/backgrounds/snow_forest_background.png",
            "assets/images/backgrounds/snow/snow_background.png",
            "assets/images/backgrounds/background.png",
            "../assets/images/backgrounds/background.png"
        };
        
        // Try to load the level-specific background
        bool loaded = false;
        try {
            assets.loadTexture("background", levelBackgroundPath);
            logInfo("Successfully loaded background: " + levelBackgroundPath);
            loaded = true;
        } 
        catch (const std::exception& e) {
            logError("Failed to load primary background: " + std::string(e.what()));
            
            // Try alternative paths
            for (const auto& path : alternativePaths) {
                try {
                    assets.loadTexture("background", path);
                    logInfo("Successfully loaded alternative background: " + path);
                    loaded = true;
                    break;
                } catch (const std::exception& innerE) {
                    logWarning("Failed to load alternative background from " + path + ": " + std::string(innerE.what()));
                }
            }
        }
        
        // Reload the layered background system for the new level
        if (loaded) {
            loadBackgroundLayers();
            logInfo("Reloaded layered backgrounds for level " + std::to_string(currentLevel));
        }
        else {
            useBackgroundPlaceholder = true;
            backgroundPlaceholder.setFillColor(sf::Color(200, 220, 255)); // Light blue for snow theme
        }
    } catch (const std::exception& e) {
        logError("Failed to load background for level " + std::to_string(currentLevel) + ": " + std::string(e.what()));
        useBackgroundPlaceholder = true;
        backgroundPlaceholder.setFillColor(sf::Color(200, 220, 255)); // Light blue for snow theme
    }
    
    // Reinitialize game elements
    initializePlatforms();
    initializeEnemies();
    initializeUI();
    initializeMiniMap();
    
    // Add extra enemies for level 2
    for (int i = 0; i < 2; i++) {
        float x = 500.f + (i * 400.f);  // Space them out
        float y = WINDOW_HEIGHT - 70.f; // On the ground
        float patrolDistance = 180.f; // Fixed patrol distance for level 2
        enemies.push_back(Enemy(x, y, patrolDistance));
    }
    
    // Puzzle-focused physics
    physicsSystem.setGravity(15.0f);
    physicsSystem.setJumpForce(200.f);
    
    // Initialize physics system
    physicsSystem.initialize();
    physicsSystem.initializePlayer(player);
    physicsSystem.initializePlatforms(platforms);
    physicsSystem.initializeEnemies(enemies);
    
    logInfo("Advanced to Snow Forest (Level 2)");
}

void Game::previousLevel() {
    // Don't go below level 1
    if (currentLevel <= 1) {
        logWarning("Already at level 1, cannot go to previous level");
        return;
    }
    
    // Decrement level (will be 1)
    currentLevel--;
    
    // Reset player position to right side of level (since we're going backwards)
    player.setPosition(sf::Vector2f(LEVEL_WIDTH - 100.f, WINDOW_HEIGHT / 2.f));
    
    // Reset view
    gameView.setCenter(sf::Vector2f(WINDOW_WIDTH / 2.f, WINDOW_HEIGHT / 2.f));
    
    // Reset game state
    currentState = GameState::Playing;
    
    // Update level text
    levelText.setString("Level " + std::to_string(currentLevel));
    sf::FloatRect levelBounds = levelText.getGlobalBounds();
    levelText.setPosition(sf::Vector2f(
        WINDOW_WIDTH / 2.f - levelBounds.size.x / 2.f,
        20.f
    ));

    // Initialize level-specific NPCs
    initializeNPCs();
    
    // Change background and theme for Snow Mountain (Level 1)
    try {
        std::string levelBackgroundPath = "assets/images/backgrounds/snow/snow_background.png";
        platformColor = sf::Color(200, 220, 255); // Light blue for snow mountain
        std::vector<std::string> alternativePaths = {
            "assets/images/backgrounds/snow_background.png",
            "assets/images/backgrounds/background.png",
            "../assets/images/backgrounds/background.png"
        };
        
        // Try to load the level-specific background
        bool loaded = false;
        try {
            assets.loadTexture("background", levelBackgroundPath);
            logInfo("Successfully loaded background: " + levelBackgroundPath);
            loaded = true;
        } 
        catch (const std::exception& e) {
            logError("Failed to load primary background: " + std::string(e.what()));
            
            // Try alternative paths
            for (const auto& path : alternativePaths) {
                try {
                    assets.loadTexture("background", path);
                    logInfo("Successfully loaded alternative background: " + path);
                    loaded = true;
                    break;
                } catch (const std::exception& innerE) {
                    logWarning("Failed to load alternative background from " + path + ": " + std::string(innerE.what()));
                }
            }
        }
        
        // Reload the layered background system for the new level
        if (loaded) {
            loadBackgroundLayers();
            logInfo("Reloaded layered backgrounds for level " + std::to_string(currentLevel));
        }
        else {
            useBackgroundPlaceholder = true;
            backgroundPlaceholder.setFillColor(sf::Color(200, 220, 255)); // Light blue for snow theme
        }
    } catch (const std::exception& e) {
        logError("Failed to load background for level " + std::to_string(currentLevel) + ": " + std::string(e.what()));
        useBackgroundPlaceholder = true;
        backgroundPlaceholder.setFillColor(sf::Color(200, 220, 255)); // Light blue for snow theme
    }
    
    // Reinitialize game elements
    initializePlatforms();
    initializeEnemies();
    initializeUI();
    initializeMiniMap();
    
    // Puzzle-focused physics
    physicsSystem.setGravity(15.0f);
    physicsSystem.setJumpForce(200.f);
    
    // Initialize physics system
    physicsSystem.initialize();
    physicsSystem.initializePlayer(player);
    physicsSystem.initializePlatforms(platforms);
    physicsSystem.initializeEnemies(enemies);
    
    logInfo("Returned to Snow Mountain (Level 1)");
}

void Game::updateImGui() {
    try {
        // SFML 3.0 ImGui update
        sf::Time deltaTime = imguiClock.restart();
        ImGui::SFML::Update(window, deltaTime);
        
        // 1. Main game settings window
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("Game Settings", &useImGuiInterface)) {
            // Tab bar for organizing settings
            if (ImGui::BeginTabBar("SettingsTabs")) {
                // Graphics tab
                if (ImGui::BeginTabItem("Graphics")) {

                    ImGui::Checkbox("Show Bounding Boxes", &showBoundingBoxes);
                    ImGui::Checkbox("Show Mini-map", &showMiniMap);
                    
                    // Debug grid controls
                    ImGui::Separator();
                    ImGui::Text("Debug Grid");
                    if (ImGui::Checkbox("Show Debug Grid", &showDebugGrid)) {
                        renderingSystem.setShowDebugGrid(showDebugGrid);
                    }
                    
                    if (showDebugGrid) {
                        if (ImGui::SliderFloat("Grid Size", &gridSize, 10.0f, 200.0f, "%.0f")) {
                            renderingSystem.setGridSize(gridSize);
                        }
                        
                        // Grid color controls
                        float gridColorArr[4] = {
                            gridColor.r / 255.0f,
                            gridColor.g / 255.0f,
                            gridColor.b / 255.0f,
                            gridColor.a / 255.0f
                        };
                        if (ImGui::ColorEdit4("Grid Color", gridColorArr)) {
                            gridColor.r = static_cast<uint8_t>(gridColorArr[0] * 255);
                            gridColor.g = static_cast<uint8_t>(gridColorArr[1] * 255);
                            gridColor.b = static_cast<uint8_t>(gridColorArr[2] * 255);
                            gridColor.a = static_cast<uint8_t>(gridColorArr[3] * 255);
                            renderingSystem.setGridColor(gridColor);
                        }
                        
                        // Origin axes color
                        float originColorArr[4] = {
                            gridOriginColor.r / 255.0f,
                            gridOriginColor.g / 255.0f,
                            gridOriginColor.b / 255.0f,
                            gridOriginColor.a / 255.0f
                        };
                        if (ImGui::ColorEdit4("Origin Color", originColorArr)) {
                            gridOriginColor.r = static_cast<uint8_t>(originColorArr[0] * 255);
                            gridOriginColor.g = static_cast<uint8_t>(originColorArr[1] * 255);
                            gridOriginColor.b = static_cast<uint8_t>(originColorArr[2] * 255);
                            gridOriginColor.a = static_cast<uint8_t>(originColorArr[3] * 255);
                            renderingSystem.setGridOriginColor(gridOriginColor);
                        }
                        
                        // Major axes color
                        float axesColorArr[4] = {
                            gridAxesColor.r / 255.0f,
                            gridAxesColor.g / 255.0f,
                            gridAxesColor.b / 255.0f,
                            gridAxesColor.a / 255.0f
                        };
                        if (ImGui::ColorEdit4("Major Axes Color", axesColorArr)) {
                            gridAxesColor.r = static_cast<uint8_t>(axesColorArr[0] * 255);
                            gridAxesColor.g = static_cast<uint8_t>(axesColorArr[1] * 255);
                            gridAxesColor.b = static_cast<uint8_t>(axesColorArr[2] * 255);
                            gridAxesColor.a = static_cast<uint8_t>(axesColorArr[3] * 255);
                            renderingSystem.setGridAxesColor(gridAxesColor);
                        }
                    }
                    
                    ImGui::Separator();
                    ImGui::SliderFloat("Sprite Scale", &spriteScale, 1.0f, 8.0f);
                    
                    // Color pickers
                    float platformColorArr[3] = {
                        platformColor.r / 255.0f, 
                        platformColor.g / 255.0f, 
                        platformColor.b / 255.0f
                    };
                    if (ImGui::ColorEdit3("Platform Color", platformColorArr)) {
                        platformColor.r = static_cast<uint8_t>(platformColorArr[0] * 255);
                        platformColor.g = static_cast<uint8_t>(platformColorArr[1] * 255);
                        platformColor.b = static_cast<uint8_t>(platformColorArr[2] * 255);
                    }
                    
                    // Background layers section
                    ImGui::Separator();
                    ImGui::Text("Background Layers");
                    
                    // Show background layer status
                    ImGui::Text("Background: %s", useBackgroundPlaceholder ? "Using placeholder" : "Using layers");
                    ImGui::Text("Loaded layers: %zu", backgroundLayers.size());
                    
                    // Layer information
                    for (size_t i = 0; i < backgroundLayers.size(); i++) {
                        const auto& layer = backgroundLayers[i];
                        ImGui::Text("%s: %s (speed: %.1f)", 
                                   layer.name.c_str(), 
                                   layer.isLoaded ? "Loaded" : "Missing",
                                   layer.parallaxSpeed);
                        
                        // Show layer controls if loaded
                        if (layer.isLoaded) {
                            ImGui::SameLine();
                            std::string buttonLabel = "Reload##" + layer.name;
                            if (ImGui::SmallButton(buttonLabel.c_str())) {
                                // Reload just this layer
                                loadBackgroundLayers();
                            }
                        }
                    }
                    
                    // Reload all layers button
                    if (ImGui::Button("Reload All Background Layers")) {
                        loadBackgroundLayers();
                    }
                    
                    ImGui::EndTabItem();
                }
                
                // Gameplay tab
                if (ImGui::BeginTabItem("Gameplay")) {
                    ImGui::SliderFloat("Game Speed", &gameSpeed, 0.1f, 2.0f);
                    ImGui::SliderFloat("Player Speed", &playerSpeed, 50.0f, 400.0f);
                    
                    ImGui::Separator();
                    ImGui::Text("Testing Controls");
                    ImGui::Checkbox("Show Enemies", &showEnemies);
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Toggle enemy visibility for easier testing");
                    }
                    
                    ImGui::Separator();
                    ImGui::Text("Level Control");
                    
                    // Current level display
                    ImGui::Text("Current Level: %d", currentLevel);
                    
                    // Level selection
                    static int selectedLevel = currentLevel;
                    if (ImGui::SliderInt("Select Level", &selectedLevel, 1, 2)) {
                        // Level will be changed when "Jump to Level" button is pressed
                    }
                    
                    // Jump to level button
                    if (ImGui::Button("Jump to Level")) {
                        jumpToLevel(selectedLevel);
                        // Sync the slider with current level after jumping
                        selectedLevel = currentLevel;
                    }
                    
                    ImGui::SameLine();
                    
                    // Quick level buttons
                    if (ImGui::Button("Level 1 (Snow Mountain)")) {
                        jumpToLevel(1);
                        selectedLevel = 1;
                    }
                    
                    ImGui::SameLine();
                    
                    if (ImGui::Button("Level 2 (Snow Forest)")) {
                        jumpToLevel(2);
                        selectedLevel = 2;
                    }
                    
                    // Level theme info
                    ImGui::Separator();
                    ImGui::Text("Level Themes:");
                    ImGui::BulletText("Level 1 = Snow Mountain (Puzzle challenges)");
                    ImGui::BulletText("Level 2 = Snow Forest (Puzzle challenges)");
                    
                    ImGui::Separator();
                    
                    if (ImGui::Button("Reset Level")) {
                        resetGame();
                    }
                    
                    ImGui::EndTabItem();
                }
                
                // Physics tab
                if (ImGui::BeginTabItem("Physics")) {
                    // Collision settings
                    ImGui::Text("Collision Settings");
                    ImGui::Separator();
                    
                    // Player collision box settings
                    ImGui::Text("Player Collision:");
                    float playerWidth = physicsSystem.getPlayerCollisionWidth();
                    float playerHeight = physicsSystem.getPlayerCollisionHeight();
                    float playerOffsetX = physicsSystem.getPlayerOffsetX();
                    float playerOffsetY = physicsSystem.getPlayerOffsetY();
                    float playerBounce = physicsSystem.getPlayerBounceFactor();
                    
                    if (ImGui::SliderFloat("Player Width", &playerWidth, 0.5f, 1.5f, "%.2f")) {
                        physicsSystem.setPlayerCollisionSize(playerWidth, playerHeight);
                    }
                    if (ImGui::SliderFloat("Player Height", &playerHeight, 0.5f, 1.5f, "%.2f")) {
                        physicsSystem.setPlayerCollisionSize(playerWidth, playerHeight);
                    }
                    if (ImGui::SliderFloat("Player Offset X", &playerOffsetX, 0.0f, 1.0f, "%.2f")) {
                        physicsSystem.setPlayerCollisionOffset(playerOffsetX, playerOffsetY);
                    }
                    if (ImGui::SliderFloat("Player Offset Y", &playerOffsetY, 0.0f, 1.0f, "%.2f")) {
                        physicsSystem.setPlayerCollisionOffset(playerOffsetX, playerOffsetY);
                    }
                    if (ImGui::SliderFloat("Player Bounce", &playerBounce, 0.0f, 1.0f, "%.2f")) {
                        physicsSystem.setPlayerBounceFactor(playerBounce);
                    }
                    
                    ImGui::Spacing();
                    
                    // Enemy collision box settings
                    ImGui::Text("Enemy Collision:");
                    float enemyWidth = physicsSystem.getEnemyCollisionWidth();
                    float enemyHeight = physicsSystem.getEnemyCollisionHeight();
                    float enemyOffsetX = physicsSystem.getEnemyOffsetX();
                    float enemyOffsetY = physicsSystem.getEnemyOffsetY();
                    float enemyBounce = physicsSystem.getEnemyBounceFactor();
                    
                    if (ImGui::SliderFloat("Enemy Width", &enemyWidth, 0.5f, 1.5f, "%.2f")) {
                        physicsSystem.setEnemyCollisionSize(enemyWidth, enemyHeight);
                    }
                    if (ImGui::SliderFloat("Enemy Height", &enemyHeight, 0.5f, 1.5f, "%.2f")) {
                        physicsSystem.setEnemyCollisionSize(enemyWidth, enemyHeight);
                    }
                    if (ImGui::SliderFloat("Enemy Offset X", &enemyOffsetX, 0.0f, 1.0f, "%.2f")) {
                        physicsSystem.setEnemyCollisionOffset(enemyOffsetX, enemyOffsetY);
                    }
                    if (ImGui::SliderFloat("Enemy Offset Y", &enemyOffsetY, 0.0f, 1.0f, "%.2f")) {
                        physicsSystem.setEnemyCollisionOffset(enemyOffsetX, enemyOffsetY);
                    }
                    if (ImGui::SliderFloat("Enemy Bounce", &enemyBounce, 0.0f, 1.0f, "%.2f")) {
                        physicsSystem.setEnemyBounceFactor(enemyBounce);
                    }
                    
                    ImGui::Spacing();
                    
                    // NPC collision box settings
                    ImGui::Text("NPC Collision:");
                    float npcWidth = physicsSystem.getNPCCollisionWidth();
                    float npcHeight = physicsSystem.getNPCCollisionHeight();
                    float npcOffsetX = physicsSystem.getNPCOffsetX();
                    float npcOffsetY = physicsSystem.getNPCOffsetY();
                    float npcBounce = physicsSystem.getNPCBounceFactor();
                    
                    if (ImGui::SliderFloat("NPC Width", &npcWidth, 0.5f, 1.5f, "%.2f")) {
                        physicsSystem.setNPCCollisionSize(npcWidth, npcHeight);
                    }
                    if (ImGui::SliderFloat("NPC Height", &npcHeight, 0.5f, 1.5f, "%.2f")) {
                        physicsSystem.setNPCCollisionSize(npcWidth, npcHeight);
                    }
                    if (ImGui::SliderFloat("NPC Offset X", &npcOffsetX, 0.0f, 1.0f, "%.2f")) {
                        physicsSystem.setNPCCollisionOffset(npcOffsetX, npcOffsetY);
                    }
                    if (ImGui::SliderFloat("NPC Offset Y", &npcOffsetY, 0.0f, 1.0f, "%.2f")) {
                        physicsSystem.setNPCCollisionOffset(npcOffsetX, npcOffsetY);
                    }
                    if (ImGui::SliderFloat("NPC Bounce", &npcBounce, 0.0f, 1.0f, "%.2f")) {
                        physicsSystem.setNPCBounceFactor(npcBounce);
                    }
                    
                    ImGui::Spacing();
                    
                    // Platform collision settings
                    ImGui::Text("Platform Physics:");
                    float platformFriction = physicsSystem.getPlatformFriction();
                    bool useOneWayPlatforms = physicsSystem.getUseOneWayPlatforms();
                    
                    if (ImGui::SliderFloat("Platform Friction", &platformFriction, 0.0f, 1.0f, "%.2f")) {
                        physicsSystem.setPlatformFriction(platformFriction);
                    }
                    if (ImGui::Checkbox("One-Way Platforms", &useOneWayPlatforms)) {
                        physicsSystem.setUseOneWayPlatforms(useOneWayPlatforms);
                    }
                    
                    ImGui::Spacing();
                    
                    // Physics parameters
                    ImGui::Text("Physics Parameters:");
                    float gravity = physicsSystem.getGravity();
                    float terminalVelocity = physicsSystem.getTerminalVelocity();
                    float jumpForce = physicsSystem.getJumpForce();
                    
                    if (ImGui::SliderFloat("Gravity", &gravity, 0.0f, 2000.0f, "%.1f")) {
                        physicsSystem.setGravity(gravity);
                    }
                    if (ImGui::SliderFloat("Terminal Velocity", &terminalVelocity, 100.0f, 2000.0f, "%.1f")) {
                        physicsSystem.setTerminalVelocity(terminalVelocity);
                    }
                    if (ImGui::SliderFloat("Jump Force", &jumpForce, 100.0f, 1000.0f, "%.1f")) {
                        physicsSystem.setJumpForce(jumpForce);
                    }
                    
                    ImGui::EndTabItem();
                }
                
                // Debug tab
                if (ImGui::BeginTabItem("Debug")) {
                    ImGui::Checkbox("Show ImGui Demo", &showImGuiDemo);
                    
                    ImGui::Text("FPS: %.1f", currentFPS);
                    ImGui::Text("Player Position: %.1f, %.1f", player.getPosition().x, player.getPosition().y);
                    
                    // Grid coordinate information
                    if (showDebugGrid) {
                        ImGui::Separator();
                        ImGui::Text("Grid Information");
                        
                        sf::Vector2f playerPos = player.getPosition();
                        ImGui::Text("Player Grid Coords: %.1f, %.1f", 
                                   playerPos.x / gridSize, playerPos.y / gridSize);
                                   
                        // View center coordinates
                        sf::Vector2f viewCenter = gameView.getCenter();
                        ImGui::Text("View Center: %.1f, %.1f", viewCenter.x, viewCenter.y);
                        ImGui::Text("View Grid Coords: %.1f, %.1f", 
                                   viewCenter.x / gridSize, viewCenter.y / gridSize);
                                   
                        // Origin information
                        ImGui::Text("Grid Origin (0,0) at world position (0,0)");
                        ImGui::Text("Grid size: %.0f units", gridSize);
                    }
                    
                    ImGui::EndTabItem();
                }
                
                // Assets tab
                if (ImGui::BeginTabItem("Assets")) {
                    ImGui::Text("Game Assets");
                    ImGui::Separator();
                    
                    // Asset status
                    ImGui::Text("Background: %s", useBackgroundPlaceholder ? "Using placeholder" : "Loaded");
                    ImGui::Text("Player: %s", usePlayerPlaceholder ? "Using placeholder" : "Loaded");
                    ImGui::Text("Enemy: %s", useEnemyPlaceholder ? "Using placeholder" : "Loaded");
                    
                    ImGui::Separator();
                    
                    // Platform tiles section
                    ImGui::Text("Platform Tiles");
                    ImGui::Text("Status: %s", renderingSystem.isLoaded() ? "Loaded" : "Not loaded");
                    if (renderingSystem.isLoaded()) {
                        ImGui::Text("Tile count: %d", renderingSystem.getTileCount());
                        
                        // Tile renderer settings
                        int tileSize = renderingSystem.getTileSize();
                        if (ImGui::SliderInt("Tile Size", &tileSize, 16, 128)) {
                            renderingSystem.setTileSize(tileSize);
                        }
                        
                        bool randomize = renderingSystem.isRandomizationEnabled();
                        if (ImGui::Checkbox("Randomize Tiles", &randomize)) {
                            renderingSystem.setRandomizationEnabled(randomize);
                        }
                        
                        if (ImGui::Button("Reload Tiles")) {
                            std::vector<std::string> tilePaths = {
                                "assets/images/platformer/tiles",
                                "./assets/images/platformer/tiles",
                                "../assets/images/platformer/tiles"
                            };
                            
                            for (const auto& path : tilePaths) {
                                if (renderingSystem.loadTiles(path)) {
                                    logInfo("Reloaded platform tiles from: " + path);
                                    break;
                                }
                            }
                        }
                    }
                    
                    ImGui::Separator();
                    
                    // Asset manager button
                    if (ImGui::Button("Open Asset Manager")) {
                        showAssetManager = true;
                        // Scan assets if we haven't done so yet
                        if (imageAssets.empty()) {
                            scanAssetDirectory("assets/images");
                        }
                    }
                    
                    if (ImGui::Button("Reload Assets")) {
                        loadAssets();
                    }
                    
                    ImGui::EndTabItem();
                }
                
                ImGui::EndTabBar();
            }
            
            ImGui::Separator();
            ImGui::Text("Press ESC or F1 to toggle interface");
        }
        ImGui::End();
        
        // Show asset manager window if enabled
        if (showAssetManager) {
            showAssetManagerWindow();
        }
        
        // Show ImGui demo window if enabled
        if (showImGuiDemo) {
            ImGui::ShowDemoWindow(&showImGuiDemo);
        }
    } catch (const std::exception& e) {
        logError("Exception in updateImGui: " + std::string(e.what()));
    } catch (...) {
        logError("Unknown exception in updateImGui");
    }
}

// Function to draw debug visualization
void Game::drawDebugBoxes() {
    if (showBoundingBoxes) {
        // Draw platform/ground collision boxes
        for (size_t platformIdx = 0; platformIdx < physicsSystem.getPlatformPhysicsCount(); platformIdx++) {
            sf::RectangleShape platformCollisionBox;
            sf::FloatRect platformPhysicsBox = physicsSystem.getPlatformPhysicsComponent(platformIdx).collisionBox;
            
            platformCollisionBox.setSize(sf::Vector2f(platformPhysicsBox.size.x, platformPhysicsBox.size.y));
            platformCollisionBox.setPosition(sf::Vector2f(platformPhysicsBox.position.x, platformPhysicsBox.position.y));
            platformCollisionBox.setFillColor(sf::Color(0, 0, 255, 30)); // Semi-transparent blue
            platformCollisionBox.setOutlineColor(sf::Color(0, 0, 255)); // Blue outline
            platformCollisionBox.setOutlineThickness(1.0f);
            window.draw(platformCollisionBox);
        }

        // Draw player collision box
        sf::FloatRect playerBounds = player.getGlobalBounds();
        float playerWidth = playerBounds.size.x * physicsSystem.getPlayerCollisionWidth();
        float playerHeight = playerBounds.size.y * physicsSystem.getPlayerCollisionHeight();
        float playerOffsetX = playerBounds.size.x * physicsSystem.getPlayerOffsetX();
        float playerOffsetY = playerBounds.size.y * physicsSystem.getPlayerOffsetY();

        sf::RectangleShape playerCollisionBox;
        playerCollisionBox.setSize(sf::Vector2f(playerWidth, playerHeight));
        playerCollisionBox.setPosition(sf::Vector2f(
            playerBounds.position.x + playerOffsetX,
            playerBounds.position.y + playerOffsetY
        ));
        playerCollisionBox.setFillColor(sf::Color(0, 255, 0, 30)); // Semi-transparent green
        playerCollisionBox.setOutlineColor(sf::Color(0, 255, 0)); // Green outline
        playerCollisionBox.setOutlineThickness(1.0f);
        window.draw(playerCollisionBox);

        // Draw NPC collision boxes
        if (npcManager) {
            const auto& npcs = npcManager->getAllNPCs();
            for (const auto& npc : npcs) {
                if (!npc.isActive || !npc.sprite) continue;

                // Get the sprite's bounds
                sf::FloatRect spriteBounds = npc.sprite->getGlobalBounds();
                
                // Calculate collision box dimensions (same as in checkPlayerNPCCollision)
                float width = spriteBounds.size.x * 0.8f;  // 80% of sprite width
                float height = spriteBounds.size.y * 0.8f;  // 80% of sprite height
                float offsetX = (spriteBounds.size.x - width) / 2.0f;
                float offsetY = (spriteBounds.size.y - height) / 2.0f;

                // Create and draw the NPC collision box
                sf::RectangleShape npcCollisionBox;
                npcCollisionBox.setSize(sf::Vector2f(width, height));
                npcCollisionBox.setPosition(sf::Vector2f(
                    spriteBounds.position.x + offsetX,
                    spriteBounds.position.y + offsetY
                ));
                npcCollisionBox.setFillColor(sf::Color(255, 165, 0, 30)); // Semi-transparent orange
                npcCollisionBox.setOutlineColor(sf::Color(255, 165, 0)); // Orange outline
                npcCollisionBox.setOutlineThickness(1.0f);
                window.draw(npcCollisionBox);
            }
        }
    }
}

// Run the game loop
void Game::run() {
    // Adjust player collision box - increase dimensions slightly for better visualization
    player.setCollisionBoxSize(sf::Vector2f(28.f, 28.f));
    
    // Run the game loop
    while (window.isOpen()) {
        handleEvents();
        update();
        draw();
    }
}

// Implementation of the asset manager window
void Game::showAssetManagerWindow() {
    // Auto-scan assets directory when window is opened if no assets are loaded
    static bool firstOpen = true;
    if (firstOpen) {
        scanAssetDirectory("assets");
        firstOpen = false;
    }

    if (ImGui::Begin("Asset Manager", &showAssetManager)) {
        // Scan button to refresh the asset list
        if (ImGui::Button("Scan Assets")) {
            scanAssetDirectory("assets");
        }
        
        ImGui::SameLine();
        
        // Help tooltip
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted("Click to scan the assets directory for all files.\nDirectory contents are shown with [DIR] prefix.\nImage files are shown with  prefix.");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
        
        ImGui::Separator();
        
        // Split window into two columns - left for list, right for preview
        ImGui::Columns(2, "assetColumns");
        
        // First column - Asset list
        ImGui::Text("Assets (%zu found)", imageAssets.size());
        ImGui::BeginChild("AssetList", ImVec2(0, 0), true);
        
        for (auto& asset : imageAssets) {
            // Display file name and status
            std::string label = asset.name;
            bool isImage = asset.name.find(" prefix.") != std::string::npos;
            bool isDir = asset.name.find("[DIR]") != std::string::npos;
            
            if (isImage && asset.isLoaded) {
                label += " [Loaded]";
            }
            
            // Set colors based on type
            if (isDir) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.8f, 1.0f, 1.0f)); // Blue for directories
            } else if (isImage) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.0f, 0.5f, 1.0f)); // Green for images
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // White for other files
            }
            
            // Selectable item with highlight
            if (ImGui::Selectable(label.c_str(), selectedAsset == &asset)) {
                selectedAsset = &asset;
                
                // Load texture for preview if available (only for images)
                previewAvailable = false;
                if (selectedAsset && isImage && selectedAsset->isLoaded) {
                    try {
                        // Try to load the texture for preview
                        if (previewTexture.loadFromFile(selectedAsset->path)) {
                            previewAvailable = true;
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "Failed to load preview texture: " << e.what() << std::endl;
                    }
                }
            }
            
            ImGui::PopStyleColor();
        }
        
        ImGui::EndChild();
        
        // Second column - Asset details and preview
        ImGui::NextColumn();
        
        if (selectedAsset) {
            // Show asset details
            ImGui::Text("File: %s", selectedAsset->name.c_str());
            ImGui::Text("Path: %s", selectedAsset->path.c_str());
            
            bool isImage = selectedAsset->name.find(" prefix.") != std::string::npos;
            bool isDir = selectedAsset->name.find("[DIR]") != std::string::npos;
            
            if (isDir) {
                ImGui::Text("Type: Directory");
                
                // Add a button to navigate into this directory
                if (ImGui::Button("Open Directory")) {
                    scanAssetDirectory(selectedAsset->path);
                }
            } else {
                ImGui::Text("Type: %s", isImage ? "Image" : "File");
                
                if (isImage) {
                    ImGui::Text("Dimensions: %ux%u", selectedAsset->dimensions.x, selectedAsset->dimensions.y);
                }
                
                // Format file size nicely (KB/MB)
                float fileSizeKB = static_cast<float>(selectedAsset->fileSize) / 1024.0f;
                if (fileSizeKB < 1024.0f) {
                    ImGui::Text("File size: %.2f KB", fileSizeKB);
                } else {
                    ImGui::Text("File size: %.2f MB", fileSizeKB / 1024.0f);
                }
                
                if (isImage) {
                    ImGui::Text("Status: %s", selectedAsset->isLoaded ? "Loaded" : "Not loaded");
                    
                    if (selectedAsset->isLoaded) {
                        ImGui::Text("Load time: %.2f ms", static_cast<float>(selectedAsset->loadTime.asMilliseconds()));
                    }
                }
            }
            
            ImGui::Separator();
            
            // Show preview if available
            if (isImage && previewAvailable) {
                // Calculate preview size maintaining aspect ratio
                float aspectRatio = static_cast<float>(previewTexture.getSize().x) / 
                                   static_cast<float>(previewTexture.getSize().y);
                
                float maxWidth = ImGui::GetContentRegionAvail().x;
                float maxHeight = 200.0f; // Maximum preview height
                
                float width = maxWidth;
                float height = width / aspectRatio;
                
                if (height > maxHeight) {
                    height = maxHeight;
                    width = height * aspectRatio;
                }
                
                // Display the texture
                ImGui::Text("Preview:");
                ImGui::Image(previewTexture.getNativeHandle(), ImVec2(width, height));
                
                // Add load button for images
                if (ImGui::Button("Load into Game")) {
                    try {
                        // Example of loading into the asset manager
                        std::string assetName = selectedAsset->name;
                        // Remove  prefix and any file extension
                        assetName = assetName.substr(assetName.find("]") + 2);
                        assetName = assetName.substr(0, assetName.find_last_of('.'));
                        assets.loadTexture(assetName, selectedAsset->path);
                        ImGui::OpenPopup("AssetLoaded");
                    } catch (const std::exception& e) {
                        std::cerr << "Failed to load asset: " << e.what() << std::endl;
                        ImGui::OpenPopup("AssetLoadError");
                    }
                }
                
                // Asset loaded popup
                if (ImGui::BeginPopupModal("AssetLoaded", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text("Asset loaded successfully!");
                    if (ImGui::Button("OK", ImVec2(120, 0))) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
                
                // Asset load error popup
                if (ImGui::BeginPopupModal("AssetLoadError", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text("Failed to load asset into game!");
                    if (ImGui::Button("OK", ImVec2(120, 0))) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
            } else if (isImage) {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Preview not available");
            }
        } else {
            ImGui::TextDisabled("Select an asset to view details");
        }
        
        ImGui::Columns(1);
        
        // Add a back button to navigate up a directory
        if (ImGui::Button("Back to Parent Directory")) {
            // Get current directory path
            std::string currentPath = selectedAsset ? selectedAsset->path : "assets";
            
            // Navigate to parent directory
            fs::path path(currentPath);
            fs::path parent = path.parent_path();
            
            // Don't go above the assets directory
            if (parent.filename().string() == "assets" || 
                parent.filename().string() == "images" || 
                parent.string().find("assets") != std::string::npos) {
                scanAssetDirectory(parent.string());
            } else {
                scanAssetDirectory("assets");
            }
        }
    }
    ImGui::End();
    
    // If window is closed, reset the firstOpen flag for next time
    if (!showAssetManager) {
        firstOpen = true;
    }
}

// Scan asset directory recursively to find image files
void Game::scanAssetDirectory(const std::string& directory) {
    try {
        logDebug("Scanning directory: " + directory);
        imageAssets.clear();
        
        // Check if the directory exists
        if (!fs::exists(directory)) {
            std::cerr << "Directory does not exist: " << directory << std::endl;
            
            // Try with different relative paths
            std::vector<std::string> pathsToTry = {
                "./assets",
                "../assets",
                "assets",
                "/Users/startup/my-game/assets"
            };
            
            bool foundPath = false;
            for (const auto& path : pathsToTry) {
                logDebug("Trying alternative path: " + path);
                if (fs::exists(path)) {
                    logDebug("Found valid path: " + path);
                    scanAssetDirectory(path);
                    foundPath = true;
                    break;
                }
            }
            
            if (!foundPath) {
                std::cerr << "Could not find assets directory in any of the tried paths" << std::endl;
            }
            return;
        }
        
        // List all entries in the directory first (non-recursive)
        logDebug("Listing top-level entries in " + directory + ":");
        
        // Track directories for subdirectory display
        std::vector<std::string> subdirectories;
        
        // First scan the top directory (non-recursive)
        for (const auto& entry : fs::directory_iterator(directory)) {
            std::string path = entry.path().string();
            std::string name = entry.path().filename().string();
            
            logDebug("Found: " + path);
            
            if (entry.is_directory()) {
                // Add to subdirectories list
                subdirectories.push_back(path);
                
                // Create a directory entry in our assets list
                ImageAssetInfo info;
                info.path = path;
                info.name = "[DIR] " + name;
                info.fileSize = 0;
                info.isLoaded = false;
                info.dimensions = sf::Vector2u(0, 0);
                imageAssets.push_back(info);
            } else if (entry.is_regular_file()) {
                std::string extension = entry.path().extension().string();
                
                // Convert extension to lowercase
                std::transform(extension.begin(), extension.end(), extension.begin(), 
                            [](unsigned char c) { return std::tolower(c); });
                
                // Add file to our assets list
                ImageAssetInfo info;
                info.path = path;
                info.name = name;
                info.fileSize = entry.file_size();
                info.isLoaded = false;
                info.dimensions = sf::Vector2u(0, 0);
                
                // Try to load the image if it's an image file
                if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || 
                    extension == ".bmp" || extension == ".tga") {
                    sf::Clock loadTimer;
                    sf::Texture texture;
                    if (texture.loadFromFile(info.path)) {
                        info.dimensions = texture.getSize();
                        info.isLoaded = true;
                        info.loadTime = loadTimer.getElapsedTime();
                        info.name = " prefix. " + name;
                    }
                }
                
                imageAssets.push_back(info);
            }
        }
        
        // Now recursively scan image subdirectories
        for (const auto& subdir : subdirectories) {
            if (subdir.find("images") != std::string::npos) {
                logDebug("Scanning image subdirectory: " + subdir);
                
                for (const auto& entry : fs::recursive_directory_iterator(subdir)) {
                    if (entry.is_regular_file()) {
                        std::string extension = entry.path().extension().string();
                        
                        // Convert extension to lowercase
                        std::transform(extension.begin(), extension.end(), extension.begin(), 
                                    [](unsigned char c) { return std::tolower(c); });
                        
                        // Check if this is an image file
                        if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || 
                            extension == ".bmp" || extension == ".tga") {
                            
                            ImageAssetInfo info;
                            info.path = entry.path().string();
                            info.name = " prefix. " + entry.path().filename().string();
                            info.fileSize = entry.file_size();
                            info.isLoaded = false;
                            info.loadTime = sf::Time::Zero;
                            
                            // Try to load the image to get dimensions
                            sf::Clock loadTimer;
                            sf::Texture texture;
                            if (texture.loadFromFile(info.path)) {
                                info.dimensions = texture.getSize();
                                info.isLoaded = true;
                                info.loadTime = loadTimer.getElapsedTime();
                            } else {
                                info.dimensions = sf::Vector2u(0, 0);
                            }
                            
                            imageAssets.push_back(info);
                        }
                    }
                }
            }
        }
        
        // Sort assets by name
        std::sort(imageAssets.begin(), imageAssets.end(), 
                 [](const ImageAssetInfo& a, const ImageAssetInfo& b) {
                     // Directories come first, then files
                     bool aIsDir = a.name.find("[DIR]") != std::string::npos;
                     bool bIsDir = b.name.find("[DIR]") != std::string::npos;
                     
                     if (aIsDir && !bIsDir) return true;
                     if (!aIsDir && bIsDir) return false;
                     
                     // Then sort by name
                     return a.name < b.name;
                 });
        
        logDebug("Asset scan complete. Found " + std::to_string(imageAssets.size()) + " items.");
                 
    } catch (const std::exception& e) {
        std::cerr << "Error scanning assets directory: " << e.what() << std::endl;
    }
} 
// Method to synchronize platforms with their physics components
void Game::syncPlatformsWithPhysics() {
    // Make sure we have physics components for each platform
    if (platforms.size() != physicsSystem.getPlatformPhysicsCount()) {
        logWarning("Platform count mismatch. Platforms: " + std::to_string(platforms.size()) + 
                  ", Physics components: " + std::to_string(physicsSystem.getPlatformPhysicsCount()));
        return;
    }
    
    // For each platform, update its position and size to match the collision box
    for (size_t i = 0; i < platforms.size(); ++i) {
        const auto& physicsBox = physicsSystem.getPlatformPhysicsComponent(i).collisionBox;
        platforms[i].setPosition(physicsBox.position);
        platforms[i].setSize(physicsBox.size);
    }
    
    logDebug("Synchronized " + std::to_string(platforms.size()) + " platforms with physics components");
}

void Game::jumpToLevel(int level) {
    // Ensure level is valid (only 1 or 2)
    level = std::min(std::max(level, 1), 2);
    
    // Set the current level
    currentLevel = level;
    
    // Reset player position
    player.reset(50.f, WINDOW_HEIGHT - GROUND_HEIGHT - 40.f);
    player.setCollisionBoxSize(sf::Vector2f(28.f, 28.f));
    
    // Reset view
    gameView.setCenter(sf::Vector2f(WINDOW_WIDTH / 2.f, WINDOW_HEIGHT / 2.f));
    
    // Reset game state
    currentState = GameState::Playing;
    playerHit = false;
    playerHitCooldown = 0.f;
    
    // Update level text
    levelText.setString("Level " + std::to_string(currentLevel));
    sf::FloatRect levelBounds = levelText.getGlobalBounds();
    levelText.setPosition(sf::Vector2f(
        WINDOW_WIDTH / 2.f - levelBounds.size.x / 2.f,
        20.f
    ));

    // Clear NPCs before reinitializing
    if (npcManager) {
        npcManager->clearNPCs();
    }
    
    // Change background and theme based on level
    try {
        // Different backgrounds for each level
        std::string levelBackgroundPath;
        std::vector<std::string> alternativePaths;
        
        if (currentLevel == 1) {
            // Level 1 - Snow Mountain
            levelBackgroundPath = "assets/images/backgrounds/snow/snow_background.png";
            platformColor = sf::Color(200, 220, 255); // Light blue for snow mountain
            alternativePaths = {
                "assets/images/backgrounds/snow_background.png",
                "assets/images/backgrounds/background.png",
                "../assets/images/backgrounds/background.png"
            };
        } else {
            // Level 2 - Snow Forest
            levelBackgroundPath = "assets/images/backgrounds/snow_forest/snow_forest_background.png";
            platformColor = sf::Color(180, 200, 240); // Slightly different blue for snow forest
            alternativePaths = {
                "assets/images/backgrounds/snow_forest_background.png",
                "assets/images/backgrounds/snow/snow_background.png",
                "assets/images/backgrounds/background.png",
                "../assets/images/backgrounds/background.png"
            };
        }
        
        // Try to load the level-specific background
        bool loaded = false;
        try {
            assets.loadTexture("background", levelBackgroundPath);
            logInfo("Successfully loaded background: " + levelBackgroundPath);
            loaded = true;
        } 
        catch (const std::exception& e) {
            logError("Failed to load primary background: " + std::string(e.what()));
            
            // Try alternative paths
            for (const auto& path : alternativePaths) {
                try {
                    assets.loadTexture("background", path);
                    logInfo("Successfully loaded alternative background: " + path);
                    loaded = true;
                    break;
                } catch (const std::exception& innerE) {
                    logWarning("Failed to load alternative background from " + path + ": " + std::string(innerE.what()));
                }
            }
        }
        
        // Reload the layered background system for the new level
        if (loaded) {
            loadBackgroundLayers();
            logInfo("Reloaded layered backgrounds for level " + std::to_string(currentLevel));
        }
        // If we couldn't load a background texture, use the placeholder
        else {
            useBackgroundPlaceholder = true;
            backgroundPlaceholder.setFillColor(sf::Color(200, 220, 255)); // Light blue for snow theme
        }
    } catch (const std::exception& e) {
        logError("Failed to load background for level " + std::to_string(currentLevel) + ": " + std::string(e.what()));
        useBackgroundPlaceholder = true;
        backgroundPlaceholder.setFillColor(sf::Color(200, 220, 255)); // Light blue for snow theme
    }
    
    // Reinitialize game elements with new variations based on level
    initializePlatforms();
    initializeEnemies();
    initializeUI();
    initializeMiniMap();
    
    // Apply level-specific physics and difficulty
    if (currentLevel == 2) {
        // Add extra enemies for level 2
        for (int i = 0; i < 2; i++) {
            float x = 500.f + (i * 400.f);  // Space them out
            float y = WINDOW_HEIGHT - 70.f; // On the ground
            float patrolDistance = 180.f; // Fixed patrol distance for level 2
            enemies.push_back(Enemy(x, y, patrolDistance));
        }
    }
    
    // Puzzle-focused physics - minimal jumping, ground-based movement
    physicsSystem.setGravity(15.0f);  // Higher gravity to keep player grounded
    physicsSystem.setJumpForce(200.f); // Much lower jump force for puzzle gameplay
    
    // Initialize physics system
    physicsSystem.initialize();
    physicsSystem.initializePlayer(player);
    physicsSystem.initializePlatforms(platforms);
    physicsSystem.initializeEnemies(enemies);
    
    logInfo("Jumped to level " + std::to_string(currentLevel));
}

// Initialize background layers with default configuration
void Game::initializeBackgroundLayers() {
    backgroundLayers.clear();
    
    // Define layers from back to front with parallax speeds
    // The drawing order is: background1 (first/back) -> background2 -> background3 -> background4 (last/front)
    // This creates the correct visual layering with background4 on top and background1 at the back
    // All layers now move at the same speed (1.0f) to avoid dizzying parallax effects
    
    // Background1 layer - moves with camera (furthest back)
    backgroundLayers.emplace_back("background1", 0.0f, true, true);
    
    // Background2 layer - moves with camera (behind background3 and background4)
    backgroundLayers.emplace_back("background2", 0.0f, true, false);
    
    // Background3 layer - moves with camera (behind background4, in front of background2)
    backgroundLayers.emplace_back("background3", 0.0f, true, false);
    
    // Background4 layer - moves with camera (closest to viewer, on top of all other layers)
    backgroundLayers.emplace_back("background4", 0.0f, true, false);
    
    logInfo("Initialized " + std::to_string(backgroundLayers.size()) + " background layers");
}

// Load background layer textures
void Game::loadBackgroundLayers() {
    useBackgroundPlaceholder = true; // Start with placeholder
    int loadedLayers = 0;
    
    for (auto& layer : backgroundLayers) {
        // Define potential paths for each layer
        std::vector<std::string> layerPaths;
        
        if (layer.name == "background1") {
            layerPaths = {
                "assets/images/backgrounds/" + std::to_string(currentLevel) + "/background1.png",
                "assets/images/backgrounds/background1.png",
                (currentLevel == 1) ? "assets/images/backgrounds/snow/background1.png" : "assets/images/backgrounds/snow_forest/background1.png",
                "assets/images/backgrounds/snow/background1.png"
            };
        } else if (layer.name == "background2") {
            layerPaths = {
                "assets/images/backgrounds/" + std::to_string(currentLevel) + "/background2.png",
                "assets/images/backgrounds/background2.png",
                (currentLevel == 1) ? "assets/images/backgrounds/snow/background2.png" : "assets/images/backgrounds/snow_forest/background2.png",
                "assets/images/backgrounds/snow/background2.png"
            };
        } else if (layer.name == "background3") {
            layerPaths = {
                "assets/images/backgrounds/" + std::to_string(currentLevel) + "/background3.png",
                "assets/images/backgrounds/background3.png",
                (currentLevel == 1) ? "assets/images/backgrounds/snow/background3.png" : "assets/images/backgrounds/snow_forest/background3.png",
                "assets/images/backgrounds/snow/background3.png"
            };
        } else if (layer.name == "background4") {
            layerPaths = {
                "assets/images/backgrounds/" + std::to_string(currentLevel) + "/background4.png",
                "assets/images/backgrounds/background4.png",
                (currentLevel == 1) ? "assets/images/backgrounds/snow/background4.png" : "assets/images/backgrounds/snow_forest/background4.png",
                "assets/images/backgrounds/snow/background4.png",
                // Fallback to the original background texture
                "assets/images/backgrounds/background.png",
                "../assets/images/backgrounds/background.png"
            };
        }
        
        // Try to load the layer texture
        bool layerLoaded = false;
        for (const auto& path : layerPaths) {
            try {
                // Use level-specific texture keys to avoid caching issues
                std::string textureKey = "bg_" + layer.name + "_level" + std::to_string(currentLevel);
                assets.loadTexture(textureKey, path);
                layer.sprite = std::make_unique<sf::Sprite>(assets.getTexture(textureKey));
                layer.textureSize = assets.getTexture(textureKey).getSize();
                layer.isLoaded = true;
                layerLoaded = true;
                loadedLayers++;
                logInfo("Successfully loaded " + layer.name + " layer from: " + path);
                logInfo("  Texture size: " + std::to_string(layer.textureSize.x) + "x" + std::to_string(layer.textureSize.y));
                break;
            } catch (const std::exception& e) {
                logWarning("Failed to load " + layer.name + " from " + path + ": " + std::string(e.what()));
                // Continue to next path
            }
        }
        
        if (!layerLoaded) {
            logWarning("Could not load " + layer.name + " layer, will skip in rendering");
        }
    }
    
    // If at least one layer loaded, don't use placeholder
    if (loadedLayers > 0) {
        useBackgroundPlaceholder = false;
        logInfo("Successfully loaded " + std::to_string(loadedLayers) + " background layers");
    } else {
        logWarning("No background layers loaded, using placeholder");
    }
    
    // Pass the background layers to the rendering system
    renderingSystem.setBackgroundLayersRef(backgroundLayers);
    renderingSystem.setUseBackgroundPlaceholder(useBackgroundPlaceholder);
}



// Logging methods implementation
void Game::logDebug(const std::string& message) {
    if (loggingEnabled && gameLogFile.is_open()) {
        gameLogFile << "[" << getCurrentTimestamp() << "] [DEBUG] " << message << std::endl;
        gameLogFile.flush();
    }
}

void Game::logInfo(const std::string& message) {
    if (loggingEnabled && gameLogFile.is_open()) {
        gameLogFile << "[" << getCurrentTimestamp() << "] [INFO] " << message << std::endl;
        gameLogFile.flush();
    }
}

void Game::logWarning(const std::string& message) {
    if (loggingEnabled && gameLogFile.is_open()) {
        gameLogFile << "[" << getCurrentTimestamp() << "] [WARNING] " << message << std::endl;
        gameLogFile.flush();
    }
}

void Game::logError(const std::string& message) {
    if (loggingEnabled && gameLogFile.is_open()) {
        gameLogFile << "[" << getCurrentTimestamp() << "] [ERROR] " << message << std::endl;
        gameLogFile.flush();
    }
}

void Game::clearGameLogFile() {
    if (gameLogFile.is_open()) {
        gameLogFile.close();
    }
    gameLogFile.open(gameLogFileName, std::ios::out | std::ios::trunc);
    if (gameLogFile.is_open()) {
        logInfo("Game log file cleared");
    }
}

std::string Game::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

void Game::initializeNPCs() {
    // Load NPC textures for different animations
    assets.loadTexture("npc_idle", "assets/images/npc/separated/idle/idle_frame_01.png");
    assets.loadTexture("npc_walking", "assets/images/npc/separated/walking/walking_frame_01.png");
   // assets.loadTexture("merchant_idle", "assets/images/npc/merchant/idle/merchant_idle_01.png");
    
    // Clear existing NPCs
    if (npcManager) {
        npcManager->clearNPCs();
    }

    // Create level-specific NPCs
    if (currentLevel == 1) {
        // Old man NPC only appears in level 1
        float npcX = LEVEL_WIDTH - 300.f;  // 300 pixels from the right edge
        float npcY = WINDOW_HEIGHT - GROUND_HEIGHT - 16;  // Just above the ground
        npcManager->createNPC("old_man", "npc_idle", npcX, npcY);
    } 
    else if (currentLevel == 2) {
        // Merchant NPC only appears in level 2
/*         float merchantX = 400.f;  // Position merchant early in level 2
        float merchantY = WINDOW_HEIGHT - GROUND_HEIGHT - 16;
        npcManager->createNPC("merchant", "merchant_idle", merchantX, merchantY); */
    }

    // Configure NPC physics properties
    physicsSystem.setNPCCollisionSize(0.8f, 0.9f);  // Slightly smaller than sprite for better collision
    physicsSystem.setNPCCollisionOffset(0.1f, 0.05f);  // Center the collision box
    physicsSystem.setNPCBounceFactor(0.0f);  // No bouncing for NPCs
    
    // Initialize NPC physics
    physicsSystem.initializeNPCs(npcManager->getAllNPCs());
}

void Game::checkPlayerNPCCollision() {
    if (!npcManager) return;
    
    const auto& npcs = npcManager->getAllNPCs();
    sf::FloatRect playerBounds = player.getGlobalBounds();
    
    for (const auto& npc : npcs) {
        if (!npc.isActive || !npc.sprite) continue;
        
        // Get the sprite's bounds first
        sf::FloatRect spriteBounds = npc.sprite->getGlobalBounds();
        
        // Calculate collision box dimensions (slightly smaller than sprite)
        float width = spriteBounds.size.x * 0.8f;  // 80% of sprite width
        float height = spriteBounds.size.y * 0.8f;  // 80% of sprite height
        
        // Center the collision box on the sprite
        float offsetX = (spriteBounds.size.x - width) / 2.0f;
        float offsetY = (spriteBounds.size.y - height) / 2.0f;
        
        // Create collision bounds centered on the sprite
        sf::FloatRect npcBounds(
            sf::Vector2f(spriteBounds.position.x + offsetX, spriteBounds.position.y + offsetY),
            sf::Vector2f(width, height)
        );
        
        // Check for collision
        bool isColliding = rectsIntersect(playerBounds, npcBounds);
        
        // Calculate center points for collision response
        float playerCenterX = playerBounds.position.x + playerBounds.size.x / 2.0f;
        float npcCenterX = npc.x;
        
        // Handle NPC interaction using player bounds
        npcManager->handleInteraction(npc.id, playerBounds);
        
        if (isColliding) {
            // Push player away from NPC to prevent overlap
            if (playerCenterX < npcCenterX) {
                float pushDistance = npcBounds.position.x - (playerBounds.size.x + 5.f);
                player.setPosition(sf::Vector2f(pushDistance, player.getPosition().y));
            } else {
                float pushDistance = npcBounds.position.x + npcBounds.size.x + 5.f;
                player.setPosition(sf::Vector2f(pushDistance, player.getPosition().y));
            }
        }
    }
}


