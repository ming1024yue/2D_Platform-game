#include "Game.hpp"
#include <iostream>
#include <cstdint> // For uint8_t
#include <sstream>
#include <iomanip>
#include <filesystem>

namespace fs = std::filesystem;

// Helper function for rectangle intersection (for SFML 3.x compatibility)
static bool rectsIntersect(const sf::FloatRect& a, const sf::FloatRect& b) {
    return a.position.x < b.position.x + b.size.x &&
           a.position.x + a.size.x > b.position.x &&
           a.position.y < b.position.y + b.size.y &&
           a.position.y + a.size.y > b.position.y;
}

Game::Game() : window(sf::VideoMode(sf::Vector2u(WINDOW_WIDTH, WINDOW_HEIGHT)), "2D Platform Puzzle Game"),
               player(50.f, WINDOW_HEIGHT - 100.f), // Increase initial height above ground
               playerHit(false),
               playerHitCooldown(0.f),
               currentState(GameState::Playing),
               healthText(defaultFont, sf::String("HP: 3"), 24),
               gameOverText(defaultFont, sf::String("GAME OVER"), 48),
               restartText(defaultFont, sf::String("Press ENTER to restart"), 24),
               fpsText(defaultFont, sf::String("FPS: 0"), 16),
               playerLightIndex(0),
               showMiniMap(true),
               showLighting(true),
               currentLevel(1),
               transitionTimer(0.f),
               levelText(defaultFont, sf::String("Level 1"), 36),
               playerPosition(50.f, WINDOW_HEIGHT / 2.f),
               playerSpeed(200.f),
               isRunning(true),
               previousState(GameState::Playing),
               showBoundingBoxes(true),
               gameSpeed(1.0f),
               platformColor(34, 139, 34),
               playerBorderColor(0, 255, 0),
               enemyBorderColor(255, 0, 0),
               spriteScale(4.0f),
               boundaryBoxHeight(0.67f),
               fpsUpdateTime(0.0f),
               frameCount(0),
               currentFPS(0.0f),
               showImGuiDemo(false),
               useImGuiInterface(true),
               showAssetManager(false),
               previewAvailable(false) {
    
    // Initialize sprite pointers with shared empty texture (created in loadAssets)
    // This is required because sf::Sprite has no default constructor in SFML 3.x
               
    window.setFramerateLimit(FPS);
    
    // Initialize lighting system
    sf::Color ambientColor(30, 30, 40, 200); // Dark blue-ish ambient light
    lightingSystem.initialize(WINDOW_WIDTH, WINDOW_HEIGHT, ambientColor);
    lightingSystem.setEnabled(false); // Lighting system off by default
    
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
    
    // Load game assets
    loadAssets();
    
    initializePlatforms();
    initializeLadders();
    initializeEnemies();
    initializeUI();
    initializeMiniMap();
    initializeLights();
    
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
    
    if (currentState == GameState::Playing) {
        // Update player
        player.update(platforms, ladders);
        
        // Update enemies
        for (auto& enemy : enemies) {
            enemy.update(platforms);
        }
        
        // Update physics system
        physicsSystem.update(deltaTime, player, enemies);
        
        // Check for player-enemy collisions
        checkPlayerEnemyCollision();
        
        // Check if game is over
        checkGameOver();
        
        // Check if level is complete
        checkLevelCompletion();
        
        // Update lighting effects
        updateLights();
        
        // Update UI
        updateUI();
        
        // Update mini-map
        updateMiniMap();
        
        // Center view on player with some limits
        float viewX = std::max(WINDOW_WIDTH / 2.f, 
                          std::min(player.getPosition().x, LEVEL_WIDTH - WINDOW_WIDTH / 2.f));
        gameView.setCenter(sf::Vector2f(viewX, gameView.getCenter().y));
        window.setView(gameView);
    } else if (currentState == GameState::LevelTransition) {
        // Handle level transition timer
        transitionTimer -= deltaTime;
        if (transitionTimer <= 0) {
            nextLevel();
        }
    }
}

