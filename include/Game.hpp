#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include "Player.hpp"
#include "Enemy.hpp"

// Game states enum
enum class GameState {
    Playing,
    GameOver
};

class Game {
public:
    Game();
    void run();

private:
    void handleEvents();
    void update();
    void draw();
    void initializePlatforms();
    void initializeLadders();
    void initializeEnemies();
    void initializeUI();
    void initializeMiniMap();
    void updateMiniMap();
    void checkPlayerEnemyCollision();
    void updateUI();
    void checkGameOver();
    void resetGame();
    
    // Helper to create a heart shape
    sf::RectangleShape createHeartIcon(float x, float y, bool filled);

    sf::RenderWindow window;
    sf::View gameView;
    sf::View uiView; // Separate view for UI elements that don't scroll
    sf::View miniMapView; // View for the mini-map
    sf::Clock clock;
    Player player;
    std::vector<sf::RectangleShape> platforms;
    std::vector<sf::RectangleShape> ladders;
    std::vector<Enemy> enemies;
    bool playerHit;
    float playerHitCooldown;
    
    // Mini-map elements
    sf::RectangleShape miniMapBorder;
    sf::RectangleShape miniMapPlayerIcon;
    std::vector<sf::RectangleShape> miniMapPlatforms;
    std::vector<sf::RectangleShape> miniMapLadders;
    std::vector<sf::RectangleShape> miniMapEnemies;
    
    // Game state
    GameState currentState;
    
    // UI elements
    sf::Font defaultFont; // Default font for initialization
    sf::Font font;        // Game font that will be loaded
    sf::Text healthText;
    sf::Text gameOverText;
    sf::Text restartText;
    std::vector<sf::RectangleShape> heartIcons; // Heart icons for health display
    
    static constexpr int WINDOW_WIDTH = 800;
    static constexpr int WINDOW_HEIGHT = 600;
    static constexpr int LEVEL_WIDTH = 3000; // Extended level width for scrolling
    static constexpr int FPS = 60;
    static constexpr float HIT_COOLDOWN = 1.5f; // 1.5 seconds invulnerability
    
    // Mini-map constants
    static constexpr int MINI_MAP_WIDTH = 200;
    static constexpr int MINI_MAP_HEIGHT = 100;
    static constexpr int MINI_MAP_MARGIN = 10;
}; 