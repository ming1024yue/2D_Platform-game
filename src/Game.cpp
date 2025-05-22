#include "Game.hpp"
#include <iostream>
#include <cstdint> // For uint8_t
#include <sstream>
#include <iomanip>

// Helper function for rectangle intersection (for SFML 3.x compatibility)
static bool rectsIntersect(const sf::FloatRect& a, const sf::FloatRect& b) {
    return a.position.x < b.position.x + b.size.x &&
           a.position.x + a.size.x > b.position.x &&
           a.position.y < b.position.y + b.size.y &&
           a.position.y + a.size.y > b.position.y;
}


Game::Game() : window(sf::VideoMode(sf::Vector2u(WINDOW_WIDTH, WINDOW_HEIGHT)), "2D Platform Puzzle Game"),
               player(50.f, WINDOW_HEIGHT / 2.f),
               playerHit(false),
               playerHitCooldown(0.f),
               currentState(GameState::Playing),
               healthText(defaultFont, sf::String("HP: 3"), 24),
               gameOverText(defaultFont, sf::String("GAME OVER"), 48),
               restartText(defaultFont, sf::String("Press ENTER to restart"), 24),
               debugPanelTitle(defaultFont, sf::String("Game Settings Panel"), 24),
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
               currentDebugTab(0),
               showBoundingBoxes(true),
               gameSpeed(1.0f),
               platformColor(34, 139, 34),
               playerBorderColor(0, 255, 0),
               enemyBorderColor(255, 0, 0),
               spriteScale(4.0f),
               boundaryBoxHeight(0.67f),
               fpsUpdateTime(0.0f),
               frameCount(0),
               currentFPS(0.0f) {
    
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
    initializeDebugPanel();
    
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
}

// Load all game assets using the AssetManager
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
            assets.loadTexture("player", "../assets/images/characters/player.png");
            playerSprite = std::make_unique<sf::Sprite>(assets.getTexture("player"));
            usePlayerPlaceholder = false;
        } catch (const std::exception& e) {
            std::cerr << "Failed to load player sprite: " << e.what() << std::endl;
            // We'll use the player placeholder instead
        }
        
        // Load enemy sprites
        try {
            assets.loadTexture("enemy", "../assets/images/enemies/enemy.png");
            enemySprite = std::make_unique<sf::Sprite>(assets.getTexture("enemy"));
            useEnemyPlaceholder = false;
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
    // Ground platform - extend it to cover the full level width
    sf::RectangleShape ground;
    ground.setSize(sf::Vector2f(LEVEL_WIDTH, 40.f));
    ground.setPosition(sf::Vector2f(0, WINDOW_HEIGHT - 40.f));
    ground.setFillColor(sf::Color(34, 139, 34)); // Forest Green color for ground
    platforms.push_back(ground);

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
    // Add enemies on different platforms
    
    // Enemy on ground
    enemies.push_back(Enemy(400.f, WINDOW_HEIGHT - 70.f, 200.f));
    
    // Enemy on platform 1
    enemies.push_back(Enemy(320.f, 370.f, 160.f));
    
    // Enemy on platform 4
    enemies.push_back(Enemy(920.f, 320.f, 160.f));
    
    // Enemy on platform 6
    enemies.push_back(Enemy(1520.f, 370.f, 160.f));
    
    // Enemy on platform 7
    enemies.push_back(Enemy(1820.f, 270.f, 160.f));
    
    // Enemy on platform 9
    enemies.push_back(Enemy(2420.f, 320.f, 160.f));
}

