#include "Physics.hpp"
#include <iostream>
#include <SFML/Graphics.hpp>
#include "NPC.hpp"

// Helper function for rectangle intersection (SFML 3.x compatibility)
static bool rectsIntersect(const sf::FloatRect& a, const sf::FloatRect& b) {
    return a.position.x < b.position.x + b.size.x &&
           a.position.x + a.size.x > b.position.x &&
           a.position.y < b.position.y + b.size.y &&
           a.position.y + a.size.y > b.position.y;
}

PhysicsSystem::PhysicsSystem() : 
    gravity(10.0f),
    terminalVelocity(600.0f),
    jumpForce(15.0f * 60.0f),
    playerCollisionWidth(1.0f),
    playerCollisionHeight(1.0f),
    playerOffsetX(0.0f),
    playerOffsetY(0.0f),
    playerBounceFactor(0.0f),
    enemyCollisionWidth(1.0f),
    enemyCollisionHeight(1.0f),
    enemyOffsetX(0.0f),
    enemyOffsetY(0.0f),
    enemyBounceFactor(0.1f),
    npcCollisionWidth(1.0f),
    npcCollisionHeight(1.0f),
    npcOffsetX(0.0f),
    npcOffsetY(0.0f),
    npcBounceFactor(0.0f),
    platformFriction(0.3f),
    useOneWayPlatforms(false),
    windowWidth(800),
    windowHeight(600) {
}

PhysicsSystem::~PhysicsSystem() {
    // Nothing to clean up
}

void PhysicsSystem::initialize() {
    // Clear all physics components
    platformPhysics.clear();
    enemyPhysics.clear();
    npcPhysics.clear();
    
    // Initialize player physics
    playerPhysics.hasGravity = true;
    playerPhysics.isStatic = false;
    playerPhysics.bounceFactor = playerBounceFactor;
    playerPhysics.friction = 0.0f; // Player has no friction
}

void PhysicsSystem::initializePlayer(Player& player) {
    // Initialize player physics component
    sf::FloatRect playerBounds = player.getGlobalBounds();
    float width = playerBounds.size.x * playerCollisionWidth;
    float height = playerBounds.size.y * playerCollisionHeight;
    
    // Center the collision box on the sprite
    float offsetX = (playerBounds.size.x - width) / 2.0f;
    float offsetY = (playerBounds.size.y - height) / 2.0f;
    
    playerPhysics.collisionBox = sf::FloatRect(
        sf::Vector2f(playerBounds.position.x + offsetX, playerBounds.position.y + offsetY),
        sf::Vector2f(width, height)
    );
    
    // Initialize with zero velocity since we'll position player properly
    playerPhysics.velocity = sf::Vector2f(0.0f, 0.0f);
    player.setVelocity(playerPhysics.velocity);
    
    // Ensure player is positioned correctly relative to ground
    float groundLevel = windowHeight - 100.0f; // Match Game::GROUND_HEIGHT
    float desiredBottom = groundLevel - 1.0f; // Position slightly above ground
    float newY = desiredBottom - height; // Use collision height for accurate positioning
    player.setPosition(sf::Vector2f(player.getPosition().x, newY));
}

void PhysicsSystem::initializePlatforms(const std::vector<sf::RectangleShape>& platforms) {
    platformPhysics.clear();
    
    for (const auto& platform : platforms) {
        PhysicsComponent pc;
        pc.collisionBox = platform.getGlobalBounds();
        pc.hasGravity = false;
        pc.isStatic = true;
        pc.friction = platformFriction;
        platformPhysics.push_back(pc);
    }
}

void PhysicsSystem::initializeEnemies(const std::vector<Enemy>& enemies) {
    enemyPhysics.clear();
    
    for (const auto& enemy : enemies) {
        PhysicsComponent pc;
        sf::FloatRect bounds = enemy.getGlobalBounds();
        // Adjust collision box based on settings
        float width = bounds.size.x * enemyCollisionWidth;
        float height = bounds.size.y * enemyCollisionHeight;
        
        // Apply custom offsets instead of automatic centering
        float offsetX = bounds.size.x * enemyOffsetX; 
        float offsetY = bounds.size.y * enemyOffsetY;
        
        pc.collisionBox = sf::FloatRect(
            sf::Vector2f(bounds.position.x + offsetX, bounds.position.y + offsetY),
            sf::Vector2f(width, height)
        );
        pc.hasGravity = true;
        pc.isStatic = false;
        pc.bounceFactor = enemyBounceFactor;
        pc.friction = 0.0f;
        
        enemyPhysics.push_back(pc);
    }
}

