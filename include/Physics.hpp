#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include "Player.hpp"
#include "Enemy.hpp"

// Forward declarations
namespace NPCSystem {
    struct NPCData;
}

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
    void initializeNPCs(const std::vector<NPCSystem::NPCData>& npcs);
    
    // Update physics
    void update(float deltaTime, Player& player, std::vector<Enemy>& enemies);
    void updateNPCs(std::vector<NPCSystem::NPCData>& npcs);
    
    // Ground detection
    bool isEntityOnGround(const PhysicsComponent& entityPhysics, const sf::Vector2f& position, float checkDistance) const;
    
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
    
    void setPlayerCollisionOffset(float offsetX, float offsetY) { 
        playerOffsetX = offsetX; 
        playerOffsetY = offsetY; 
    }
    float getPlayerOffsetX() const { return playerOffsetX; }
    float getPlayerOffsetY() const { return playerOffsetY; }
    
    void setPlayerBounceFactor(float f) { playerBounceFactor = f; }
    float getPlayerBounceFactor() const { return playerBounceFactor; }
    
    void setEnemyCollisionSize(float width, float height) { 
        enemyCollisionWidth = width; 
        enemyCollisionHeight = height; 
    }
    float getEnemyCollisionWidth() const { return enemyCollisionWidth; }
    float getEnemyCollisionHeight() const { return enemyCollisionHeight; }
    
    void setEnemyCollisionOffset(float offsetX, float offsetY) { 
        enemyOffsetX = offsetX; 
        enemyOffsetY = offsetY; 
    }
    float getEnemyOffsetX() const { return enemyOffsetX; }
    float getEnemyOffsetY() const { return enemyOffsetY; }
    
    void setEnemyBounceFactor(float f) { enemyBounceFactor = f; }
    float getEnemyBounceFactor() const { return enemyBounceFactor; }
    
    void setPlatformFriction(float f) { platformFriction = f; }
    float getPlatformFriction() const { return platformFriction; }
    
    void setUseOneWayPlatforms(bool use) { useOneWayPlatforms = use; }
    bool getUseOneWayPlatforms() const { return useOneWayPlatforms; }

    void setPlayerAcceleration(float a) { playerAcceleration = a; }
    float getPlayerAcceleration() const { return playerAcceleration; }
    
    // Access to physics components for rendering/debugging
    const PhysicsComponent& getPlayerPhysicsComponent() const { return playerPhysics; }
    const PhysicsComponent& getEnemyPhysicsComponent(size_t index) const { 
        return (index < enemyPhysics.size()) ? enemyPhysics[index] : playerPhysics; // Fallback to player if out of bounds
    }
    size_t getEnemyPhysicsCount() const { return enemyPhysics.size(); }
    
    // Access to platform physics components for visualization
    const PhysicsComponent& getPlatformPhysicsComponent(size_t index) const {
        return (index < platformPhysics.size()) ? platformPhysics[index] : playerPhysics; // Fallback to player if out of bounds
    }
    size_t getPlatformPhysicsCount() const { return platformPhysics.size(); }
    
    // NPC collision settings
    void setNPCCollisionSize(float width, float height) { 
        npcCollisionWidth = width; 
        npcCollisionHeight = height; 
    }
    float getNPCCollisionWidth() const { return npcCollisionWidth; }
    float getNPCCollisionHeight() const { return npcCollisionHeight; }
    
    void setNPCCollisionOffset(float offsetX, float offsetY) { 
        npcOffsetX = offsetX; 
        npcOffsetY = offsetY; 
    }
    float getNPCOffsetX() const { return npcOffsetX; }
    float getNPCOffsetY() const { return npcOffsetY; }
    
    void setNPCBounceFactor(float f) { npcBounceFactor = f; }
    float getNPCBounceFactor() const { return npcBounceFactor; }
    
    // Access to NPC physics components
    const PhysicsComponent& getNPCPhysicsComponent(size_t index) const {
        return (index < npcPhysics.size()) ? npcPhysics[index] : playerPhysics;
    }
    size_t getNPCPhysicsCount() const { return npcPhysics.size(); }
    
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
    float playerOffsetX;
    float playerOffsetY;
    float playerBounceFactor;
    float enemyCollisionWidth;
    float enemyCollisionHeight;
    float enemyOffsetX;
    float enemyOffsetY;
    float enemyBounceFactor;
    float platformFriction;
    float playerAcceleration;
    bool useOneWayPlatforms;
    
    // NPC physics parameters
    float npcCollisionWidth;
    float npcCollisionHeight;
    float npcOffsetX;
    float npcOffsetY;
    float npcBounceFactor;
    
    // Physics components
    PhysicsComponent playerPhysics;
    std::vector<PhysicsComponent> enemyPhysics;
    std::vector<PhysicsComponent> platformPhysics;
    std::vector<PhysicsComponent> npcPhysics;
    
    // Window dimensions (needed for ground collision)
    int windowWidth;
    int windowHeight;
}; 