void Game::loadAssets() {
    try {
        // Set placeholder flags to true initially
        useBackgroundPlaceholder = true;
        usePlayerPlaceholder = true;
        useEnemyPlaceholder = true;
        
        // Initialize placeholder shapes for drawing
        backgroundPlaceholder.setSize(sf::Vector2f(LEVEL_WIDTH, WINDOW_HEIGHT));
        backgroundPlaceholder.setFillColor(sf::Color(100, 180, 100)); // Green
        backgroundPlaceholder.setPosition(sf::Vector2f(0, 0));
        
        playerPlaceholder.setSize(sf::Vector2f(32, 32));
        playerPlaceholder.setFillColor(sf::Color::Blue);
        
        enemyPlaceholder.setSize(sf::Vector2f(32, 32));
        enemyPlaceholder.setFillColor(sf::Color::Red);
        
        // Load backgrounds
        try {
            // Try to load background_forest.png first (this matches our code)
            try {
                assets.loadTexture("background", "../assets/images/backgrounds/background.png");
            } catch (const std::exception& e) {
                // If that fails, try loading background.png instead
                std::cerr << "Trying alternate background file..." << std::endl;
                assets.loadTexture("background", "assets/images/backgrounds/background.png");
            }
            
            backgroundSprite = std::make_unique<sf::Sprite>(assets.getTexture("background"));
            
            // Don't scale the background sprite - we'll use tiling instead
            // Get the natural texture size to use for tiling calculations
            backgroundTextureSize = assets.getTexture("background").getSize();
            
            useBackgroundPlaceholder = false;
            std::cout << "Successfully loaded and created background sprite" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Failed to load background: " << e.what() << std::endl;
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
                    std::cout << "Successfully loaded player sprite from: " << path << std::endl;
                    break;
                } catch (const std::exception& e) {
                    std::cerr << "Failed to load player sprite from " << path << ": " << e.what() << std::endl;
                }
            }
            
            if (playerLoaded) {
                playerSprite = std::make_unique<sf::Sprite>(assets.getTexture("player"));
                usePlayerPlaceholder = false;
            } else {
                std::cerr << "Failed to load player sprite from any path" << std::endl;
                // We'll use the player placeholder instead
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to load player sprite: " << e.what() << std::endl;
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
                    std::cout << "Successfully loaded enemy sprite from: " << path << std::endl;
                    break;
                } catch (const std::exception& e) {
                    std::cerr << "Failed to load enemy sprite from " << path << ": " << e.what() << std::endl;
                }
            }
            
            if (enemyLoaded) {
                enemySprite = std::make_unique<sf::Sprite>(assets.getTexture("enemy"));
                useEnemyPlaceholder = false;
            } else {
                std::cerr << "Failed to load enemy sprite from any path" << std::endl;
                // We'll use the enemy placeholder instead
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to load enemy sprite: " << e.what() << std::endl;
            // We'll use the enemy placeholder instead
        }
        
        // Load fonts
        try {
            assets.loadFont("pixel_font", "assets/fonts/pixel.ttf");
        } catch (const std::exception& e) {
            std::cerr << "Failed to load font: " << e.what() << std::endl;
            // Font errors are handled in initializeUI with fallbacks
        }
        
        // Load sounds
        try {
            assets.loadSoundBuffer("jump", "assets/audio/sfx/jump.wav");
            assets.loadSoundBuffer("hit", "assets/audio/sfx/hit.wav");
        } catch (const std::exception& e) {
            std::cerr << "Failed to load sound: " << e.what() << std::endl;
            // Game can continue without sounds
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Asset loading error: " << e.what() << std::endl;
        std::cerr << "Game will continue without some assets." << std::endl;
    }
}

