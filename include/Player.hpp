#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

class Player {
public:
    Player(float x, float y);
    void update(const std::vector<sf::RectangleShape>& platforms, const std::vector<sf::RectangleShape>& ladders);
    void draw(sf::RenderWindow& window);
    void handleInput();
    
    // Getter for player position
    sf::Vector2f getPosition() const { return shape.getPosition(); }
    sf::Vector2f getSize() const { return shape.getSize(); }
    sf::FloatRect getGlobalBounds() const { return shape.getGlobalBounds(); }
    
    // Setter for player position (used when pushed by enemies)
    void setPosition(const sf::Vector2f& pos) { shape.setPosition(pos); }
    
    // Health methods
    int getHealth() const { return health; }
    void decreaseHealth() { if (health > 0) health--; }
    void resetHealth() { health = maxHealth; }

private:
    sf::RectangleShape shape;
    sf::Vector2f velocity;
    bool isJumping;
    bool onGround;
    bool onLadder;
    
    // Health properties
    int health;
    int maxHealth;
    
    static constexpr float PLAYER_SPEED = 5.0f;
    static constexpr float CLIMB_SPEED = 3.0f;
    static constexpr float JUMP_FORCE = -15.0f;
    static constexpr float GRAVITY = 0.8f;
    
    // Helper method for platform checks
    bool checkPlatformAbove(const std::vector<sf::RectangleShape>& platforms);
}; 