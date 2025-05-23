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
               player(50.f, WINDOW_HEIGHT - 100.f, physicsSystem), // Pass physicsSystem reference
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
               showEnemies(false),  // Show enemies by default
               showDebugGrid(false),
               gridSize(50.0f),
               gridColor(128, 128, 128, 64),  // Semi-transparent gray
               gridOriginColor(255, 255, 0, 128),  // Semi-transparent yellow for origin
               gridAxesColor(255, 255, 255, 96),   // Semi-transparent white for axes
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
        player.update(deltaTime, platforms, ladders);
        
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
        
        // Initialize placeholder shapes for drawing - make sure it covers the full screen
        backgroundPlaceholder.setSize(sf::Vector2f(LEVEL_WIDTH, WINDOW_HEIGHT));
        backgroundPlaceholder.setFillColor(sf::Color(100, 180, 100)); // Green
        backgroundPlaceholder.setPosition(sf::Vector2f(0, 0));
        
        playerPlaceholder.setSize(sf::Vector2f(32, 32));
        playerPlaceholder.setFillColor(sf::Color::Blue);
        
        enemyPlaceholder.setSize(sf::Vector2f(32, 32));
        enemyPlaceholder.setFillColor(sf::Color::Red);
        
        // Load backgrounds
        try {
            // First attempt - try loading from the new organized folder structure with more paths
            std::vector<std::string> backgroundPaths = {
                "assets/images/backgrounds/forest/forest_background.png",
                "assets/images/backgrounds/background.png",
                "../assets/images/backgrounds/background.png"
            };
            
            bool loaded = false;
            for (const auto& path : backgroundPaths) {
                try {
                    assets.loadTexture("background", path);
                    std::cout << "Successfully loaded background from: " << path << std::endl;
                    loaded = true;
                    break;
                } catch (const std::exception& e) {
                    std::cerr << "Failed to load background from " << path << ": " << e.what() << std::endl;
                }
            }
            
            if (loaded) {
                backgroundSprite = std::make_unique<sf::Sprite>(assets.getTexture("background"));
                backgroundTextureSize = assets.getTexture("background").getSize();
                useBackgroundPlaceholder = false;
                std::cout << "Successfully created background sprite with dimensions: " 
                         << backgroundTextureSize.x << "x" << backgroundTextureSize.y << std::endl;
            } else {
                std::cerr << "Failed to load background from any path, using placeholder" << std::endl;
            }
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
    // Clear existing platforms
    platforms.clear();
    
    // Different platform layouts based on level
    if (currentLevel % 3 == 1) {
        // Level 1 (or 4, 7...) - Forest theme with standard layout
        forestLevelPlatforms();
    } else if (currentLevel % 3 == 2) {
        // Level 2 (or 5, 8...) - Desert theme with spaced platforms
        desertLevelPlatforms();
    } else {
        // Level 3 (or 6, 9...) - Snow theme with vertical challenges
        snowLevelPlatforms();
    }
    
    // Ensure platforms have the correct color applied before physics initialization
    for (auto& platform : platforms) {
        platform.setFillColor(platformColor);
    }
    
    // Reinitialize physics system with the new platforms
    physicsSystem.initialize();
    physicsSystem.initializePlatforms(platforms);
    
    // Debug ground platform info
    auto& ground = platforms[0]; // First platform is ground
    std::cout << "Ground platform position: " << ground.getPosition().x << ", " << ground.getPosition().y 
              << " size: " << ground.getSize().x << ", " << ground.getSize().y << std::endl;
}