sf::RectangleShape Game::createHeartIcon(float x, float y, bool filled) {
    // Create a simple pixel art heart using a rectangle with custom texture
    sf::RectangleShape heart;
    heart.setSize(sf::Vector2f(24.f, 24.f)); // 24x24 pixel heart
    heart.setPosition(sf::Vector2f(x, y));
    
    // Set color based on filled status (red for filled, darker red for empty)
    if (filled) {
        heart.setFillColor(sf::Color(255, 40, 40)); // Bright red
    } else {
        heart.setFillColor(sf::Color(150, 20, 20)); // Dark red
    }
    
    // Add outline for better visibility
    heart.setOutlineColor(sf::Color::Black);
    heart.setOutlineThickness(1.0f);
    
    return heart;
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
        healthText.setFont(font);
        gameOverText.setFont(font);
        restartText.setFont(font);
        levelText.setFont(font); 
        fpsText.setFont(font); // Also update the FPS text font
    }
    
    // Make FPS text more visible - use larger size and bright color
    fpsText.setFillColor(sf::Color::White);  // White is more visible
    fpsText.setOutlineColor(sf::Color::Black);
    fpsText.setOutlineThickness(2.0f);
    fpsText.setCharacterSize(24); // Larger text
    
    // Configure health text properties for pixel art style
    healthText.setFillColor(sf::Color::White);
    healthText.setOutlineColor(sf::Color::Black);
    healthText.setOutlineThickness(1.0f);
    healthText.setLetterSpacing(1.5f); // Add some spacing for that pixel art feel
    
    // Position the text in the top-left with a small margin
    // Just use text for "HP:" label
    healthText.setString(sf::String("HP:"));
    healthText.setPosition(sf::Vector2f(16.f, 16.f));
    
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
    
    // Create heart icons for health
    const float heartStartX = 60.f; // Start after the "HP:" text
    const float heartY = 16.f;      // Same Y position as text
    const float heartSpacing = 30.f; // Space between hearts
    
    // Clear any existing hearts
    heartIcons.clear();
    
    // Create heart icons based on max health (assuming 3 max)
    for (int i = 0; i < 3; i++) {
        bool filled = i < player.getHealth();
        heartIcons.push_back(createHeartIcon(heartStartX + i * heartSpacing, heartY, filled));
    }
}

void Game::updateUI() {
    // Update heart icons to reflect current health
    for (int i = 0; i < heartIcons.size(); i++) {
        bool filled = i < player.getHealth();
        heartIcons[i].setFillColor(filled ? sf::Color(255, 40, 40) : sf::Color(150, 20, 20));
    }
}

void Game::initializePlatforms() {
    // Ground platform - extend it to cover the full level width and make it thicker
    sf::RectangleShape ground;
    ground.setSize(sf::Vector2f(LEVEL_WIDTH, 60.f)); // Thicker ground
    ground.setPosition(sf::Vector2f(0, WINDOW_HEIGHT - 60.f)); // Adjusted position
    ground.setFillColor(sf::Color(34, 139, 34)); // Forest Green color for ground
    platforms.push_back(ground);
    
    // Debug ground platform info
    std::cout << "Ground platform position: " << ground.getPosition().x << ", " << ground.getPosition().y 
              << " size: " << ground.getSize().x << ", " << ground.getSize().y << std::endl;

    // Platform 1
    sf::RectangleShape platform1;
    platform1.setSize(sf::Vector2f(200.f, 20.f));
    platform1.setPosition(sf::Vector2f(300.f, 400.f));
    platform1.setFillColor(sf::Color(34, 139, 34)); // Same Forest Green color
    platforms.push_back(platform1);

    // Platform 2
    sf::RectangleShape platform2;
    platform2.setSize(sf::Vector2f(200.f, 20.f));
    platform2.setPosition(sf::Vector2f(100.f, 300.f));
    platform2.setFillColor(sf::Color(34, 139, 34)); // Same Forest Green color
    platforms.push_back(platform2);

    // Platform 3
    sf::RectangleShape platform3;
    platform3.setSize(sf::Vector2f(200.f, 20.f));
    platform3.setPosition(sf::Vector2f(500.f, 200.f));
    platform3.setFillColor(sf::Color(34, 139, 34)); // Same Forest Green color
    platforms.push_back(platform3);
    
    // Additional platforms for scrolling section
    sf::RectangleShape platform4;
    platform4.setSize(sf::Vector2f(200.f, 20.f));
    platform4.setPosition(sf::Vector2f(900.f, 350.f));
    platform4.setFillColor(sf::Color(34, 139, 34));
    platforms.push_back(platform4);
    
    sf::RectangleShape platform5;
    platform5.setSize(sf::Vector2f(200.f, 20.f));
    platform5.setPosition(sf::Vector2f(1200.f, 250.f));
    platform5.setFillColor(sf::Color(34, 139, 34));
    platforms.push_back(platform5);
    
    sf::RectangleShape platform6;
    platform6.setSize(sf::Vector2f(200.f, 20.f));
    platform6.setPosition(sf::Vector2f(1500.f, 400.f));
    platform6.setFillColor(sf::Color(34, 139, 34));
    platforms.push_back(platform6);
    
    sf::RectangleShape platform7;
    platform7.setSize(sf::Vector2f(200.f, 20.f));
    platform7.setPosition(sf::Vector2f(1800.f, 300.f));
    platform7.setFillColor(sf::Color(34, 139, 34));
    platforms.push_back(platform7);
    
    sf::RectangleShape platform8;
    platform8.setSize(sf::Vector2f(200.f, 20.f));
    platform8.setPosition(sf::Vector2f(2100.f, 200.f));
    platform8.setFillColor(sf::Color(34, 139, 34));
    platforms.push_back(platform8);
    
    sf::RectangleShape platform9;
    platform9.setSize(sf::Vector2f(200.f, 20.f));
    platform9.setPosition(sf::Vector2f(2400.f, 350.f));
    platform9.setFillColor(sf::Color(34, 139, 34));
    platforms.push_back(platform9);
    
    sf::RectangleShape platform10;
    platform10.setSize(sf::Vector2f(300.f, 20.f));
    platform10.setPosition(sf::Vector2f(2700.f, 250.f));
    platform10.setFillColor(sf::Color(34, 139, 34));
    platforms.push_back(platform10);
}