void Game::handleEvents() {
    while (auto event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window.close();
        }
        if (auto key = event->getIf<sf::Event::KeyPressed>()) {
            if (key->code == sf::Keyboard::Key::Escape) {
                // If in debug panel, return to previous state
                if (currentState == GameState::DebugPanel) {
                    currentState = previousState;
                } else {
                    window.close();
                }
            }
            
            // Toggle minimap with M key
            if (key->code == sf::Keyboard::Key::M) {
                showMiniMap = !showMiniMap;
            }
            
            // Toggle lighting with L key
            if (key->code == sf::Keyboard::Key::L) {
                showLighting = !showLighting;
            }
            
            // Toggle debug panel with F1 key
            if (key->code == sf::Keyboard::Key::F1) {
                if (currentState == GameState::DebugPanel) {
                    currentState = previousState;
                } else {
                    previousState = currentState;
                    currentState = GameState::DebugPanel;
                }
            }
            
            // Handle restart when in game over state
            if (currentState == GameState::GameOver && key->code == sf::Keyboard::Key::Enter) {
                resetGame();
            }
        }
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
    player = Player(50.f, WINDOW_HEIGHT / 2.f);
    
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

void Game::update() {
    // Update FPS counter
    updateFPS();
    
    if (currentState == GameState::Playing) {
        // Update player
        player.update(platforms, ladders);
        
        // Update enemies
        for (auto& enemy : enemies) {
            enemy.update(platforms);
        }
        
        // Check collisions between player and enemies
        checkPlayerEnemyCollision();
        
        // Check if game over
        checkGameOver();
        
        // Check if level is completed
        checkLevelCompletion();
        
        // Update UI elements
        updateUI();
        
        // Update mini-map
        updateMiniMap();
        
        // Update lights
        updateLights();
        
        // Update the view to follow the player horizontally
        float playerX = player.getPosition().x + player.getSize().x / 2.f;
        float viewX = playerX;
        
        // Keep the view within level boundaries
        if (viewX < WINDOW_WIDTH / 2.f) {
            viewX = WINDOW_WIDTH / 2.f;
        } else if (viewX > LEVEL_WIDTH - WINDOW_WIDTH / 2.f) {
            viewX = LEVEL_WIDTH - WINDOW_WIDTH / 2.f;
        }
        
        gameView.setCenter(sf::Vector2f(viewX, WINDOW_HEIGHT / 2.f));
    }
    else if (currentState == GameState::LevelTransition) {
        // Handle level transition
        transitionTimer -= 1.0f / FPS;
        
        if (transitionTimer <= 0) {
            // Transition to next level
            nextLevel();
        }
    }
    
    // Always set the view for drawing
    window.setView(gameView);
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

void Game::draw() {
    window.clear(sf::Color(100, 100, 255)); // Sky blue background
    
    // Set the game view for scrolling game world
    window.setView(gameView);
    
    // Draw background 
    if (useBackgroundPlaceholder) {
        // Draw the green rectangle placeholder
        window.draw(backgroundPlaceholder);
    } else if (backgroundSprite) {
        // Calculate how many background tiles we need based on view position
        sf::Vector2f viewCenter = gameView.getCenter();
        sf::Vector2f viewSize = gameView.getSize();
        
        // Calculate the visible area
        float leftX = viewCenter.x - viewSize.x / 2.0f;
        float rightX = viewCenter.x + viewSize.x / 2.0f;
        
        // Determine the first tile position (round down to nearest tile width)
        float startX = std::floor(leftX / backgroundTextureSize.x) * backgroundTextureSize.x;
        
        // Draw enough tiles to cover the visible area plus a bit extra
        for (float x = startX; x < rightX + backgroundTextureSize.x; x += backgroundTextureSize.x) {
            backgroundSprite->setPosition(sf::Vector2f(x, 0));
            window.draw(*backgroundSprite);
        }
    }
    
    // Draw platforms
    for (const auto& platform : platforms) {
        // Apply platform color from settings
        sf::RectangleShape coloredPlatform = platform;
        coloredPlatform.setFillColor(platformColor);
        window.draw(coloredPlatform);
    }
    
    // Draw ladders
    for (const auto& ladder : ladders) {
        window.draw(ladder);
    }
    
    // Draw enemies - first draw the shapes from Enemy class
    for (const auto& enemy : enemies) {
        // Skip drawing the solid enemy shape
        // enemy.draw(window);
        
        // Then draw enemy sprite on top if available
        if (!useEnemyPlaceholder && enemySprite) {
            // Position and scale the sprite to match the enemy shape
            sf::FloatRect bounds = enemy.getGlobalBounds();
            
            // Make sprite larger than the shape - centered on the shape
            // Use the configurable sprite scale factor
            
            // Calculate scale to match the shape size and apply enlargement factor
            sf::Vector2u textureSize = enemySprite->getTexture().getSize();
            float scaleX = bounds.size.x / textureSize.x * spriteScale;
            float scaleY = bounds.size.y / textureSize.y * spriteScale;
            enemySprite->setScale(sf::Vector2f(scaleX, scaleY));
            
            // Center the sprite on the shape
            float offsetX = (bounds.size.x - (bounds.size.x * spriteScale)) / 2.0f;
            float offsetY = (bounds.size.y - (bounds.size.y * spriteScale)) / 2.0f;
            enemySprite->setPosition(sf::Vector2f(bounds.position.x + offsetX, bounds.position.y + offsetY));
            
            window.draw(*enemySprite);
            
            // Draw red boundary box around the enemy character if enabled
            if (showBoundingBoxes) {
                // Use the original enemy bounds for the boundary box, but move it downward
                sf::RectangleShape boundaryBox;
                
                // Make boundary box configurable height and position it at the bottom
                float boxHeight = bounds.size.y * boundaryBoxHeight;
                float boxY = bounds.position.y + bounds.size.y - boxHeight;
                
                boundaryBox.setSize(sf::Vector2f(bounds.size.x, boxHeight));
                boundaryBox.setPosition(sf::Vector2f(bounds.position.x, boxY));
                boundaryBox.setFillColor(sf::Color::Transparent);
                boundaryBox.setOutlineColor(enemyBorderColor);
                boundaryBox.setOutlineThickness(2.0f);
                window.draw(boundaryBox);
            }
        } else if (useEnemyPlaceholder) {
            // Position the placeholder rectangle to match the enemy
            sf::FloatRect bounds = enemy.getGlobalBounds();
            
            // Make boundary box configurable height and position it at the bottom
            float boxHeight = bounds.size.y * boundaryBoxHeight;
            float boxY = bounds.position.y + bounds.size.y - boxHeight;
            
            enemyPlaceholder.setSize(sf::Vector2f(bounds.size.x, boxHeight));
            enemyPlaceholder.setPosition(sf::Vector2f(bounds.position.x, boxY));
            enemyPlaceholder.setFillColor(sf::Color::Transparent);
            enemyPlaceholder.setOutlineColor(enemyBorderColor);
            enemyPlaceholder.setOutlineThickness(2.0f);
            window.draw(enemyPlaceholder);
        }
    }
    
    // Draw player - skip drawing the solid player shape
    // player.draw(window);
    
    // Then draw player sprite on top if available
    if (!usePlayerPlaceholder && playerSprite) {
        // Position and scale the sprite to match the player shape
        sf::FloatRect bounds = player.getGlobalBounds();
        
        // Make sprite larger than the shape - centered on the shape
        // Use the configurable sprite scale factor
        
        // Calculate scale to match the shape size and apply enlargement factor
        sf::Vector2u textureSize = playerSprite->getTexture().getSize();
        float scaleX = bounds.size.x / textureSize.x * spriteScale;
        float scaleY = bounds.size.y / textureSize.y * spriteScale;
        playerSprite->setScale(sf::Vector2f(scaleX, scaleY));
        
        // Center the sprite on the shape
        float offsetX = (bounds.size.x - (bounds.size.x * spriteScale)) / 2.0f;
        float offsetY = (bounds.size.y - (bounds.size.y * spriteScale)) / 2.0f;
        playerSprite->setPosition(sf::Vector2f(bounds.position.x + offsetX, bounds.position.y + offsetY));
        
        window.draw(*playerSprite);
        
        // Draw green boundary box around the player character if enabled
        if (showBoundingBoxes) {
            // Use the original player bounds for the boundary box, but move it downward
            sf::RectangleShape boundaryBox;
            
            // Make boundary box configurable height and position it at the bottom
            float boxHeight = bounds.size.y * boundaryBoxHeight;
            float boxY = bounds.position.y + bounds.size.y - boxHeight;
            
            boundaryBox.setSize(sf::Vector2f(bounds.size.x, boxHeight));
            boundaryBox.setPosition(sf::Vector2f(bounds.position.x, boxY));
            boundaryBox.setFillColor(sf::Color::Transparent);
            boundaryBox.setOutlineColor(playerBorderColor);
            boundaryBox.setOutlineThickness(2.0f);
            window.draw(boundaryBox);
        }
    } else if (usePlayerPlaceholder) {
        // Position the placeholder rectangle to match the player
        sf::FloatRect bounds = player.getGlobalBounds();
        
        // Make boundary box configurable height and position it at the bottom
        float boxHeight = bounds.size.y * boundaryBoxHeight;
        float boxY = bounds.position.y + bounds.size.y - boxHeight;
        
        playerPlaceholder.setSize(sf::Vector2f(bounds.size.x, boxHeight));
        playerPlaceholder.setPosition(sf::Vector2f(bounds.position.x, boxY));
        playerPlaceholder.setFillColor(sf::Color::Transparent);
        playerPlaceholder.setOutlineColor(playerBorderColor);
        playerPlaceholder.setOutlineThickness(2.0f);
        window.draw(playerPlaceholder);
    }
    
    // Draw lighting effects on top of everything if enabled
    if (showLighting && lightingSystem.isEnabled()) {
        lightingSystem.draw(window, gameView);
    }
    
    // Switch to UI view for UI elements
    window.setView(uiView);
    
    // Draw UI elements
    if (currentState == GameState::Playing) {
        // Draw health UI during gameplay
        window.draw(healthText);
        
        // Draw heart icons
        for (const auto& heart : heartIcons) {
            window.draw(heart);
        }
        
        // Draw level indicator at the top
        window.draw(levelText);
    } else if (currentState == GameState::GameOver) {
        // Draw game over text
        window.draw(gameOverText);
        window.draw(restartText);
    } else if (currentState == GameState::LevelTransition) {
        // Create semi-transparent dark overlay for level transition
        sf::RectangleShape overlay;
        overlay.setSize(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
        overlay.setFillColor(sf::Color(0, 0, 0, 180)); // Semi-transparent black
        window.draw(overlay);
        
        // Draw level transition text
        window.draw(levelText);
    } else if (currentState == GameState::DebugPanel) {
        // Draw the debug panel
        drawDebugPanel();
    }
    
    // Only draw minimap when showMiniMap is true
    if (showMiniMap) {
        // Switch to UI view for mini-map border
        window.setView(uiView);
        
        // Draw mini-map border first
        window.draw(miniMapBorder);
        
        // Create a new view specifically for mini-map content
        sf::View miniContentView;
        
        // Set the viewport to match exactly inside the border with small margin
        float borderX = WINDOW_WIDTH - MINI_MAP_WIDTH - MINI_MAP_MARGIN;
        float borderY = WINDOW_HEIGHT - MINI_MAP_HEIGHT - MINI_MAP_MARGIN;
        float borderWidth = MINI_MAP_WIDTH;
        float borderHeight = MINI_MAP_HEIGHT;
        
        // Add small margin to position inside the border
        float margin = 4.0f;
        float viewX = (borderX + margin) / WINDOW_WIDTH;
        float viewY = (borderY + margin) / WINDOW_HEIGHT;
        float viewWidth = (borderWidth - 2 * margin) / WINDOW_WIDTH;
        float viewHeight = (borderHeight - 2 * margin) / WINDOW_HEIGHT;
        
        // Set the viewport explicitly so that content appears inside the border
        miniContentView.setViewport(sf::FloatRect(sf::Vector2f(viewX, viewY), 
                                                sf::Vector2f(viewWidth, viewHeight)));
        
        // Set the view's size to the entire level for consistent scaling
        miniContentView.setSize(sf::Vector2f(LEVEL_WIDTH, WINDOW_HEIGHT));
        miniContentView.setCenter(sf::Vector2f(LEVEL_WIDTH / 2.f, WINDOW_HEIGHT / 2.f));
        
        // Apply the mini-map view
        window.setView(miniContentView);
        
        // Draw mini-map platforms
        for (const auto& platform : platforms) {
            // Draw a scaled-down version directly rather than using precomputed rectangles
            sf::RectangleShape miniPlatform = platform;
            float scaleX = viewWidth * WINDOW_WIDTH / LEVEL_WIDTH;
            float scaleY = viewHeight * WINDOW_HEIGHT / WINDOW_HEIGHT;
            miniPlatform.setFillColor(sf::Color::Green);
            window.draw(miniPlatform);
        }
        
        // Draw mini-map ladders
        for (const auto& ladder : ladders) {
            // Draw a scaled-down version directly
            sf::RectangleShape miniLadder = ladder;
            miniLadder.setFillColor(sf::Color(139, 69, 19)); // Brown
            window.draw(miniLadder);
        }
        
        // Draw mini-map enemies
        for (const auto& enemy : enemies) {
            // Draw scaled enemy positions
            sf::RectangleShape miniEnemy;
            miniEnemy.setSize(sf::Vector2f(10.f, 10.f));
            miniEnemy.setPosition(enemy.getGlobalBounds().position);
            miniEnemy.setFillColor(sf::Color::Red);
            window.draw(miniEnemy);
        }
        
        // Draw player icon
        sf::RectangleShape miniPlayer;
        miniPlayer.setSize(sf::Vector2f(10.f, 10.f));
        miniPlayer.setPosition(player.getPosition());
        miniPlayer.setFillColor(sf::Color::Yellow);
        window.draw(miniPlayer);
    }
    
    // Switch back to UI view for final display
    window.setView(uiView);
    
    // Draw FPS counter - ensure it's drawn last so it's on top of everything
    drawFPS();
    
    window.display();
}

void Game::run() {
    while (window.isOpen()) {
        handleEvents();
        update();
        draw();
    }
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

void Game::initializeDebugPanel() {
    // Set up the debug panel background
    debugPanelBackground.setSize(sf::Vector2f(WINDOW_WIDTH - 100, WINDOW_HEIGHT - 100));
    debugPanelBackground.setPosition(sf::Vector2f(50, 50));
    debugPanelBackground.setFillColor(sf::Color(30, 30, 30, 230)); // Semi-transparent dark background
    debugPanelBackground.setOutlineColor(sf::Color(100, 100, 100));
    debugPanelBackground.setOutlineThickness(2.0f);
    
    // Set up title text
    debugPanelTitle.setFont(font);
    debugPanelTitle.setString("Game Settings Panel");
    debugPanelTitle.setCharacterSize(24);
    debugPanelTitle.setFillColor(sf::Color::White);
    
    // Center the title at the top of the panel
    sf::FloatRect titleBounds = debugPanelTitle.getGlobalBounds();
    debugPanelTitle.setPosition(sf::Vector2f(
        WINDOW_WIDTH / 2 - titleBounds.size.x / 2,
        60
    ));
}

void Game::drawDebugPanel() {
    // Draw the panel background
    window.draw(debugPanelBackground);
    
    // Draw panel title
    window.draw(debugPanelTitle);
    
    // Draw tabs
    drawTabButton("Graphics", 0, &currentDebugTab, 100, 100);
    drawTabButton("Gameplay", 1, &currentDebugTab, 230, 100);
    drawTabButton("Physics", 2, &currentDebugTab, 360, 100);
    drawTabButton("Debug", 3, &currentDebugTab, 490, 100);
    drawTabButton("Assets", 4, &currentDebugTab, 620, 100);
    
    // Draw a separator line
    sf::RectangleShape separator;
    separator.setSize(sf::Vector2f(WINDOW_WIDTH - 150, 2));
    separator.setPosition(sf::Vector2f(75, 130));
    separator.setFillColor(sf::Color(100, 100, 100));
    window.draw(separator);
    
    // Content area starts at y=150
    if (currentDebugTab == 0) { // Graphics settings
        drawSettingsButton("Show Lighting", &showLighting, 100, 150);
        drawSettingsButton("Show Bounding Boxes", &showBoundingBoxes, 100, 190);
        drawSettingsButton("Show Mini-map", &showMiniMap, 100, 230);
        
        drawSlider("Sprite Scale", &spriteScale, 1.0f, 8.0f, 400, 150);
        drawSlider("Box Height", &boundaryBoxHeight, 0.1f, 1.0f, 400, 190);
        
        drawColorPicker("Platform Color", &platformColor, 100, 300);
        drawColorPicker("Player Border", &playerBorderColor, 100, 350);
        drawColorPicker("Enemy Border", &enemyBorderColor, 100, 400);
    }
    else if (currentDebugTab == 1) { // Gameplay settings
        drawSlider("Game Speed", &gameSpeed, 0.1f, 2.0f, 100, 150);
        drawSlider("Player Speed", &playerSpeed, 50.0f, 400.0f, 100, 190);
        
        // Level reset button
        sf::RectangleShape resetButton;
        resetButton.setSize(sf::Vector2f(150, 30));
        resetButton.setPosition(sf::Vector2f(100, 250));
        resetButton.setFillColor(sf::Color(200, 50, 50));
        resetButton.setOutlineColor(sf::Color::White);
        resetButton.setOutlineThickness(1);
        window.draw(resetButton);
        
        sf::Text resetText(font, sf::String("Reset Level"), 16);
        resetText.setFillColor(sf::Color::White);
        
        sf::FloatRect resetBounds = resetText.getGlobalBounds();
        resetText.setPosition(sf::Vector2f(
            100 + 75 - resetBounds.size.x / 2,
            250 + 15 - resetBounds.size.y / 2
        ));
        window.draw(resetText);
        
        // Check if reset button was clicked
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);
            if (mousePos.x >= 100 && mousePos.x <= 250 &&
                mousePos.y >= 250 && mousePos.y <= 280) {
                resetGame();
                currentState = GameState::Playing;
            }
        }
    }
    else if (currentDebugTab == 2) { // Physics settings
        // Add physics settings controls here
    }
    else if (currentDebugTab == 3) { // Debug settings
        // Add debug settings controls here
    }
    else if (currentDebugTab == 4) { // Asset settings
        // List assets and allow reloading
        sf::Text assetTitle(font, sf::String("Game Assets"), 18);
        assetTitle.setFillColor(sf::Color::White);
        assetTitle.setPosition(sf::Vector2f(100, 150));
        window.draw(assetTitle);
        
        // Background asset info
        sf::Text bgText(font, sf::String(std::string("Background: ") + 
            (useBackgroundPlaceholder ? "Using placeholder" : "Loaded")), 14);
        bgText.setFillColor(useBackgroundPlaceholder ? sf::Color::Red : sf::Color::Green);
        bgText.setPosition(sf::Vector2f(100, 180));
        window.draw(bgText);
        
        // Player asset info
        sf::Text playerText(font, sf::String(std::string("Player: ") + 
            (usePlayerPlaceholder ? "Using placeholder" : "Loaded")), 14);
        playerText.setFillColor(usePlayerPlaceholder ? sf::Color::Red : sf::Color::Green);
        playerText.setPosition(sf::Vector2f(100, 200));
        window.draw(playerText);
        
        // Enemy asset info
        sf::Text enemyText(font, sf::String(std::string("Enemy: ") + 
            (useEnemyPlaceholder ? "Using placeholder" : "Loaded")), 14);
        enemyText.setFillColor(useEnemyPlaceholder ? sf::Color::Red : sf::Color::Green);
        enemyText.setPosition(sf::Vector2f(100, 220));
        window.draw(enemyText);
        
        // Reload assets button
        sf::RectangleShape reloadButton;
        reloadButton.setSize(sf::Vector2f(150, 30));
        reloadButton.setPosition(sf::Vector2f(100, 260));
        reloadButton.setFillColor(sf::Color(50, 100, 200));
        reloadButton.setOutlineColor(sf::Color::White);
        reloadButton.setOutlineThickness(1);
        window.draw(reloadButton);
        
        sf::Text reloadText(font, sf::String("Reload Assets"), 16);
        reloadText.setFillColor(sf::Color::White);
        
        sf::FloatRect reloadBounds = reloadText.getGlobalBounds();
        reloadText.setPosition(sf::Vector2f(
            100 + 75 - reloadBounds.size.x / 2,
            260 + 15 - reloadBounds.size.y / 2
        ));
        window.draw(reloadText);
        
        // Check if reload button was clicked
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);
            if (mousePos.x >= 100 && mousePos.x <= 250 &&
                mousePos.y >= 260 && mousePos.y <= 290) {
                loadAssets();
            }
        }
    }
    
    // Instructions at the bottom
    sf::Text instructionsText(font, sf::String("Press ESC or F1 to return to game"), 14);
    instructionsText.setFillColor(sf::Color(200, 200, 200));
    
    sf::FloatRect instructBounds = instructionsText.getGlobalBounds();
    instructionsText.setPosition(sf::Vector2f(
        WINDOW_WIDTH / 2 - instructBounds.size.x / 2,
        WINDOW_HEIGHT - 80
    ));
    window.draw(instructionsText);
}

