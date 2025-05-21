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
}

void Enemy::update(const std::vector<sf::RectangleShape>& platforms) {
    // Apply gravity
    velocity.y += GRAVITY;
    
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
    
    // Update position
    shape.move(velocity);

    // Handle platform collisions
    bool onGround = false;
    sf::Vector2f prevPos = shape.getPosition();
    
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
}

void Enemy::draw(sf::RenderWindow& window) const {
    window.draw(shape);
} 