void Game::initializeLadders() {
    // Ladder to reach platform 3 (the highest one initially)
    sf::RectangleShape ladder1;
    ladder1.setSize(sf::Vector2f(30.f, 380.f)); // Make it tall enough to reach the platform
    ladder1.setPosition(sf::Vector2f(580.f, WINDOW_HEIGHT - 40.f - 380.f)); // Start from ground up to platform
    ladder1.setFillColor(sf::Color(139, 69, 19)); // Brown color
    ladders.push_back(ladder1);
    
    // Ladder to reach platform 5
    sf::RectangleShape ladder2;
    ladder2.setSize(sf::Vector2f(30.f, 350.f));
    ladder2.setPosition(sf::Vector2f(1300.f, 250.f - 100.f)); // Center with platform and extend below it
    ladder2.setFillColor(sf::Color(139, 69, 19));
    ladders.push_back(ladder2);
    
    // Ladder to reach platform 8
    sf::RectangleShape ladder3;
    ladder3.setSize(sf::Vector2f(30.f, 400.f));
    ladder3.setPosition(sf::Vector2f(2200.f, 200.f - 100.f)); // Extend below platform
    ladder3.setFillColor(sf::Color(139, 69, 19));
    ladders.push_back(ladder3);
    
    // Ladder for platform 10
    sf::RectangleShape ladder4;
    ladder4.setSize(sf::Vector2f(30.f, 350.f));
    ladder4.setPosition(sf::Vector2f(2850.f, 250.f - 100.f)); // Extend below platform
    ladder4.setFillColor(sf::Color(139, 69, 19));
    ladders.push_back(ladder4);
}

