#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include "Animation.hpp"

// Forward declaration to avoid circular includes
class PhysicsSystem;

class Player {
public:
    Player(float x, float y, PhysicsSystem& physics);
    void update(float deltaTime, const std::vector<sf::RectangleShape>& platforms, const std::vector<sf::RectangleShape>& ladders);
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
    bool isJumping() const { return mIsJumping; }
    void setJumping(bool jumping) { mIsJumping = jumping; }
    
    // Reset player to initial state
    void reset(float x, float y);
    
    // Animation methods
    void initializeAnimations();
    void updateAnimation(float deltaTime);
    const sf::Sprite& getAnimatedSprite() const;
    bool hasAnimations() const;

    // Debug methods
    void drawDebugInfo(sf::RenderWindow& window);
    void toggleDebugInfo() { showDebugInfo = !showDebugInfo; }
    bool isDebugInfoEnabled() const { return showDebugInfo; }

private:
    sf::Vector2f position;           // Player's position in the world
    sf::RectangleShape collisionBox; // Collision box for physics
    sf::Vector2f collisionOffset;    // Offset of collision box from position
    sf::Vector2f velocity;
    bool mIsJumping;                 // Renamed from isJumping to avoid conflict
    bool onGround;
    bool onLadder;
    bool facingLeft;                 // Track which direction player is facing
    
    // Debug properties
    bool showDebugInfo = false;
    sf::Clock stateChangeTimer;
    struct StateDebugInfo {
        float timeInCurrentState = 0.0f;
        int stateChanges = 0;
        bool prevOnGround = false;
        bool prevIsJumping = false;
        float lastGroundY = 0.0f;
    } debugInfo;
    
    // Reference to physics system
    PhysicsSystem& physicsSystem;
    
    // Animation system
    Animation playerAnimation;
    bool animationsLoaded;
    
    static constexpr float PLAYER_SPEED = 300.0f;
    static constexpr float CLIMB_SPEED = 3.0f;
    static constexpr float JUMP_FORCE = -15.0f;
    static constexpr float GRAVITY = 0.6f;
    
    // Helper method for platform checks
    bool checkPlatformAbove(const std::vector<sf::RectangleShape>& platforms);
}; 