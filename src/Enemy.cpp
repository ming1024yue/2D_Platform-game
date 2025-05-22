#include "Enemy.hpp"

// Helper function for rectangle intersection (reused from Player class)
static bool enemyRectIntersect(const sf::FloatRect& a, const sf::FloatRect& b) {
    return a.position.x < b.position.x + b.size.x &&
           a.position.x + a.size.x > b.position.x &&
           a.position.y < b.position.y + b.size.y &&
           a.position.y + a.size.y > b.position.y;
}

Enemy::Enemy(float x, float y, float patrolWidth) {
    shape.setSize(sf::Vector2f(30.f, 30.f));
    shape.setPosition(sf::Vector2f(x, y));
    shape.setFillColor(sf::Color(0, 100, 0)); // Dark green enemy
    velocity = sf::Vector2f(ENEMY_SPEED, 0.f);
    startX = x;
    this->patrolWidth = patrolWidth;
    movingRight = true;
    
    // Initialize with a positive velocity to ensure enemies start moving right
    velocity.x = ENEMY_SPEED;
}

void Enemy::update(const std::vector<sf::RectangleShape>& platforms) {
    // Store previous position for collision resolution
    sf::Vector2f prevPos = shape.getPosition();
    
    // Apply gravity
    velocity.y += GRAVITY;
    
    // Safety check - prevent any stuck enemies at game start
    static bool firstUpdate = true;
    if (firstUpdate) {
        // Force moving right on first update
        movingRight = true;
        velocity.x = ENEMY_SPEED;
        firstUpdate = false;
    }
    
    // Update horizontal position based on patrol
    if (movingRight) {
        if (shape.getPosition().x >= startX + patrolWidth) {
            movingRight = false;
            velocity.x = -ENEMY_SPEED;
        }
    } else {
        if (shape.getPosition().x <= startX) {
            movingRight = true;
            velocity.x = ENEMY_SPEED;
        }
    }
    
    // Ensure velocity is properly set based on direction
    velocity.x = movingRight ? ENEMY_SPEED : -ENEMY_SPEED;
    
    // Update position
    shape.move(velocity);

    // Handle platform collisions
    bool onGround = false;
    
    for (const auto& platform : platforms) {
        if (enemyRectIntersect(shape.getGlobalBounds(), platform.getGlobalBounds())) {
            // Collision from above (falling)
            if (velocity.y > 0 && prevPos.y + shape.getSize().y <= platform.getPosition().y + 5) {
                shape.setPosition(sf::Vector2f(shape.getPosition().x,
                                platform.getPosition().y - shape.getSize().y));
                velocity.y = 0;
                onGround = true;
            }
            // Collision from side - reverse direction
            else if (velocity.y == 0 || (prevPos.y + shape.getSize().y > platform.getPosition().y + 5 &&
                                        prevPos.y < platform.getPosition().y + platform.getSize().y - 5)) {
                if (velocity.x > 0) {  // Moving right
                    shape.setPosition(sf::Vector2f(platform.getPosition().x - shape.getSize().x, shape.getPosition().y));
                    movingRight = false;
                    velocity.x = -ENEMY_SPEED;
                } else if (velocity.x < 0) {  // Moving left
                    shape.setPosition(sf::Vector2f(platform.getPosition().x + platform.getSize().x, shape.getPosition().y));
                    movingRight = true;
                    velocity.x = ENEMY_SPEED;
                }
            }
        }
    }
    
    // Fall off edge detection
    if (onGround) {
        // Check if there's a platform edge
        bool leftEdge = false;
        bool rightEdge = false;
        
        // Check left edge
        sf::Vector2f leftCheck = shape.getPosition();
        leftCheck.x -= 5.0f;
        
        // Check right edge
        sf::Vector2f rightCheck = shape.getPosition();
        rightCheck.x += shape.getSize().x + 5.0f;
        
        bool leftSupported = false;
        bool rightSupported = false;
        
        for (const auto& platform : platforms) {
            // Check if left edge is supported
            if (leftCheck.x >= platform.getPosition().x && 
                leftCheck.x <= platform.getPosition().x + platform.getSize().x &&
                std::abs(shape.getPosition().y + shape.getSize().y - platform.getPosition().y) < 5.0f) {
                leftSupported = true;
            }
            
            // Check if right edge is supported
            if (rightCheck.x >= platform.getPosition().x && 
                rightCheck.x <= platform.getPosition().x + platform.getSize().x &&
                std::abs(shape.getPosition().y + shape.getSize().y - platform.getPosition().y) < 5.0f) {
                rightSupported = true;
            }
        }
        
        // Reverse direction if about to fall off edge
        if (!leftSupported && velocity.x < 0) {
            movingRight = true;
            velocity.x = ENEMY_SPEED;
        } else if (!rightSupported && velocity.x > 0) {
            movingRight = false;
            velocity.x = -ENEMY_SPEED;
        }
    }
    
    // Enhanced edge safety checks - prevent getting stuck at screen boundaries
    
    // Enforce minimum X position to prevent enemies from getting stuck at the left edge
    if (shape.getPosition().x < 10) {
        shape.setPosition(sf::Vector2f(10, shape.getPosition().y));
        movingRight = true;
        velocity.x = ENEMY_SPEED;
    }
    
    // Reset patrol point if enemy starts near left edge to avoid patrols that go outside screen
    if (startX < 20.0f && !movingRight) {
        startX = 20.0f;
        movingRight = true;
        velocity.x = ENEMY_SPEED;
    }
    
    // If enemy has moved too far from its patrol area, reset its position
    float distanceFromStart = std::abs(shape.getPosition().x - startX);
    if (distanceFromStart > patrolWidth * 1.5f) {
        // Enemy has wandered too far - reset to start position with right movement
        shape.setPosition(sf::Vector2f(startX, shape.getPosition().y));
        movingRight = true;
        velocity.x = ENEMY_SPEED;
    }
}

void Enemy::draw(sf::RenderWindow& window) const {
    window.draw(shape);
} 