void Game::forestLevelPlatforms() {
    // Ground platform
    sf::RectangleShape ground;
    ground.setSize(sf::Vector2f(LEVEL_WIDTH, 60.f));
    ground.setPosition(sf::Vector2f(0, WINDOW_HEIGHT - 60.f));
    ground.setFillColor(platformColor);
    platforms.push_back(ground);

    // Platform layout focused on horizontal traversal
    // Platform 1
    sf::RectangleShape platform1;
    platform1.setSize(sf::Vector2f(200.f, 20.f));
    platform1.setPosition(sf::Vector2f(300.f, 400.f));
    platform1.setFillColor(platformColor);
    platforms.push_back(platform1);

    // Platform 2
    sf::RectangleShape platform2;
    platform2.setSize(sf::Vector2f(200.f, 20.f));
    platform2.setPosition(sf::Vector2f(100.f, 300.f));
    platform2.setFillColor(platformColor);
    platforms.push_back(platform2);

    // Platform 3
    sf::RectangleShape platform3;
    platform3.setSize(sf::Vector2f(200.f, 20.f));
    platform3.setPosition(sf::Vector2f(500.f, 200.f));
    platform3.setFillColor(platformColor);
    platforms.push_back(platform3);
    
    // Forest specific - longer platforms with gaps
    sf::RectangleShape platform4;
    platform4.setSize(sf::Vector2f(300.f, 20.f));
    platform4.setPosition(sf::Vector2f(900.f, 350.f));
    platform4.setFillColor(platformColor);
    platforms.push_back(platform4);
    
    sf::RectangleShape platform5;
    platform5.setSize(sf::Vector2f(300.f, 20.f));
    platform5.setPosition(sf::Vector2f(1300.f, 250.f));
    platform5.setFillColor(platformColor);
    platforms.push_back(platform5);
    
    sf::RectangleShape platform6;
    platform6.setSize(sf::Vector2f(300.f, 20.f));
    platform6.setPosition(sf::Vector2f(1700.f, 350.f));
    platform6.setFillColor(platformColor);
    platforms.push_back(platform6);
    
    sf::RectangleShape platform7;
    platform7.setSize(sf::Vector2f(300.f, 20.f));
    platform7.setPosition(sf::Vector2f(2100.f, 250.f));
    platform7.setFillColor(platformColor);
    platforms.push_back(platform7);
    
    sf::RectangleShape platform8;
    platform8.setSize(sf::Vector2f(300.f, 20.f));
    platform8.setPosition(sf::Vector2f(2500.f, 350.f));
    platform8.setFillColor(platformColor);
    platforms.push_back(platform8);
    
    // End platform
    sf::RectangleShape platform9;
    platform9.setSize(sf::Vector2f(200.f, 20.f));
    platform9.setPosition(sf::Vector2f(2850.f, 250.f));
    platform9.setFillColor(platformColor);
    platforms.push_back(platform9);
}

void Game::desertLevelPlatforms() {
    // Ground platform - desert has some gaps in the ground, but safer layout
    sf::RectangleShape ground1;
    ground1.setSize(sf::Vector2f(950.f, 60.f));  // Extended to x=950 to reduce gap
    ground1.setPosition(sf::Vector2f(0, WINDOW_HEIGHT - 60.f));
    ground1.setFillColor(platformColor);
    platforms.push_back(ground1);
    
    sf::RectangleShape ground2;
    ground2.setSize(sf::Vector2f(850.f, 60.f));  // Extended and moved closer
    ground2.setPosition(sf::Vector2f(980.f, WINDOW_HEIGHT - 60.f));  // Moved from 1000 to 980 (smaller gap)
    ground2.setFillColor(platformColor);
    platforms.push_back(ground2);
    
    sf::RectangleShape ground3;
    ground3.setSize(sf::Vector2f(1000.f, 60.f));
    ground3.setPosition(sf::Vector2f(2000.f, WINDOW_HEIGHT - 60.f));
    ground3.setFillColor(platformColor);
    platforms.push_back(ground3);

    // Desert has more scattered platforms with various heights - improved layout
    // Platform 1-3 with better spacing and less height difference
    for (int i = 0; i < 3; i++) {
        sf::RectangleShape platform;
        platform.setSize(sf::Vector2f(180.f, 20.f));  // Wider platforms for easier landing
        platform.setPosition(sf::Vector2f(200.f + i * 230.f, 420.f - i * 30.f));  // Reduced height difference from 40 to 30
        platform.setFillColor(platformColor);
        platforms.push_back(platform);
    }
    
    // Middle section - floating islands
    for (int i = 0; i < 5; i++) {
        sf::RectangleShape platform;
        platform.setSize(sf::Vector2f(180.f, 20.f));
        float x = 1100.f + i * 300.f;
        // Alternating heights
        float y = (i % 2 == 0) ? 300.f : 420.f;
        platform.setPosition(sf::Vector2f(x, y));
        platform.setFillColor(platformColor);
        platforms.push_back(platform);
    }
    
    // End platforms
    sf::RectangleShape platform8;
    platform8.setSize(sf::Vector2f(200.f, 20.f));
    platform8.setPosition(sf::Vector2f(2600.f, 350.f));
    platform8.setFillColor(platformColor);
    platforms.push_back(platform8);
    
    sf::RectangleShape platform9;
    platform9.setSize(sf::Vector2f(200.f, 20.f));
    platform9.setPosition(sf::Vector2f(2800.f, 250.f));
    platform9.setFillColor(platformColor);
    platforms.push_back(platform9);
}