void Game::initializeEnemies() {
    // Clear existing enemies first
    enemies.clear();
    
    // Add enemies on different platforms with safe starting positions
    // Keep enemy positions well inside platform boundaries and away from edges
    
    // Enemy on ground - adjust for new ground height, start a bit away from the edge
    enemies.push_back(Enemy(400.f, WINDOW_HEIGHT - 90.f, 180.f));
    
    // Debug enemy position
    std::cout << "Enemy 1 initialized at position: " << enemies.back().getPosition().x << ", " 
              << enemies.back().getPosition().y << std::endl;
    
    // Enemy on platform 1 - start more centered on the platform to avoid edge issues
    // Platform 1 is at (300, 400) with size 200x20
    enemies.push_back(Enemy(380.f, 370.f, 80.f)); // Position more to the center with reduced patrol width
    
    std::cout << "Enemy 2 initialized at position: " << enemies.back().getPosition().x << ", " 
              << enemies.back().getPosition().y << std::endl;
    
    // Enemy on platform 2 - add enemy on the previously unused platform
    // Platform 2 is at (100, 300) with size 200x20
    enemies.push_back(Enemy(180.f, 270.f, 80.f)); // Position more to the center with reduced patrol width
    
    std::cout << "Enemy 3 initialized at position: " << enemies.back().getPosition().x << ", " 
              << enemies.back().getPosition().y << std::endl;
    
    // Enemy on platform 4
    // Platform 4 is at (900, 350) with size 200x20
    enemies.push_back(Enemy(980.f, 320.f, 80.f));
    
    // Enemy on platform 6
    // Platform 6 is at (1500, 400) with size 200x20
    enemies.push_back(Enemy(1580.f, 370.f, 80.f));
    
    // Enemy on platform 7
    // Platform 7 is at (1800, 300) with size 200x20
    enemies.push_back(Enemy(1880.f, 270.f, 80.f));
    
    // Enemy on platform 9
    // Platform 9 is at (2400, 350) based on code
    enemies.push_back(Enemy(2480.f, 320.f, 80.f));
    
    // Verify all enemies were created properly
    std::cout << "Total enemies created: " << enemies.size() << std::endl;
    
    // Force enemies to start moving right
    for (auto& enemy : enemies) {
        // Set a positive initial velocity and reset position if too close to left edge
        sf::Vector2f pos = enemy.getPosition();
        if (pos.x < 50.0f) {
            pos.x = 100.0f; // Move away from edge
            enemy.setPosition(pos);
        }
        enemy.setVelocity(sf::Vector2f(2.0f, 0.0f)); // Ensure initial velocity is to the right
    }
}

void Game::checkGameOver() {
    // Check if player health is zero
    if (player.getHealth() <= 0 && currentState == GameState::Playing) {
        currentState = GameState::GameOver;
    }
}