void PhysicsSystem::initializeNPCs(const std::vector<NPCSystem::NPCData>& npcs) {
    npcPhysics.clear();
    
    for (const auto& npc : npcs) {
        PhysicsComponent pc;
        
        // Get sprite bounds if available
        sf::FloatRect bounds;
        if (npc.sprite) {
            bounds = npc.sprite->getGlobalBounds();
        } else {
            // Default size if no sprite
            bounds = sf::FloatRect(
                sf::Vector2f(npc.x, npc.y),
                sf::Vector2f(32.f, 32.f)
            );
        }
        
        // Adjust collision box based on settings
        float width = bounds.size.x * npcCollisionWidth;
        float height = bounds.size.y * npcCollisionHeight;
        
        // Apply custom offsets
        float offsetX = bounds.size.x * npcOffsetX;
        float offsetY = bounds.size.y * npcOffsetY;
        
        pc.collisionBox = sf::FloatRect(
            sf::Vector2f(bounds.position.x + offsetX, bounds.position.y + offsetY),
            sf::Vector2f(width, height)
        );
        
        pc.hasGravity = true;
        pc.isStatic = true; // NPCs are static by default
        pc.bounceFactor = npcBounceFactor;
        pc.friction = 0.0f;
        
        npcPhysics.push_back(pc);
    }
}

void PhysicsSystem::update(float deltaTime, Player& player, std::vector<Enemy>& enemies) {
    // Update player physics component
    sf::FloatRect playerBounds = player.getGlobalBounds();
    float width = playerBounds.size.x * playerCollisionWidth;
    float height = playerBounds.size.y * playerCollisionHeight;
    
    // Center the collision box on the sprite
    float offsetX = (playerBounds.size.x - width) / 2.0f;
    float offsetY = (playerBounds.size.y - height) / 2.0f;
    
    playerPhysics.collisionBox = sf::FloatRect(
        sf::Vector2f(playerBounds.position.x + offsetX, playerBounds.position.y + offsetY),
        sf::Vector2f(width, height)
    );
    
    // Copy player's velocity to physics system to ensure jumps are processed
    playerPhysics.velocity = player.getVelocity();
    
    // Check ground state first, but be more lenient during jumps
    bool playerOnGround = false;
    if (!player.isJumping() || playerPhysics.velocity.y > 0) {
        // Only check for ground when not jumping up
        playerOnGround = isEntityOnGround(playerPhysics, player.getPosition(), 5.0f);
    }
    
    // Update player's ground state only if not actively jumping upward
    if (!player.isJumping() || playerPhysics.velocity.y >= 0) {
        player.setOnGround(playerOnGround);
    }
    
    if (playerPhysics.hasGravity && !playerOnGround) {
        // Apply gravity
        playerPhysics.velocity.y += gravity * deltaTime;
        
        // Clamp to terminal velocity
        if (playerPhysics.velocity.y > terminalVelocity) {
            playerPhysics.velocity.y = terminalVelocity;
        }
    } else if (playerOnGround) {
        // When on ground, respect the player's vertical velocity
        // This allows jumps to be initiated
        if (playerPhysics.velocity.y < 0) {
            // Maintain the jump velocity
            std::cout << "Physics system preserving jump velocity: " << playerPhysics.velocity.y << std::endl;
        } else {
            // Reset vertical velocity when on ground and not jumping
            playerPhysics.velocity.y = 0;
            if (!player.isJumping()) {
                player.setJumping(false); // Only reset jump state if not actively jumping
            }
        }
    } 
    
    // Update enemy physics
    for (size_t i = 0; i < enemies.size() && i < enemyPhysics.size(); ++i) {
        sf::FloatRect enemyBounds = enemies[i].getGlobalBounds();
        float width = enemyBounds.size.x * enemyCollisionWidth;
        float height = enemyBounds.size.y * enemyCollisionHeight;
        
        // Apply custom offsets instead of automatic centering
        float offsetX = enemyBounds.size.x * enemyOffsetX;
        float offsetY = enemyBounds.size.y * enemyOffsetY;
        
        enemyPhysics[i].collisionBox = sf::FloatRect(
            sf::Vector2f(enemyBounds.position.x + offsetX, enemyBounds.position.y + offsetY),
            sf::Vector2f(width, height)
        );
        
        // Apply gravity to enemies
        if (enemyPhysics[i].hasGravity) {
            enemyPhysics[i].velocity.y += gravity * deltaTime;
            
            // Clamp to terminal velocity
            if (enemyPhysics[i].velocity.y > terminalVelocity) {
                enemyPhysics[i].velocity.y = terminalVelocity;
            }
        }
    }
    
    // Resolve collisions
    resolveCollisions(player, enemies);
    
    // Apply physics to entities
    applyPhysicsToEntities(player, enemies);
}

