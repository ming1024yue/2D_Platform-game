#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include "Player.hpp"
#include "Enemy.hpp"

// Physics component to store collision properties
struct PhysicsComponent {
    sf::FloatRect collisionBox;
    sf::Vector2f velocity;
    bool hasGravity;
    bool isStatic;
    float bounceFactor;
    float friction;
    
    PhysicsComponent() : velocity(0.0f, 0.0f), hasGravity(true), isStatic(false), 
                         bounceFactor(0.0f), friction(0.0f) {}
};

class PhysicsSystem {
public:
    PhysicsSystem();
    ~PhysicsSystem();
    
    // Initialization
    void initialize();
    void initializePlayer(Player& player);
    void initializePlatforms(const std::vector<sf::RectangleShape>& platforms);
    void initializeEnemies(const std::vector<Enemy>& enemies);
    
    // Update physics
    void update(float deltaTime, Player& player, std::vector<Enemy>& enemies);
    
    // Getters and setters for physics properties
    void setGravity(float g) { gravity = g; }
    float getGravity() const { return gravity; }
    
    void setTerminalVelocity(float v) { terminalVelocity = v; }
    float getTerminalVelocity() const { return terminalVelocity; }
    
    void setJumpForce(float f) { jumpForce = f; }
    float getJumpForce() const { return jumpForce; }
    
    void setPlayerCollisionSize(float width, float height) { 
        playerCollisionWidth = width; 
        playerCollisionHeight = height; 
    }
    float getPlayerCollisionWidth() const { return playerCollisionWidth; }
    float getPlayerCollisionHeight() const { return playerCollisionHeight; }
    
    void setPlayerBounceFactor(float f) { playerBounceFactor = f; }
    float getPlayerBounceFactor() const { return playerBounceFactor; }
    
    void setEnemyCollisionSize(float width, float height) { 
        enemyCollisionWidth = width; 
        enemyCollisionHeight = height; 
    }
    float getEnemyCollisionWidth() const { return enemyCollisionWidth; }
    float getEnemyCollisionHeight() const { return enemyCollisionHeight; }
    
    void setEnemyBounceFactor(float f) { enemyBounceFactor = f; }
    float getEnemyBounceFactor() const { return enemyBounceFactor; }
    
    void setPlatformFriction(float f) { platformFriction = f; }
    float getPlatformFriction() const { return platformFriction; }
    
    void setUseOneWayPlatforms(bool use) { useOneWayPlatforms = use; }
    bool getUseOneWayPlatforms() const { return useOneWayPlatforms; }
    
    // Access to physics components for rendering/debugging
    const PhysicsComponent& getPlayerPhysicsComponent() const { return playerPhysics; }
    const PhysicsComponent& getEnemyPhysicsComponent(size_t index) const { 
        return (index < enemyPhysics.size()) ? enemyPhysics[index] : playerPhysics; // Fallback to player if out of bounds
    }
    size_t getEnemyPhysicsCount() const { return enemyPhysics.size(); }
    
private:
    // Helper methods
    void resolveCollisions(Player& player, std::vector<Enemy>& enemies);
    bool checkCollision(const PhysicsComponent& a, const PhysicsComponent& b);
    void applyPhysicsToEntities(Player& player, std::vector<Enemy>& enemies);
    
    // Physics parameters
    float gravity;
    float terminalVelocity;
    float jumpForce;
    float playerCollisionWidth;
    float playerCollisionHeight;
    float playerBounceFactor;
    float enemyCollisionWidth;
    float enemyCollisionHeight;
    float enemyBounceFactor;
    float platformFriction;
    bool useOneWayPlatforms;
    
    // Physics components
    PhysicsComponent playerPhysics;
    std::vector<PhysicsComponent> enemyPhysics;
    std::vector<PhysicsComponent> platformPhysics;
    
    // Window dimensions (needed for ground collision)
    int windowWidth;
    int windowHeight;
}; 