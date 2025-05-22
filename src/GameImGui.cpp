#include "Game.hpp"
#include <sstream>
#include <iomanip>
#include <cstdint> // For uint8_t
#include <iostream> // For std::cout, std::cerr, std::endl

// Initialize ImGui with SFML integration
void Game::initializeImGui() {
    try {
        // Initialize ImGui context and link it with SFML
        bool initSuccess = ImGui::SFML::Init(window);
        
        if (!initSuccess) {
            std::cerr << "Failed to initialize ImGui::SFML" << std::endl;
            useImGuiInterface = false;
            return;
        }
        
        std::cout << "ImGui::SFML initialized successfully" << std::endl;
        
        // Set ImGui style
        ImGui::StyleColorsDark();
        
        // Adjust style to match game aesthetic
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 6.0f;
        style.FrameRounding = 4.0f;
        style.ItemSpacing = ImVec2(8, 6);
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.15f, 0.9f);
        style.Colors[ImGuiCol_Header] = ImVec4(0.2f, 0.4f, 0.2f, 0.8f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.3f, 0.6f, 0.3f, 0.8f);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.2f, 0.7f, 0.2f, 1.0f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.2f, 0.4f, 0.2f, 0.8f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.3f, 0.6f, 0.3f, 0.8f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.2f, 0.7f, 0.2f, 1.0f);
    } catch (const std::exception& e) {
        std::cerr << "Exception in initializeImGui: " << e.what() << std::endl;
        useImGuiInterface = false;
    } catch (...) {
        std::cerr << "Unknown exception in initializeImGui" << std::endl;
        useImGuiInterface = false;
    }
}

// Note: UpdateImGui method is implemented in Game.cpp

// Render ImGui interface
void Game::renderImGui() {
    try {
        // Always render ImGui to avoid assertion failures
        // Just don't display windows when interface is disabled
        ImGui::SFML::Render(window);
    } catch (const std::exception& e) {
        std::cerr << "Exception in renderImGui: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception in renderImGui" << std::endl;
    }
}

// Shutdown ImGui when the game is closed
void Game::shutdownImGui() {
    try {
        std::cout << "Shutting down ImGui::SFML..." << std::endl;
        ImGui::SFML::Shutdown();
        std::cout << "ImGui::SFML shutdown completed" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception in shutdownImGui: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception in shutdownImGui" << std::endl;
    }
}

