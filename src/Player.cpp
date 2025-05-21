#include "Player.hpp"

// Helper function for rectangle intersection (SFML 3.x compatibility)
static bool rectsIntersect(const sf::FloatRect& a, const sf::FloatRect& b) {
    return a.position.x < b.position.x + b.size.x &&
           a.position.x + a.size.x > b.position.x &&
           a.position.y < b.position.y + b.size.y &&
           a.position.y + a.size.y > b.position.y;
}

Player::Player(float x, float y) {
    shape.setSize(sf::Vector2f(30.f, 30.f));
    shape.setPosition(sf::Vector2f(x, y));
    shape.setFillColor(sf::Color::Red);
    velocity = sf::Vector2f(0.f, 0.f);
    isJumping = false;
    onGround = false;
    onLadder = false;
    maxHealth = 3; // 3 lives
    health = maxHealth;
}

void Player::handleInput() {
    // Handle left and right movement
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
        velocity.x = -PLAYER_SPEED;
    }
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
        velocity.x = PLAYER_SPEED;
    }
    else {
        velocity.x = 0;
    }

    // Handle climbing or jumping
    if (onLadder) {
        // Vertical movement on ladder
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
            velocity.y = -CLIMB_SPEED;
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
            velocity.y = CLIMB_SPEED;
        }
        else {
            velocity.y = 0;
        }
        
        // Allow jumping off ladder
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) {
            onLadder = false;
            velocity.y = JUMP_FORCE;
            isJumping = true;
        }
    }
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) && onGround) {
        velocity.y = JUMP_FORCE;
        isJumping = true;
        onGround = false;
    }
}

bool Player::checkPlatformAbove(const std::vector<sf::RectangleShape>& platforms) {
    // Check if there's a platform right above the player that they can step onto
    for (const auto& platform : platforms) {
        float playerTop = shape.getPosition().y;
        float platformBottom = platform.getPosition().y + platform.getSize().y;
        
        // If platform is just above player's head (within 10 pixels)
        if (std::abs(playerTop - platformBottom) < 10.0f) {
            // Check if player's position overlaps with the platform horizontally
            float playerLeft = shape.getPosition().x;
            float playerRight = playerLeft + shape.getSize().x;
            float platformLeft = platform.getPosition().x;
            float platformRight = platformLeft + platform.getSize().x;
            
            if (playerRight > platformLeft && playerLeft < platformRight) {
                return true;
            }
        }
    }
    return false;
}

void Player::update(const std::vector<sf::RectangleShape>& platforms, const std::vector<sf::RectangleShape>& ladders) {
    // Store previous position for collision resolution
    sf::Vector2f prevPosition = shape.getPosition();
    
    // Check ladder interaction
    bool wasOnLadder = onLadder;
    onLadder = false;
    
    for (const auto& ladder : ladders) {
        if (rectsIntersect(shape.getGlobalBounds(), ladder.getGlobalBounds())) {
            onLadder = true;
            break;
        }
    }
    
    handleInput();
    
    // Apply gravity only when not on ladder
    if (!onLadder) {
        velocity.y += GRAVITY;
    }
    
    // Update position
    shape.move(velocity);
    
    // Keep player from going off the left edge only
    if (shape.getPosition().x < 0) {
        shape.setPosition(sf::Vector2f(0, shape.getPosition().y));
    }
    
    // Check for transitions from ladder to platforms
    if (onLadder) {
        for (const auto& platform : platforms) {
            // If player's head is at platform level when climbing up
            if (velocity.y < 0 && // Moving up
                shape.getPosition().y <= platform.getPosition().y &&
                shape.getPosition().y + 5.0f >= platform.getPosition().y &&
                shape.getPosition().x + shape.getSize().x > platform.getPosition().x &&
                shape.getPosition().x < platform.getPosition().x + platform.getSize().x) {
                
                // Move player onto platform
                shape.setPosition(sf::Vector2f(shape.getPosition().x, platform.getPosition().y - shape.getSize().y));
                onLadder = false;
                onGround = true;
                break;
            }
        }
    }
    
    // Check collisions with platforms
    onGround = false;
    for (const auto& platform : platforms) {
        if (rectsIntersect(shape.getGlobalBounds(), platform.getGlobalBounds())) {
            // Skip platform collisions when on ladder (can climb through)
            if (onLadder) {
                continue;
            }
            
            // Player is falling onto platform
            if (velocity.y > 0 && prevPosition.y + shape.getSize().y <= platform.getPosition().y + 5) {
                shape.setPosition(sf::Vector2f(shape.getPosition().x,
                                platform.getPosition().y - shape.getSize().y));
                velocity.y = 0;
                onGround = true;
            }
            // Player is jumping into platform from below
            else if (velocity.y < 0 && prevPosition.y >= platform.getPosition().y + platform.getSize().y - 5) {
                shape.setPosition(sf::Vector2f(shape.getPosition().x,
                                platform.getPosition().y + platform.getSize().y));
                velocity.y = 0;
            }
            // Horizontal collision (side) - only check when not on ladder
            else if (!onLadder && (velocity.y == 0 || (prevPosition.y + shape.getSize().y > platform.getPosition().y + 5 &&
                                         prevPosition.y < platform.getPosition().y + platform.getSize().y - 5))) {
                // Check which side we hit and push back
                if (velocity.x > 0) {  // Moving right
                    shape.setPosition(sf::Vector2f(platform.getPosition().x - shape.getSize().x, shape.getPosition().y));
                } else if (velocity.x < 0) {  // Moving left
                    shape.setPosition(sf::Vector2f(platform.getPosition().x + platform.getSize().x, shape.getPosition().y));
                }
                velocity.x = 0;
            }
        }
    }
}

void Player::draw(sf::RenderWindow& window) {
    window.draw(shape);
} 