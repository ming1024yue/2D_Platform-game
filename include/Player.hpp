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
    sf::Vector2f getPosition() const { return position; }
    sf::Vector2f getSize() const { return collisionBox.getSize(); }
    sf::FloatRect getGlobalBounds() const { return collisionBox.getGlobalBounds(); }
    
    // Direction facing check
    bool isFacingLeft() const { return facingLeft; }
    
    // Setter for player position (used when pushed by enemies)
    void setPosition(const sf::Vector2f& pos);
    
    // Collision box settings
    void setCollisionBoxSize(const sf::Vector2f& size);
    void setCollisionBoxOffset(const sf::Vector2f& offset);
    
    // Physics methods
    sf::Vector2f getVelocity() const { return velocity; }
    void setVelocity(const sf::Vector2f& vel) { velocity = vel; }
    bool isOnGround() const { return onGround; }
    void setOnGround(bool grounded) { onGround = grounded; }
    bool isOnLadder() const { return onLadder; }
    
    // Health methods
    int getHealth() const { return health; }
    void decreaseHealth() { if (health > 0) health--; }
    void resetHealth() { health = maxHealth; }

private:
    sf::Vector2f position;           // Player's position in the world
    sf::RectangleShape collisionBox; // Collision box for physics
    sf::Vector2f collisionOffset;    // Offset of collision box from position
    sf::Vector2f velocity;
    bool isJumping;
    bool onGround;
    bool onLadder;
    bool facingLeft;                 // Track which direction player is facing
    
    // Health properties
    int health;
    int maxHealth;
    
    static constexpr float PLAYER_SPEED = 5.0f;
    static constexpr float CLIMB_SPEED = 3.0f;
    static constexpr float JUMP_FORCE = -15.0f;
    static constexpr float GRAVITY = 0.6f;
    
    // Helper method for platform checks
    bool checkPlatformAbove(const std::vector<sf::RectangleShape>& platforms);
}; 