void Game::snowLevelPlatforms() {
    // Ground platform - snow level has full ground
    sf::RectangleShape ground;
    ground.setSize(sf::Vector2f(LEVEL_WIDTH, 60.f));
    ground.setPosition(sf::Vector2f(0, WINDOW_HEIGHT - 60.f));
    ground.setFillColor(platformColor);
    platforms.push_back(ground);

    // Snow level has more vertical challenges - staggered platforms
    // First section - climbing pattern
    for (int i = 0; i < 4; i++) {
        sf::RectangleShape platform;
        platform.setSize(sf::Vector2f(150.f, 20.f));
        float x = 200.f + (i % 2) * 200.f;
        float y = 450.f - i * 80.f;
        platform.setPosition(sf::Vector2f(x, y));
        platform.setFillColor(platformColor);
        platforms.push_back(platform);
    }
    
    // Middle section - icy platforms (visually the same but gameplay logic could change)
    for (int i = 0; i < 4; i++) {
        sf::RectangleShape platform;
        platform.setSize(sf::Vector2f(220.f, 20.f));
        platform.setPosition(sf::Vector2f(800.f + i * 350.f, 250.f));
        platform.setFillColor(platformColor);
        platforms.push_back(platform);
    }
    
    // Final section - descending platforms
    for (int i = 0; i < 4; i++) {
        sf::RectangleShape platform;
        platform.setSize(sf::Vector2f(180.f, 20.f));
        platform.setPosition(sf::Vector2f(2200.f + i * 180.f, 200.f + i * 50.f));
        platform.setFillColor(platformColor);
        platforms.push_back(platform);
    }
}