bool Game::drawSettingsButton(const std::string& label, bool* value, float x, float y, float width) {
    bool clicked = false;
    
    // Draw button background
    sf::RectangleShape button;
    button.setSize(sf::Vector2f(width, 30));
    button.setPosition(sf::Vector2f(x, y));
    button.setFillColor(*value ? sf::Color(50, 150, 50) : sf::Color(150, 50, 50));
    button.setOutlineColor(sf::Color::White);
    button.setOutlineThickness(1);
    window.draw(button);
    
    // Draw button text
    sf::Text buttonText(font, sf::String(label + ": " + (*value ? "ON" : "OFF")), 14);
    buttonText.setFillColor(sf::Color::White);
    
    // Center text on button
    sf::FloatRect textBounds = buttonText.getGlobalBounds();
    buttonText.setPosition(sf::Vector2f(
        x + width/2 - textBounds.size.x/2,
        y + 15 - textBounds.size.y/2
    ));
    window.draw(buttonText);
    
    // Check if button was clicked
    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        if (mousePos.x >= x && mousePos.x <= x + width &&
            mousePos.y >= y && mousePos.y <= y + 30) {
            *value = !(*value);
            clicked = true;
        }
    }
    
    return clicked;
}

bool Game::drawSlider(const std::string& label, float* value, float min, float max, float x, float y, float width) {
    bool changed = false;
    
    // Draw slider label
    sf::Text sliderLabel(font, sf::String(label + ": " + std::to_string(*value)), 14);
    sliderLabel.setFillColor(sf::Color::White);
    sliderLabel.setPosition(sf::Vector2f(x, y));
    window.draw(sliderLabel);
    
    // Draw slider track
    sf::RectangleShape track;
    track.setSize(sf::Vector2f(width, 5));
    track.setPosition(sf::Vector2f(x, y + 25));
    track.setFillColor(sf::Color(100, 100, 100));
    window.draw(track);
    
    // Draw slider handle
    float handlePos = x + ((*value - min) / (max - min)) * width;
    sf::CircleShape handle;
    handle.setRadius(8);
    handle.setPosition(sf::Vector2f(handlePos - 8, y + 25 - 5));
    handle.setFillColor(sf::Color(200, 200, 200));
    window.draw(handle);
    
    // Check if slider is being dragged
    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        if (mousePos.y >= y + 15 && mousePos.y <= y + 35 &&
            mousePos.x >= x && mousePos.x <= x + width) {
            // Calculate new value based on mouse position
            float newValue = min + ((mousePos.x - x) / width) * (max - min);
            // Clamp value to range
            newValue = std::max(min, std::min(max, newValue));
            
            if (*value != newValue) {
                *value = newValue;
                changed = true;
            }
        }
    }
    
    return changed;
}

