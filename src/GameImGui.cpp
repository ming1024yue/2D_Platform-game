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
            logError("Failed to initialize ImGui::SFML");
            useImGuiInterface = false;
            return;
        }
        
        logInfo("ImGui::SFML initialized successfully");
        
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
        logError("Exception in initializeImGui: " + std::string(e.what()));
        useImGuiInterface = false;
    } catch (...) {
        logError("Unknown exception in initializeImGui");
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
        logError("Exception in renderImGui: " + std::string(e.what()));
    } catch (...) {
        logError("Unknown exception in renderImGui");
    }
}

// Shutdown ImGui when the game is closed
void Game::shutdownImGui() {
    try {
        logInfo("Shutting down ImGui::SFML...");
        ImGui::SFML::Shutdown();
        logInfo("ImGui::SFML shutdown completed");
    } catch (const std::exception& e) {
        logError("Exception in shutdownImGui: " + std::string(e.what()));
    } catch (...) {
        logError("Unknown exception in shutdownImGui");
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
            
            // Toggle player debug info with F3 key
            if (key->code == sf::Keyboard::Key::F3) {
                showPlayerDebug = !showPlayerDebug;
                player.toggleDebugInfo();
                logDebug("Player debug info " + std::string(showPlayerDebug ? "enabled" : "disabled"));
            }
            
            // Toggle debug grid with G key
            if (key->code == sf::Keyboard::Key::G) {
                showDebugGrid = !showDebugGrid;
                renderingSystem.setShowDebugGrid(showDebugGrid);
                logDebug("Debug grid " + std::string(showDebugGrid ? "enabled" : "disabled"));
            }
            
            // Toggle ImGui interface with F1 key - with safety checks
            if (key->code == sf::Keyboard::Key::F1) {
                // Log debug info
                logDebug("F1 pressed: Toggling ImGui from " + std::string(useImGuiInterface ? "ON" : "OFF") + 
                         " to " + std::string(!useImGuiInterface ? "ON" : "OFF"));
                
                // Set the interface state
                useImGuiInterface = !useImGuiInterface;
            }
            
            // Toggle fullscreen with F4 key
            if (key->code == sf::Keyboard::Key::F4) {
                isFullscreen = !isFullscreen;
                
                if (isFullscreen) {
                    // Store current window properties
                    previousVideoMode = sf::VideoMode(sf::Vector2u(window.getSize().x, window.getSize().y));
                    previousPosition = window.getPosition();
                    
                    // Switch to fullscreen mode using the first available fullscreen mode
                    auto fullscreenModes = sf::VideoMode::getFullscreenModes();
                    if (!fullscreenModes.empty()) {
                        window.create(fullscreenModes[0], "2D Platform Puzzle Game", 
                                    sf::Style::None); // Borderless fullscreen
                        window.setPosition(sf::Vector2i(0, 0)); // Position at top-left
                    } else {
                        logError("No fullscreen modes available");
                        isFullscreen = false;
                    }
                } else {
                    // Restore windowed mode with previous properties
                    window.create(previousVideoMode, "2D Platform Puzzle Game", 
                                sf::Style::Close | sf::Style::Titlebar | sf::Style::Resize);
                    window.setPosition(previousPosition);
                }
                
                // Reinitialize ImGui after recreating the window
                if (!ImGui::SFML::Init(window)) {
                    logError("Failed to reinitialize ImGui after toggling fullscreen");
                    useImGuiInterface = false;
                }
                
                // Reset window framerate limit
                window.setFramerateLimit(FPS);
                
                logDebug("Toggled fullscreen mode: " + std::string(isFullscreen ? "ON" : "OFF"));
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
    
    // Draw background layers
    if (useBackgroundPlaceholder) {
        // Draw the green rectangle placeholder
        window.draw(backgroundPlaceholder);
    } else {
        // Draw all background layers with parallax effect using rendering system
        renderingSystem.setRenderTarget(&window);
        renderingSystem.renderBackgroundLayers();
    }
    
    // Draw debug grid for canonical coordinates
    renderingSystem.setRenderTarget(&window);
    renderingSystem.renderDebugGrid();
    
    // Draw platforms using rendering system
    if (renderingSystem.isLoaded()) {
        // Use rendering system for textured platforms
        renderingSystem.renderPlatforms(window, platforms, true);
    } else {
        // Fallback to original platform rendering if tiles not loaded
        for (const auto& platform : platforms) {
            // If we have background layers loaded, make platforms semi-transparent
            // so the ground layer texture shows through
            if (!useBackgroundPlaceholder) {
                // Create a copy of the platform with reduced opacity
                sf::RectangleShape transparentPlatform = platform;
                sf::Color platformColor = transparentPlatform.getFillColor();
                platformColor.a = 100; // Make it semi-transparent (was 255, now 100)
                transparentPlatform.setFillColor(platformColor);
                window.draw(transparentPlatform);
            } else {
                // Use normal opaque platforms when using placeholder background
                window.draw(platform);
            }
        }
    }
    
    // Draw ladders
    for (const auto& ladder : ladders) {
        window.draw(ladder);
    }
    
    // Draw collision boxes for debugging
    drawDebugBoxes();
    
    // Draw enemies
    if (showEnemies) {
        for (const auto& enemy : enemies) {
            enemy.draw(window);
        }
    }
    
    // Draw NPCs
    if (npcManager) {
        npcManager->renderAll();
    }
    
    // Draw player
    player.draw(window);
    
    // Draw player debug info if enabled
    if (showPlayerDebug) {
        player.drawDebugInfo(window);
    }
    
    // Create semi-transparent overlay for game over state
    if (currentState == GameState::GameOver) {
        // Create a semi-transparent dark overlay
        sf::RectangleShape overlay;
        overlay.setSize(sf::Vector2f(WINDOW_WIDTH * 2, WINDOW_HEIGHT * 2)); // Make it larger to cover everything
        overlay.setPosition(gameView.getCenter() - sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT)); // Center on view
        overlay.setFillColor(sf::Color(0, 0, 0, 180)); // Semi-transparent black
        window.draw(overlay);
    }
    
    // Draw UI elements only if not using ImGui or it's a state-specific UI
    if (!useImGuiInterface || currentState != GameState::Playing) {
        if (currentState == GameState::Playing) {
            // Draw level indicator at the top
            window.draw(levelText);
        } else if (currentState == GameState::GameOver) {
            // Draw game over text and restart text (now positioned in update())
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
        if (showEnemies) {
            for (const auto& enemy : enemies) {
                // Draw scaled enemy positions
                sf::RectangleShape miniEnemy;
                miniEnemy.setSize(sf::Vector2f(10.f, 10.f));
                miniEnemy.setPosition(enemy.getGlobalBounds().position);
                miniEnemy.setFillColor(sf::Color::Red);
                window.draw(miniEnemy);
            }
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
    // Log shutdown and close log file
    if (gameLogFile.is_open()) {
        logInfo("Game shutting down - session ended");
        gameLogFile.close();
    }
    
    // Shutdown ImGui when the game is destroyed
    shutdownImGui();
} 