void Game::initializeLadders() {
    // Clear existing ladders
    ladders.clear();
    
    // Different ladder layouts based on level
    sf::Color ladderColor;
    
    // Adjust ladder colors based on level theme
    switch (currentLevel % 3) {
        case 1: // Forest theme
            ladderColor = sf::Color(139, 69, 19); // Brown wooden ladders
            break;
        case 2: // Desert theme
            ladderColor = sf::Color(205, 133, 63); // Sandy/rope ladders
            break;
        default: // Snow theme
            ladderColor = sf::Color(220, 220, 250); // Ice/metal ladders
            break;
    }
    
    if (currentLevel % 3 == 1) {
        // Forest level - standard ladders
        
        // Ladder to reach higher platform
        sf::RectangleShape ladder1;
        ladder1.setSize(sf::Vector2f(30.f, 380.f));
        ladder1.setPosition(sf::Vector2f(580.f, WINDOW_HEIGHT - 40.f - 380.f));
        ladder1.setFillColor(ladderColor);
        ladders.push_back(ladder1);
        
        // Middle section ladders
        sf::RectangleShape ladder2;
        ladder2.setSize(sf::Vector2f(30.f, 350.f));
        ladder2.setPosition(sf::Vector2f(1400.f, 250.f - 100.f));
        ladder2.setFillColor(ladderColor);
        ladders.push_back(ladder2);
        
        // Final ladder
        sf::RectangleShape ladder3;
        ladder3.setSize(sf::Vector2f(30.f, 350.f));
        ladder3.setPosition(sf::Vector2f(2850.f, 250.f - 100.f));
        ladder3.setFillColor(ladderColor);
        ladders.push_back(ladder3);
    }
    else if (currentLevel % 3 == 2) {
        // Desert level - fewer, but taller ladders
        
        // First ladder to climb out of pit
        sf::RectangleShape ladder1;
        ladder1.setSize(sf::Vector2f(30.f, 200.f));
        ladder1.setPosition(sf::Vector2f(950.f, WINDOW_HEIGHT - 60.f - 200.f));
        ladder1.setFillColor(ladderColor);
        ladders.push_back(ladder1);
        
        // Middle ladder
        sf::RectangleShape ladder2;
        ladder2.setSize(sf::Vector2f(30.f, 420.f));
        ladder2.setPosition(sf::Vector2f(1850.f, 300.f - 100.f));
        ladder2.setFillColor(ladderColor);
        ladders.push_back(ladder2);
        
        // Final ladder
        sf::RectangleShape ladder3;
        ladder3.setSize(sf::Vector2f(30.f, 350.f));
        ladder3.setPosition(sf::Vector2f(2750.f, 250.f - 150.f));
        ladder3.setFillColor(ladderColor);
        ladders.push_back(ladder3);
    }
    else {
        // Snow level - more ladders for vertical traversal
        
        // Several shorter ladders in the first section
        for (int i = 0; i < 3; i++) {
            sf::RectangleShape ladder;
            ladder.setSize(sf::Vector2f(30.f, 150.f));
            ladder.setPosition(sf::Vector2f(250.f + i * 180.f, 450.f - i * 80.f - 150.f));
            ladder.setFillColor(ladderColor);
            ladders.push_back(ladder);
        }
        
        // Middle ladders
        sf::RectangleShape ladder4;
        ladder4.setSize(sf::Vector2f(30.f, 300.f));
        ladder4.setPosition(sf::Vector2f(1100.f, 250.f - 100.f));
        ladder4.setFillColor(ladderColor);
        ladders.push_back(ladder4);
        
        sf::RectangleShape ladder5;
        ladder5.setSize(sf::Vector2f(30.f, 300.f));
        ladder5.setPosition(sf::Vector2f(1800.f, 250.f - 100.f));
        ladder5.setFillColor(ladderColor);
        ladders.push_back(ladder5);
        
        // Final ladders - longer for the final section
        sf::RectangleShape ladder6;
        ladder6.setSize(sf::Vector2f(30.f, 400.f));
        ladder6.setPosition(sf::Vector2f(2700.f, 350.f - 150.f));
        ladder6.setFillColor(ladderColor);
        ladders.push_back(ladder6);
    }
}

