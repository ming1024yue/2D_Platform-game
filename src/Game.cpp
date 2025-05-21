#include "Game.hpp"
#include <iostream>
#include <cstdint> // For uint8_t

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
               playerLightIndex(0),
               showMiniMap(true),
               showLighting(true),
               currentLevel(1),
               transitionTimer(0.f),
               levelText(defaultFont, sf::String("Level 1"), 36) {
    window.setFramerateLimit(FPS);
    
    // Initialize lighting system
    sf::Color ambientColor(30, 30, 40, 200); // Dark blue-ish ambient light
    lightingSystem.initialize(WINDOW_WIDTH, WINDOW_HEIGHT, ambientColor);
    
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
    
    initializePlatforms();
    initializeLadders();
    initializeEnemies();
    initializeUI();
    initializeMiniMap();
    initializeLights();
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
    }
    
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
                window.close();
            }
            
            // Toggle minimap with M key
            if (key->code == sf::Keyboard::Key::M) {
                showMiniMap = !showMiniMap;
            }
            
            // Toggle lighting with L key
            if (key->code == sf::Keyboard::Key::L) {
                showLighting = !showLighting;
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

void Game::draw() {
    window.clear(sf::Color::Black); // Black background
    
    // Use game view for game elements
    window.setView(gameView);
    
    // Draw platforms
    for (const auto& platform : platforms) {
        window.draw(platform);
    }
    
    // Draw ladders
    for (const auto& ladder : ladders) {
        window.draw(ladder);
    }
    
    // Draw enemies
    for (const auto& enemy : enemies) {
        enemy.draw(window);
    }
    
    // Draw player (unless briefly invisible due to hit invulnerability blinking)
    if (currentState == GameState::Playing) {
        if (!playerHit || static_cast<int>(playerHitCooldown * 10) % 2 == 0) {
            player.draw(window);
        }
    } else {
        // Still draw player when game over, but don't blink
        player.draw(window);
    }
    
    // Draw lighting effect only if enabled
    if (showLighting) {
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