void Game::resetGame() {
    // Reset player
    player = Player(50.f, WINDOW_HEIGHT - 100.f); // Start player higher above the ground
    
    // Set player collision box
    player.setCollisionBoxSize(sf::Vector2f(28.f, 28.f));
    
    // Reset view
    gameView.setCenter(sf::Vector2f(WINDOW_WIDTH / 2.f, WINDOW_HEIGHT / 2.f));
    
    // Reset game state
    playerHit = false;
    playerHitCooldown = 0.f;
    currentState = GameState::Playing;
    
    // Reinitialize everything
    initializePlatforms();
    initializeLadders();
    initializeEnemies();
    initializeUI();
    initializeMiniMap();
    initializeLights();
    
    // Initialize physics system
    physicsSystem.initialize();
    physicsSystem.initializePlayer(player);
    physicsSystem.initializePlatforms(platforms);
    physicsSystem.initializeEnemies(enemies);
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
            
            // Decrease player health
            player.decreaseHealth();
            
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

void Game::initializeLights() {
    lightingSystem.clearLights();
    
    // Add player light (follows the player)
    sf::Vector2f playerPos = player.getPosition();
    playerLightIndex = 0; // First light is the player's light
    lightingSystem.addLight(
        sf::Vector2f(playerPos.x + player.getSize().x / 2.f, playerPos.y + player.getSize().y / 2.f),
        150.0f,                    // Radius
        sf::Color(255, 220, 150),  // Warm light color
        0.8f                       // Intensity
    );
    
    // Add some static lights around the level
    
    // Light near the first ladder
    lightingSystem.addLight(
        sf::Vector2f(580.f, WINDOW_HEIGHT - 200.f),
        180.0f,                    // Radius
        sf::Color(150, 220, 255),  // Blueish light
        0.7f                       // Intensity
    );
    
    // Light on platform 3 (highest initial platform)
    lightingSystem.addLight(
        sf::Vector2f(600.f, 180.f),
        160.0f,                    // Radius
        sf::Color(255, 170, 100),  // Orange-ish light
        0.75f                      // Intensity
    );
    
    // Light on platform 5 in scrolling area
    lightingSystem.addLight(
        sf::Vector2f(1300.f, 230.f),
        170.0f,                    // Radius
        sf::Color(150, 255, 150),  // Green-ish light
        0.7f                       // Intensity
    );
    
    // Light on platform 8 in scrolling area
    lightingSystem.addLight(
        sf::Vector2f(2200.f, 180.f),
        170.0f,                    // Radius
        sf::Color(255, 150, 220),  // Pink-ish light
        0.7f                       // Intensity
    );
    
    // Light on final platform
    lightingSystem.addLight(
        sf::Vector2f(2850.f, 230.f),
        190.0f,                    // Radius
        sf::Color(255, 255, 150),  // Yellow-ish light
        0.8f                       // Intensity
    );
}

void Game::updateLights() {
    // Update player light position to follow player
    sf::Vector2f playerCenter(
        player.getPosition().x + player.getSize().x / 2.f,
        player.getPosition().y + player.getSize().y / 2.f
    );
    lightingSystem.updateLight(playerLightIndex, playerCenter);
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
    // Check if player has reached the end of the level (right edge)
    if (player.getPosition().x >= LEVEL_WIDTH - player.getSize().x - 50.f) {
        // Player has reached the end of the level
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
    }
}

void Game::nextLevel() {
    // Increment level
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
    
    // Reinitialize game elements with new variations based on level
    initializePlatforms();
    initializeLadders();
    initializeEnemies();
    initializeMiniMap();
    initializeLights();
    
    // For higher levels, increase difficulty by adding more enemies or making them faster
    if (currentLevel > 1) {
        // Add extra enemies based on level
        for (int i = 0; i < currentLevel; i++) {
            float x = 500.f + (i * 400.f);  // Space them out
            float y = WINDOW_HEIGHT - 70.f; // On the ground
            float patrolDistance = 160.f + (currentLevel * 20.f); // Longer patrol for higher levels
            enemies.push_back(Enemy(x, y, patrolDistance));
        }
        
        // Update minimap to include new enemies
        initializeMiniMap();
    }
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
                    ImGui::Checkbox("Show Lighting", &showLighting);
                    ImGui::Checkbox("Show Bounding Boxes", &showBoundingBoxes);
                    ImGui::Checkbox("Show Mini-map", &showMiniMap);
                    
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
                    
                    ImGui::EndTabItem();
                }
                
                // Gameplay tab
                if (ImGui::BeginTabItem("Gameplay")) {
                    ImGui::SliderFloat("Game Speed", &gameSpeed, 0.1f, 2.0f);
                    ImGui::SliderFloat("Player Speed", &playerSpeed, 50.0f, 400.0f);
                    
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
                    float playerBounce = physicsSystem.getPlayerBounceFactor();
                    
                    if (ImGui::SliderFloat("Player Width", &playerWidth, 0.5f, 1.5f, "%.2f")) {
                        physicsSystem.setPlayerCollisionSize(playerWidth, playerHeight);
                    }
                    if (ImGui::SliderFloat("Player Height", &playerHeight, 0.5f, 1.5f, "%.2f")) {
                        physicsSystem.setPlayerCollisionSize(playerWidth, playerHeight);
                    }
                    if (ImGui::SliderFloat("Player Bounce", &playerBounce, 0.0f, 1.0f, "%.2f")) {
                        physicsSystem.setPlayerBounceFactor(playerBounce);
                    }
                    
                    ImGui::Spacing();
                    
                    // Enemy collision box settings
                    ImGui::Text("Enemy Collision:");
                    float enemyWidth = physicsSystem.getEnemyCollisionWidth();
                    float enemyHeight = physicsSystem.getEnemyCollisionHeight();
                    float enemyBounce = physicsSystem.getEnemyBounceFactor();
                    
                    if (ImGui::SliderFloat("Enemy Width", &enemyWidth, 0.5f, 1.5f, "%.2f")) {
                        physicsSystem.setEnemyCollisionSize(enemyWidth, enemyHeight);
                    }
                    if (ImGui::SliderFloat("Enemy Height", &enemyHeight, 0.5f, 1.5f, "%.2f")) {
                        physicsSystem.setEnemyCollisionSize(enemyWidth, enemyHeight);
                    }
                    if (ImGui::SliderFloat("Enemy Bounce", &enemyBounce, 0.0f, 1.0f, "%.2f")) {
                        physicsSystem.setEnemyBounceFactor(enemyBounce);
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
        std::cerr << "Exception in updateImGui: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception in updateImGui" << std::endl;
    }
}

// Function to draw debug visualization
void Game::drawDebugBoxes() {
    if (showBoundingBoxes) {
        // Draw player's collision box
        sf::RectangleShape playerCollisionBox;
        sf::FloatRect physicsBox = physicsSystem.getPlayerPhysicsComponent().collisionBox;
        
        playerCollisionBox.setSize(sf::Vector2f(physicsBox.size.x, physicsBox.size.y));
        playerCollisionBox.setPosition(sf::Vector2f(physicsBox.position.x, physicsBox.position.y));
        playerCollisionBox.setFillColor(sf::Color(0, 255, 0, 50)); // Semi-transparent green
        playerCollisionBox.setOutlineColor(sf::Color(0, 255, 0)); // Green outline
        playerCollisionBox.setOutlineThickness(1.0f);
        window.draw(playerCollisionBox);
        
        // Draw enemy collision boxes
        for (size_t enemyIdx = 0; enemyIdx < enemies.size() && enemyIdx < physicsSystem.getEnemyPhysicsCount(); enemyIdx++) {
            sf::RectangleShape enemyCollisionBox;
            sf::FloatRect enemyPhysicsBox = physicsSystem.getEnemyPhysicsComponent(enemyIdx).collisionBox;
            
            enemyCollisionBox.setSize(sf::Vector2f(enemyPhysicsBox.size.x, enemyPhysicsBox.size.y));
            enemyCollisionBox.setPosition(sf::Vector2f(enemyPhysicsBox.position.x, enemyPhysicsBox.position.y));
            enemyCollisionBox.setFillColor(sf::Color(255, 0, 0, 50)); // Semi-transparent red
            enemyCollisionBox.setOutlineColor(sf::Color(255, 0, 0)); // Red outline
            enemyCollisionBox.setOutlineThickness(1.0f);
            window.draw(enemyCollisionBox);
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
            ImGui::TextUnformatted("Click to scan the assets directory for all files.\nDirectory contents are shown with [DIR] prefix.\nImage files are shown with [IMG] prefix.");
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
            bool isImage = asset.name.find("[IMG]") != std::string::npos;
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
            
            bool isImage = selectedAsset->name.find("[IMG]") != std::string::npos;
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
                        // Remove [IMG] prefix and any file extension
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
        std::cout << "Scanning directory: " << directory << std::endl;
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
                std::cout << "Trying alternative path: " << path << std::endl;
                if (fs::exists(path)) {
                    std::cout << "Found valid path: " << path << std::endl;
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
        std::cout << "Listing top-level entries in " << directory << ":" << std::endl;
        
        // Track directories for subdirectory display
        std::vector<std::string> subdirectories;
        
        // First scan the top directory (non-recursive)
        for (const auto& entry : fs::directory_iterator(directory)) {
            std::string path = entry.path().string();
            std::string name = entry.path().filename().string();
            
            std::cout << "Found: " << path << std::endl;
            
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
                        info.name = "[IMG] " + name;
                    }
                }
                
                imageAssets.push_back(info);
            }
        }
        
        // Now recursively scan image subdirectories
        for (const auto& subdir : subdirectories) {
            if (subdir.find("images") != std::string::npos) {
                std::cout << "Scanning image subdirectory: " << subdir << std::endl;
                
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
                            info.name = "[IMG] " + entry.path().filename().string();
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
        
        std::cout << "Asset scan complete. Found " << imageAssets.size() << " items." << std::endl;
                 
    } catch (const std::exception& e) {
        std::cerr << "Error scanning assets directory: " << e.what() << std::endl;
    }
} 