void Game::initializeEnemies() {
    // Clear existing enemies first
    enemies.clear();
    
    // Different enemy patterns based on level theme
    if (currentLevel % 3 == 1) {
        // Forest level - standard enemy pattern
        initializeForestEnemies();
    } else if (currentLevel % 3 == 2) {
        // Desert level - more ground enemies, fewer platform enemies
        initializeDesertEnemies();
    } else {
        // Snow level - enemies concentrated in middle section
        initializeSnowEnemies();
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
    std::cout << "Total enemies created: " << enemies.size() << std::endl;
}

void Game::initializeForestEnemies() {
    // Enemy on ground
    enemies.push_back(Enemy(400.f, WINDOW_HEIGHT - 90.f, 180.f));
    
    // Enemy on first platform
    enemies.push_back(Enemy(380.f, 370.f, 80.f));
    
    // Enemy on second platform
    enemies.push_back(Enemy(180.f, 270.f, 80.f));
    
    // Middle section enemies
    enemies.push_back(Enemy(980.f, 320.f, 100.f));
    enemies.push_back(Enemy(1400.f, 220.f, 120.f));
    enemies.push_back(Enemy(1800.f, 320.f, 120.f));
    
    // End section enemy
    enemies.push_back(Enemy(2600.f, 320.f, 120.f));
}

void Game::initializeDesertEnemies() {
    // More ground enemies in the desert
    enemies.push_back(Enemy(300.f, WINDOW_HEIGHT - 90.f, 200.f));
    enemies.push_back(Enemy(600.f, WINDOW_HEIGHT - 90.f, 200.f));
    
    // Desert has more ground enemies
    if (currentLevel > 2) {
        enemies.push_back(Enemy(1200.f, WINDOW_HEIGHT - 90.f, 220.f));
        enemies.push_back(Enemy(2200.f, WINDOW_HEIGHT - 90.f, 220.f));
    }
    
    // Few platform enemies
    enemies.push_back(Enemy(350.f, 320.f, 100.f));
    
    // Middle section has faster enemies with wider patrol range
    enemies.push_back(Enemy(1250.f, 270.f, 150.f));
    enemies.push_back(Enemy(1850.f, 390.f, 130.f));
    
    // Final platform enemy
    enemies.push_back(Enemy(2700.f, 220.f, 100.f));
}

void Game::initializeSnowEnemies() {
    // Snow level has fewer ground enemies due to the cold
    enemies.push_back(Enemy(500.f, WINDOW_HEIGHT - 90.f, 150.f));
    
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

void Game::checkGameOver() {
    // Check if player health is zero
    if (player.getHealth() <= 0 && currentState == GameState::Playing) {
        currentState = GameState::GameOver;
    }
}

void Game::resetGame() {
    // Reset player
    player.reset(50.f, WINDOW_HEIGHT - 100.f); // Start player higher above the ground
    
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
    
    // Change background and theme based on level
    try {
        // Different backgrounds for each level
        std::string levelBackgroundPath;
        std::vector<std::string> alternativePaths;
        
        switch (currentLevel % 3) {
            case 1: // Forest theme (levels 1, 4, 7...)
                levelBackgroundPath = "assets/images/backgrounds/forest/forest_background.png";
                platformColor = sf::Color(34, 139, 34); // Forest green
                alternativePaths = {
                    "assets/images/backgrounds/forest_background.png",
                    "assets/images/backgrounds/background.png",
                    "../assets/images/backgrounds/background.png"
                };
                break;
                
            case 2: // Desert theme (levels 2, 5, 8...)
                levelBackgroundPath = "assets/images/backgrounds/desert/desert_background.png";
                platformColor = sf::Color(210, 180, 140); // Desert sand color
                alternativePaths = {
                    "assets/images/backgrounds/desert_background.png",
                    "assets/images/backgrounds/background.png",
                    "../assets/images/backgrounds/background.png"
                };
                break;
                
            case 0: // Snow theme (levels 3, 6, 9...)
                levelBackgroundPath = "assets/images/backgrounds/snow/snow_background.png";
                platformColor = sf::Color(200, 220, 255); // Light blue for snow
                alternativePaths = {
                    "assets/images/backgrounds/snow_background.png",
                    "assets/images/backgrounds/background.png",
                    "../assets/images/backgrounds/background.png"
                };
                break;
        }
        
        // Try to load the level-specific background
        bool loaded = false;
        try {
            assets.loadTexture("background", levelBackgroundPath);
            std::cout << "Successfully loaded background: " << levelBackgroundPath << std::endl;
            loaded = true;
        } 
        catch (const std::exception& e) {
            std::cerr << "Failed to load primary background: " << e.what() << std::endl;
            
            // Try alternative paths
            for (const auto& path : alternativePaths) {
                try {
                    assets.loadTexture("background", path);
                    std::cout << "Successfully loaded alternative background: " << path << std::endl;
                    loaded = true;
                    break;
                } catch (const std::exception& innerE) {
                    std::cerr << "Failed to load alternative background from " << path << ": " << innerE.what() << std::endl;
                }
            }
        }
        
        // If we loaded a background texture, create the sprite
        if (loaded) {
            // Update the sprite with new texture
            backgroundSprite = std::make_unique<sf::Sprite>(assets.getTexture("background"));
            backgroundTextureSize = assets.getTexture("background").getSize();
            useBackgroundPlaceholder = false;
            
            std::cout << "Successfully created background sprite with dimensions: " 
                      << backgroundTextureSize.x << "x" << backgroundTextureSize.y << std::endl;
            
            // Apply a slight tint to the background based on the level theme
            // This helps create more visual distinction between levels
            if (currentLevel % 3 == 1) { // Forest - slightly more green
                backgroundSprite->setColor(sf::Color(230, 255, 230));
            } 
            else if (currentLevel % 3 == 2) { // Desert - warm/orange tint
                backgroundSprite->setColor(sf::Color(255, 240, 220));
            } 
            else { // Snow - cool/blue tint
                backgroundSprite->setColor(sf::Color(230, 240, 255));
            }
        }
        // If we couldn't load a background texture, use the placeholder
        else {
            useBackgroundPlaceholder = true;
            
            // Adjust placeholder color based on level theme
            if (currentLevel % 3 == 1) { // Forest
                backgroundPlaceholder.setFillColor(sf::Color(100, 180, 100)); // Green
            } 
            else if (currentLevel % 3 == 2) { // Desert
                backgroundPlaceholder.setFillColor(sf::Color(210, 180, 140)); // Sand
            } 
            else { // Snow
                backgroundPlaceholder.setFillColor(sf::Color(200, 220, 255)); // Light blue
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to load background for level " << currentLevel << ": " << e.what() << std::endl;
        useBackgroundPlaceholder = true;
        
        // Adjust placeholder color based on level theme
        if (currentLevel % 3 == 1) { // Forest
            backgroundPlaceholder.setFillColor(sf::Color(100, 180, 100)); // Green
        } 
        else if (currentLevel % 3 == 2) { // Desert
            backgroundPlaceholder.setFillColor(sf::Color(210, 180, 140)); // Sand
        } 
        else { // Snow
            backgroundPlaceholder.setFillColor(sf::Color(200, 220, 255)); // Light blue
        }
    }
    
    // Reinitialize game elements with new variations based on level
    initializePlatforms();
    initializeLadders();
    initializeEnemies();
    initializeMiniMap();
    initializeLights();
    
    // For higher levels, increase difficulty and add level-specific features
    if (currentLevel > 1) {
        // Add extra enemies based on level
        for (int i = 0; i < currentLevel; i++) {
            float x = 500.f + (i * 400.f);  // Space them out
            float y = WINDOW_HEIGHT - 70.f; // On the ground
            float patrolDistance = 160.f + (currentLevel * 20.f); // Longer patrol for higher levels
            enemies.push_back(Enemy(x, y, patrolDistance));
        }
        
        // Level-specific physics changes
        switch (currentLevel % 3) {
            case 1: // Forest level - normal physics
                physicsSystem.setGravity(10.0f);
                physicsSystem.setJumpForce(400.f);
                break;
            case 2: // Desert level - same gravity, slightly different jump
                physicsSystem.setGravity(10.0f);
                physicsSystem.setJumpForce(420.f);
                break;
            case 0: // Snow level - same gravity, different jump
                physicsSystem.setGravity(10.0f);
                physicsSystem.setJumpForce(350.f);
                break;
        }
        
        // Ensure physics components for enemies are reinitialized
        physicsSystem.initializeEnemies(enemies);
        
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
                    
                    // Debug grid controls
                    ImGui::Separator();
                    ImGui::Text("Debug Grid");
                    ImGui::Checkbox("Show Debug Grid", &showDebugGrid);
                    
                    if (showDebugGrid) {
                        ImGui::SliderFloat("Grid Size", &gridSize, 10.0f, 200.0f, "%.0f");
                        
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
                    if (ImGui::SliderInt("Select Level", &selectedLevel, 1, 20)) {
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
                    if (ImGui::Button("Level 1 (Forest)")) {
                        jumpToLevel(1);
                        selectedLevel = 1;
                    }
                    
                    if (ImGui::Button("Level 2 (Desert)")) {
                        jumpToLevel(2);
                        selectedLevel = 2;
                    }
                    
                    ImGui::SameLine();
                    
                    if (ImGui::Button("Level 3 (Snow)")) {
                        jumpToLevel(3);
                        selectedLevel = 3;
                    }
                    
                    // Level theme info
                    ImGui::Separator();
                    ImGui::Text("Level Themes:");
                    ImGui::BulletText("Levels 1, 4, 7... = Forest (Normal physics)");
                    ImGui::BulletText("Levels 2, 5, 8... = Desert (Higher gravity)");
                    ImGui::BulletText("Levels 3, 6, 9... = Snow (Lower gravity)");
                    
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
    }
}

// Function to draw canonical coordinate grid for debugging
void Game::drawDebugGrid() {
    if (!showDebugGrid) return;
    
    // Get the current view bounds to only draw grid lines that are visible
    sf::Vector2f viewCenter = gameView.getCenter();
    sf::Vector2f viewSize = gameView.getSize();
    
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
        window.draw(gridLines);
    }
    if (axisLines.getVertexCount() > 0) {
        window.draw(axisLines);
    }
    if (originLines.getVertexCount() > 0) {
        window.draw(originLines);
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
// Method to synchronize platforms with their physics components
void Game::syncPlatformsWithPhysics() {
    // Make sure we have physics components for each platform
    if (platforms.size() != physicsSystem.getPlatformPhysicsCount()) {
        std::cout << "Warning: Platform count mismatch. Platforms: " << platforms.size() 
                  << ", Physics components: " << physicsSystem.getPlatformPhysicsCount() << std::endl;
        return;
    }
    
    // For each platform, update its position and size to match the collision box
    for (size_t i = 0; i < platforms.size(); ++i) {
        const auto& physicsBox = physicsSystem.getPlatformPhysicsComponent(i).collisionBox;
        platforms[i].setPosition(physicsBox.position);
        platforms[i].setSize(physicsBox.size);
    }
    
    std::cout << "Synchronized " << platforms.size() << " platforms with physics components" << std::endl;
}

void Game::jumpToLevel(int level) {
    // Ensure level is valid (at least 1)
    if (level < 1) {
        level = 1;
    }
    
    // Set the current level
    currentLevel = level;
    
    // Reset player position and health
    player.reset(50.f, WINDOW_HEIGHT - 100.f);
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
    
    // Change background and theme based on level
    try {
        // Different backgrounds for each level
        std::string levelBackgroundPath;
        std::vector<std::string> alternativePaths;
        
        switch (currentLevel % 3) {
            case 1: // Forest theme (levels 1, 4, 7...)
                levelBackgroundPath = "assets/images/backgrounds/forest/forest_background.png";
                platformColor = sf::Color(34, 139, 34); // Forest green
                alternativePaths = {
                    "assets/images/backgrounds/forest_background.png",
                    "assets/images/backgrounds/background.png",
                    "../assets/images/backgrounds/background.png"
                };
                break;
                
            case 2: // Desert theme (levels 2, 5, 8...)
                levelBackgroundPath = "assets/images/backgrounds/desert/desert_background.png";
                platformColor = sf::Color(210, 180, 140); // Desert sand color
                alternativePaths = {
                    "assets/images/backgrounds/desert_background.png",
                    "assets/images/backgrounds/background.png",
                    "../assets/images/backgrounds/background.png"
                };
                break;
                
            case 0: // Snow theme (levels 3, 6, 9...)
                levelBackgroundPath = "assets/images/backgrounds/snow/snow_background.png";
                platformColor = sf::Color(200, 220, 255); // Light blue for snow
                alternativePaths = {
                    "assets/images/backgrounds/snow_background.png",
                    "assets/images/backgrounds/background.png",
                    "../assets/images/backgrounds/background.png"
                };
                break;
        }
        
        // Try to load the level-specific background
        bool loaded = false;
        try {
            assets.loadTexture("background", levelBackgroundPath);
            std::cout << "Successfully loaded background: " << levelBackgroundPath << std::endl;
            loaded = true;
        } 
        catch (const std::exception& e) {
            std::cerr << "Failed to load primary background: " << e.what() << std::endl;
            
            // Try alternative paths
            for (const auto& path : alternativePaths) {
                try {
                    assets.loadTexture("background", path);
                    std::cout << "Successfully loaded alternative background: " << path << std::endl;
                    loaded = true;
                    break;
                } catch (const std::exception& innerE) {
                    std::cerr << "Failed to load alternative background from " << path << ": " << innerE.what() << std::endl;
                }
            }
        }
        
        // If we loaded a background texture, create the sprite
        if (loaded) {
            // Update the sprite with new texture
            backgroundSprite = std::make_unique<sf::Sprite>(assets.getTexture("background"));
            backgroundTextureSize = assets.getTexture("background").getSize();
            useBackgroundPlaceholder = false;
            
            std::cout << "Successfully created background sprite with dimensions: " 
                      << backgroundTextureSize.x << "x" << backgroundTextureSize.y << std::endl;
            
            // Apply a slight tint to the background based on the level theme
            if (currentLevel % 3 == 1) { // Forest - slightly more green
                backgroundSprite->setColor(sf::Color(230, 255, 230));
            } 
            else if (currentLevel % 3 == 2) { // Desert - warm/orange tint
                backgroundSprite->setColor(sf::Color(255, 240, 220));
            } 
            else { // Snow - cool/blue tint
                backgroundSprite->setColor(sf::Color(230, 240, 255));
            }
        }
        // If we couldn't load a background texture, use the placeholder
        else {
            useBackgroundPlaceholder = true;
            
            // Adjust placeholder color based on level theme
            if (currentLevel % 3 == 1) { // Forest
                backgroundPlaceholder.setFillColor(sf::Color(100, 180, 100)); // Green
            } 
            else if (currentLevel % 3 == 2) { // Desert
                backgroundPlaceholder.setFillColor(sf::Color(210, 180, 140)); // Sand
            } 
            else { // Snow
                backgroundPlaceholder.setFillColor(sf::Color(200, 220, 255)); // Light blue
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to load background for level " << currentLevel << ": " << e.what() << std::endl;
        useBackgroundPlaceholder = true;
        
        // Adjust placeholder color based on level theme
        if (currentLevel % 3 == 1) { // Forest
            backgroundPlaceholder.setFillColor(sf::Color(100, 180, 100)); // Green
        } 
        else if (currentLevel % 3 == 2) { // Desert
            backgroundPlaceholder.setFillColor(sf::Color(210, 180, 140)); // Sand
        } 
        else { // Snow
            backgroundPlaceholder.setFillColor(sf::Color(200, 220, 255)); // Light blue
        }
    }
    
    // Reinitialize game elements with new variations based on level
    initializePlatforms();
    initializeLadders();
    initializeEnemies();
    initializeUI();
    initializeMiniMap();
    initializeLights();
    
    // Apply level-specific physics and difficulty
    if (currentLevel > 1) {
        // Add extra enemies based on level
        for (int i = 0; i < currentLevel; i++) {
            float x = 500.f + (i * 400.f);  // Space them out
            float y = WINDOW_HEIGHT - 70.f; // On the ground
            float patrolDistance = 160.f + (currentLevel * 20.f); // Longer patrol for higher levels
            enemies.push_back(Enemy(x, y, patrolDistance));
        }
    }
    
    // Level-specific physics changes
    switch (currentLevel % 3) {
        case 1: // Forest level - normal physics
            physicsSystem.setGravity(10.0f);
            physicsSystem.setJumpForce(400.f);
            break;
        case 2: // Desert level - same gravity, slightly different jump
            physicsSystem.setGravity(10.0f);
            physicsSystem.setJumpForce(420.f);
            break;
        case 0: // Snow level - same gravity, different jump
            physicsSystem.setGravity(10.0f);
            physicsSystem.setJumpForce(350.f);
            break;
    }
    
    // Initialize physics system
    physicsSystem.initialize();
    physicsSystem.initializePlayer(player);
    physicsSystem.initializePlatforms(platforms);
    physicsSystem.initializeEnemies(enemies);
    
    std::cout << "Jumped to level " << currentLevel << " for testing" << std::endl;
}