void PhysicsSystem::updateNPCs(std::vector<NPCSystem::NPCData>& npcs) {
    // Update NPC physics components
    for (size_t i = 0; i < npcs.size() && i < npcPhysics.size(); ++i) {
        if (!npcs[i].isActive) continue;
        
        // Get sprite bounds if available
        sf::FloatRect bounds;
        if (npcs[i].sprite) {
            bounds = npcs[i].sprite->getGlobalBounds();
        } else {
            bounds = sf::FloatRect(
                sf::Vector2f(npcs[i].x, npcs[i].y),
                sf::Vector2f(32.f, 32.f)
            );
        }
        
        // Update collision box
        float width = bounds.size.x * npcCollisionWidth;
        float height = bounds.size.y * npcCollisionHeight;
        float offsetX = bounds.size.x * npcOffsetX;
        float offsetY = bounds.size.y * npcOffsetY;
        
        npcPhysics[i].collisionBox = sf::FloatRect(
            sf::Vector2f(bounds.position.x + offsetX, bounds.position.y + offsetY),
            sf::Vector2f(width, height)
        );
        
        // Check for ground collision
        bool npcOnGround = isEntityOnGround(npcPhysics[i], sf::Vector2f(npcs[i].x, npcs[i].y), 5.0f);
        
        // Apply gravity if not on ground
        if (!npcOnGround && npcPhysics[i].hasGravity) {
            npcs[i].y += gravity * 0.016f; // Using a fixed timestep for stability
        }
        
        // Check platform collisions
        for (const auto& platform : platformPhysics) {
            if (checkCollision(npcPhysics[i], platform)) {
                // Position NPC on top of platform
                float newY = platform.collisionBox.position.y - height - 0.1f;
                npcs[i].y = newY;
                break;
            }
        }
        
        // Update sprite position
        if (npcs[i].sprite) {
            npcs[i].sprite->setPosition(sf::Vector2f(npcs[i].x, npcs[i].y));
        }
    }
}

bool PhysicsSystem::isEntityOnGround(const PhysicsComponent& entityPhysics, const sf::Vector2f& position, float checkDistance) const {
    // First check main ground platform
    float groundLevel = windowHeight - 100.0f; // Match Game::GROUND_HEIGHT
    float entityBottom = position.y + entityPhysics.collisionBox.size.y;
    
    // Use a smaller check distance for more stable detection
    float actualCheckDistance = 2.0f; // Reduced from the passed-in value for more stability
    
    // Check if entity is at ground level
    if (entityBottom >= groundLevel - actualCheckDistance && entityBottom <= groundLevel + actualCheckDistance) {
        return true;
    }
    
    // Then check all other platforms
    for (const auto& platform : platformPhysics) {
        float platformTop = platform.collisionBox.position.y;
        float platformLeft = platform.collisionBox.position.x;
        float platformRight = platformLeft + platform.collisionBox.size.x;
        
        // Check if entity is above the platform horizontally
        if (position.x + entityPhysics.collisionBox.size.x >= platformLeft && 
            position.x <= platformRight) {
            
            // Check if entity is at the right height for the platform
            if (entityBottom >= platformTop - actualCheckDistance && 
                entityBottom <= platformTop + actualCheckDistance) {
                return true;
            }
        }
    }
    
    return false;
}