void Game::drawColorPicker(const std::string& label, sf::Color* color, float x, float y) {
    // Draw color picker label
    sf::Text colorLabel(font, sf::String(label), 14);
    colorLabel.setFillColor(sf::Color::White);
    colorLabel.setPosition(sf::Vector2f(x, y));
    window.draw(colorLabel);
    
    // Draw current color preview
    sf::RectangleShape colorPreview;
    colorPreview.setSize(sf::Vector2f(30, 30));
    colorPreview.setPosition(sf::Vector2f(x + 160, y));
    colorPreview.setFillColor(*color);
    colorPreview.setOutlineColor(sf::Color::White);
    colorPreview.setOutlineThickness(1);
    window.draw(colorPreview);
    
    // Draw RGB sliders
    // Create local float variables for RGB values
    float r = static_cast<float>(color->r);
    float g = static_cast<float>(color->g);
    float b = static_cast<float>(color->b);
    
    bool rChanged = drawSlider("R", &r, 0, 255, x + 200, y, 150);
    bool gChanged = drawSlider("G", &g, 0, 255, x + 200, y + 30, 150);
    bool bChanged = drawSlider("B", &b, 0, 255, x + 200, y + 60, 150);
    
    // Update color if any slider changed
    if (rChanged || gChanged || bChanged) {
        color->r = static_cast<uint8_t>(r);
        color->g = static_cast<uint8_t>(g);
        color->b = static_cast<uint8_t>(b);
    }
}

void Game::drawTabButton(const std::string& label, int tabId, int* currentTabId, float x, float y, float width) {
    bool isActive = (tabId == *currentTabId);
    
    // Draw tab button
    sf::RectangleShape tabButton;
    tabButton.setSize(sf::Vector2f(width, 30));
    tabButton.setPosition(sf::Vector2f(x, y));
    tabButton.setFillColor(isActive ? sf::Color(80, 80, 100) : sf::Color(50, 50, 70));
    tabButton.setOutlineColor(sf::Color::White);
    tabButton.setOutlineThickness(isActive ? 2 : 1);
    window.draw(tabButton);
    
    // Draw tab text
    sf::Text tabText(font, sf::String(label), 16);
    tabText.setFillColor(sf::Color::White);
    
    // Center text on button
    sf::FloatRect textBounds = tabText.getGlobalBounds();
    tabText.setPosition(sf::Vector2f(
        x + width/2 - textBounds.size.x/2,
        y + 15 - textBounds.size.y/2
    ));
    window.draw(tabText);
    
    // Check if tab was clicked
    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        if (mousePos.x >= x && mousePos.x <= x + width &&
            mousePos.y >= y && mousePos.y <= y + 30) {
            *currentTabId = tabId;
        }
    }
} 