// Modified version of handleEvents to process ImGui events
void Game::handleEvents() {
    while (auto event = window.pollEvent()) {
        // Pass event to ImGui first
        ImGui::SFML::ProcessEvent(window, *event);

        if (event->is<sf::Event::Closed>()) {
            window.close();
        }
        if (auto key = event->getIf<sf::Event::KeyPressed>()) {
            if (key->code == sf::Keyboard::Key::Escape) {
                // Toggle ImGui interface if it's enabled
                if (useImGuiInterface) {
                    useImGuiInterface = false;
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
            
            // Toggle ImGui interface with F1 key - with safety checks
            if (key->code == sf::Keyboard::Key::F1) {
                // Print debug info
                std::cout << "F1 pressed: Toggling ImGui from " << (useImGuiInterface ? "ON" : "OFF") 
                          << " to " << (!useImGuiInterface ? "ON" : "OFF") << std::endl;
                
                // Set the interface state
                useImGuiInterface = !useImGuiInterface;
            }
            
            // Handle restart when in game over state
            if (currentState == GameState::GameOver && key->code == sf::Keyboard::Key::Enter) {
                resetGame();
            }
        }
    }
}

// Note: Update method is implemented in Game.cpp

// Modified version of draw to include ImGui
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
    
    // Draw enemies
    for (size_t enemyIdx = 0; enemyIdx < enemies.size(); enemyIdx++) {
        const auto& enemy = enemies[enemyIdx];
        // Then draw enemy sprite on top if available
        if (!useEnemyPlaceholder && enemySprite) {
            // Position and scale the sprite to match the enemy shape
            sf::FloatRect bounds = enemy.getGlobalBounds();
            
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
            
            // Draw physics collision box only if enabled
            if (showBoundingBoxes) {
                // Draw physics collision box - use actual physics component
                if (enemyIdx < physicsSystem.getEnemyPhysicsCount()) {
                    sf::FloatRect physicsBox = physicsSystem.getEnemyPhysicsComponent(enemyIdx).collisionBox;
                    sf::RectangleShape collisionBox;
                    collisionBox.setSize(sf::Vector2f(physicsBox.size.x, physicsBox.size.y));
                    collisionBox.setPosition(sf::Vector2f(physicsBox.position.x, physicsBox.position.y));
                    collisionBox.setFillColor(sf::Color(255, 0, 0, 50)); // Semi-transparent red
                    collisionBox.setOutlineColor(sf::Color(255, 0, 0));
                    collisionBox.setOutlineThickness(1.0f);
                    window.draw(collisionBox);
                }
            }
        } else if (useEnemyPlaceholder) {
            // Position the placeholder rectangle to match the enemy
            sf::FloatRect bounds = enemy.getGlobalBounds();
            
            // Just draw a simple placeholder, no boundary box
            enemyPlaceholder.setSize(sf::Vector2f(bounds.size.x, bounds.size.y));
            enemyPlaceholder.setPosition(sf::Vector2f(bounds.position.x, bounds.position.y));
            enemyPlaceholder.setFillColor(sf::Color::Red);
            enemyPlaceholder.setOutlineColor(sf::Color::Black);
            enemyPlaceholder.setOutlineThickness(1.0f);
            window.draw(enemyPlaceholder);
        }
    }
    
    // Draw player
    if (!usePlayerPlaceholder && playerSprite) {
        // Position and scale the sprite to match the player shape
        sf::FloatRect bounds = player.getGlobalBounds();
        
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
        
        // Draw physics collision box only if enabled
        if (showBoundingBoxes) {
            // Draw physics collision box - use actual physics component
            sf::FloatRect physicsBox = physicsSystem.getPlayerPhysicsComponent().collisionBox;
            sf::RectangleShape collisionBox;
            collisionBox.setSize(sf::Vector2f(physicsBox.size.x, physicsBox.size.y));
            collisionBox.setPosition(sf::Vector2f(physicsBox.position.x, physicsBox.position.y));
            
            // Change color based on player state (green normally, blue when on ladder)
            if (player.isOnLadder()) {
                // Blue when on ladder to indicate ladder physics is active
                collisionBox.setFillColor(sf::Color(0, 100, 255, 50)); // Semi-transparent blue
                collisionBox.setOutlineColor(sf::Color(0, 100, 255));
            } else {
                // Regular green physics box
                collisionBox.setFillColor(sf::Color(0, 255, 0, 50)); // Semi-transparent green
                collisionBox.setOutlineColor(sf::Color(0, 255, 0));
            }
            
            collisionBox.setOutlineThickness(1.0f);
            window.draw(collisionBox);
        }
    } else if (usePlayerPlaceholder) {
        // Position the placeholder rectangle to match the player
        sf::FloatRect bounds = player.getGlobalBounds();
        
        // Just draw a simple placeholder, no boundary box
        playerPlaceholder.setSize(sf::Vector2f(bounds.size.x, bounds.size.y));
        playerPlaceholder.setPosition(sf::Vector2f(bounds.position.x, bounds.position.y));
        playerPlaceholder.setFillColor(sf::Color::Blue);
        playerPlaceholder.setOutlineColor(sf::Color::Black);
        playerPlaceholder.setOutlineThickness(1.0f);
        window.draw(playerPlaceholder);
    }
    
    // Draw lighting effects on top of everything if enabled
    if (showLighting && lightingSystem.isEnabled()) {
        lightingSystem.draw(window, gameView);
    }
    
    // Switch to UI view for UI elements
    window.setView(uiView);
    
    // Draw UI elements only if not using ImGui or it's a state-specific UI
    if (!useImGuiInterface || currentState != GameState::Playing) {
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
            // Draw a scaled-down version directly
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
    
    // Draw FPS counter only if not using ImGui (ImGui shows FPS already)
    if (!useImGuiInterface) {
        drawFPS();
    }
    
    // Render ImGui interface
    renderImGui();
    
    window.display();
}

// Game destructor implementation
Game::~Game() {
    // Shutdown ImGui when the game is destroyed
    shutdownImGui();
} 