void PhysicsSystem::resolveCollisions(Player& player, std::vector<Enemy>& enemies) {
    // Check player collision with platforms
    for (size_t i = 0; i < platformPhysics.size(); i++) {
        const auto& platform = platformPhysics[i];
        
        if (checkCollision(playerPhysics, platform)) {
            // Get previous position (before applying velocity)
            sf::Vector2f prevPos = sf::Vector2f(
                player.getPosition().x - player.getVelocity().x,
                player.getPosition().y - player.getVelocity().y
            );
            
            // Handle collision with platform
            if (playerPhysics.velocity.y > 0 && 
                prevPos.y + playerPhysics.collisionBox.size.y <= platform.collisionBox.position.y + 5) {
                // Player is falling - land on platform
                playerPhysics.velocity.y = 0;
                player.setOnGround(true);
                player.setJumping(false); // Reset jump state when landing
                
                // Position player on top of platform with a small offset to prevent sinking
                float newY = platform.collisionBox.position.y - playerPhysics.collisionBox.size.y - 0.1f;
                player.setPosition(sf::Vector2f(player.getPosition().x, newY));
                
                // Apply platform friction to horizontal velocity
                playerPhysics.velocity.x *= (1.0f - platform.friction);
            } else if (playerPhysics.velocity.y < 0 && !useOneWayPlatforms) {
                // Player is jumping - hit bottom of platform
                playerPhysics.velocity.y = -playerPhysics.velocity.y * playerPhysics.bounceFactor;
            }
        }
    }
    
    // Check enemy collisions with platforms
    for (size_t i = 0; i < enemyPhysics.size() && i < enemies.size(); ++i) {
        bool enemyOnGround = false;
        
        for (const auto& platform : platformPhysics) {
            if (checkCollision(enemyPhysics[i], platform)) {
                // Get previous position (before applying velocity)
                sf::Vector2f prevPos = sf::Vector2f(
                    enemies[i].getPosition().x - enemies[i].getVelocity().x,
                    enemies[i].getPosition().y - enemies[i].getVelocity().y
                );
                
                // Handle collision with platform
                if (enemyPhysics[i].velocity.y > 0 && 
                    prevPos.y + enemyPhysics[i].collisionBox.size.y <= platform.collisionBox.position.y + 5) {
                    // Enemy is falling - land on platform
                    enemyPhysics[i].velocity.y = 0;
                    enemyOnGround = true;
                    
                    // Position enemy on top of platform with a small offset to prevent sinking
                    float newY = platform.collisionBox.position.y - enemyPhysics[i].collisionBox.size.y - 0.1f;
                    enemies[i].setPosition(sf::Vector2f(enemies[i].getPosition().x, newY));
                    
                    // Apply platform friction to horizontal velocity
                    enemyPhysics[i].velocity.x *= (1.0f - platform.friction);
                } else if (enemyPhysics[i].velocity.y < 0 && !useOneWayPlatforms) {
                    // Enemy is jumping - hit bottom of platform
                    enemyPhysics[i].velocity.y = -enemyPhysics[i].velocity.y * enemyPhysics[i].bounceFactor;
                }
            }
        }
        
        // Special case: If enemy is below the ground platform, force them above it
        if (!enemyOnGround && enemies[i].getPosition().y > windowHeight - 90) {
            enemies[i].setPosition(sf::Vector2f(enemies[i].getPosition().x, windowHeight - 90));
            enemyPhysics[i].velocity.y = 0;
        }
    }
}

bool PhysicsSystem::checkCollision(const PhysicsComponent& a, const PhysicsComponent& b) {
    // Check if collision boxes intersect
    return rectsIntersect(a.collisionBox, b.collisionBox);
}

void PhysicsSystem::applyPhysicsToEntities(Player& player, std::vector<Enemy>& enemies) {
    // If player is jumping (negative Y velocity), preserve that
    if (player.getVelocity().y < 0) {
        // Keep player's jump velocity
        std::cout << "Preserving player jump velocity: " << player.getVelocity().y << std::endl;
    } else {
        // Otherwise apply physics system velocity
        sf::Vector2f playerVel = player.getVelocity();
        playerVel.y = playerPhysics.velocity.y;
        player.setVelocity(playerVel);
    }
    
    // Add a check to prevent player from sinking into ground
    if (player.isOnGround() && player.getVelocity().y > 0) {
        sf::Vector2f playerVel = player.getVelocity();
        playerVel.y = 0;
        player.setVelocity(playerVel);
    }
    
    // Apply enemy physics - only vertical velocity to preserve AI movement
    for (size_t i = 0; i < enemies.size() && i < enemyPhysics.size(); ++i) {
        sf::Vector2f enemyVel = enemies[i].getVelocity();
        enemyVel.y = enemyPhysics[i].velocity.y;
        
        // Ensure enemy X velocity is preserved based on their AI movement direction
        // We don't want to overwrite their horizontal movement logic
        bool movingRight = enemyVel.x > 0;
        
        // Update the velocity
        enemies[i].setVelocity(enemyVel);
        
        // Comprehensive check for stuck enemies at screen edges
        sf::Vector2f pos = enemies[i].getPosition();
        
        // Fix enemies stuck at left edge
        if (pos.x < 10.0f) {
            pos.x = 20.0f; // Move them away from edge
            enemies[i].setPosition(pos);
            
            // Force them to move right
            enemyVel.x = std::abs(enemyVel.x);
            if (enemyVel.x < 2.0f) enemyVel.x = 2.0f; // Ensure minimum velocity
            enemies[i].setVelocity(enemyVel);
            
            std::cout << "Fixed stuck enemy " << i << " at position: " << pos.x << ", " << pos.y << std::endl;
        }
        
        // Check for potential edge case where physics and enemy AI disagree on movement
        if ((movingRight && enemyVel.x < 0) || (!movingRight && enemyVel.x > 0)) {
            // There's a conflict - respect the enemy AI's direction but ensure non-zero velocity
            float absVel = std::abs(enemyVel.x);
            if (absVel < 2.0f) absVel = 2.0f; // Minimum velocity
            
            enemyVel.x = movingRight ? absVel : -absVel;
            enemies[i].setVelocity(enemyVel);
        }
    }
}

