#include "Player.hpp"
#include "Physics.hpp"
#include <iostream>

// Helper function for rectangle intersection (SFML 3.x compatibility)
static bool rectsIntersect(const sf::FloatRect& a, const sf::FloatRect& b) {
    return a.position.x < b.position.x + b.size.x &&
           a.position.x + a.size.x > b.position.x &&
           a.position.y < b.position.y + b.size.y &&
           a.position.y + a.size.y > b.position.y;
}

Player::Player(float x, float y, PhysicsSystem& physics) : physicsSystem(physics) {
    position = sf::Vector2f(x, y);
    collisionOffset = sf::Vector2f(0.f, 0.f);
    
    // Set up collision box
    collisionBox.setSize(sf::Vector2f(30.f, 30.f));
    collisionBox.setPosition(position);
    collisionBox.setFillColor(sf::Color::Transparent); // Invisible collision box
    collisionBox.setOutlineColor(sf::Color::Green);    // Green outline for debugging
    collisionBox.setOutlineThickness(1.0f);           // Thin outline
    
    velocity = sf::Vector2f(0.f, 0.f);
    isJumping = false;
    onGround = false; // Start in air and let physics place the player
    onLadder = false;
    facingLeft = false; // Start facing right
    maxHealth = 3; // 3 lives
    health = maxHealth;
    

}

void Player::setPosition(const sf::Vector2f& pos) {
    position = pos;
    collisionBox.setPosition(position + collisionOffset);
}

void Player::setCollisionBoxSize(const sf::Vector2f& size) {
    collisionBox.setSize(size);
    // Reapply position to ensure the collision box is positioned correctly
    collisionBox.setPosition(position + collisionOffset);
}

void Player::setCollisionBoxOffset(const sf::Vector2f& offset) {
    collisionOffset = offset;
    // Update collision box position
    collisionBox.setPosition(position + collisionOffset);
}

void Player::handleInput() {
    // Handle left and right movement
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
        physicsSystem.setPlayerAcceleration(-1.0f);
        velocity.x = PLAYER_SPEED  * physicsSystem.getPlayerAcceleration();
        facingLeft = true; // Update facing direction
    }
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
        physicsSystem.setPlayerAcceleration(1.0f);
        velocity.x = PLAYER_SPEED * physicsSystem.getPlayerAcceleration();
        facingLeft = false; // Update facing direction
    }
    else {
        physicsSystem.setPlayerAcceleration(0.0f);
        velocity.x = 0;
        // Keep the previous facing direction when not moving
    }

    // Debug space key press
    bool spacePressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);


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
        if (spacePressed) {
            onLadder = false;
            velocity.y = JUMP_FORCE;
            isJumping = true;
            std::cout << "Jumped off ladder! velocity.y=" << velocity.y << std::endl;
        }
    }
    else if (spacePressed && onGround) {
        velocity.y = JUMP_FORCE;
        isJumping = true;
        onGround = false;
        
        // Debug jumping
        std::cout << "Player jumped from position: " << position.x << ", " 
                  << position.y << " with velocity " << velocity.y << std::endl;
    }
}

bool Player::checkPlatformAbove(const std::vector<sf::RectangleShape>& platforms) {
    // Check if there's a platform right above the player that they can step onto
    for (const auto& platform : platforms) {
        float playerTop = position.y;
        float platformBottom = platform.getPosition().y + platform.getSize().y;
        
        // If platform is just above player's head (within 10 pixels)
        if (std::abs(playerTop - platformBottom) < 10.0f) {
            // Check if player's position overlaps with the platform horizontally
            float playerLeft = position.x;
            float playerRight = playerLeft + collisionBox.getSize().x;
            float platformLeft = platform.getPosition().x;
            float platformRight = platformLeft + platform.getSize().x;
            
            if (playerRight > platformLeft && playerLeft < platformRight) {
                return true;
            }
        }
    }
    return false;
}

void Player::update(float deltaTime,const std::vector<sf::RectangleShape>& platforms, const std::vector<sf::RectangleShape>& ladders) {
    // Store previous position for collision resolution
    sf::Vector2f prevPosition = position;
    

    
    // Check ladder interaction
    bool wasOnLadder = onLadder;
    onLadder = false;
    
    for (const auto& ladder : ladders) {
        if (rectsIntersect(collisionBox.getGlobalBounds(), ladder.getGlobalBounds())) {
            onLadder = true;
            break;
        }
    }
    
    // Process input before movement
    handleInput();
    
    // Apply gravity only when not on ladder and not being handled by physics system
    if (!onLadder && !onGround) {
        // Use a reduced gravity when physics system is active
    
        velocity.y += GRAVITY;
        std::cout << "Applied gravity, new velocity.y = " << velocity.y << std::endl;
    }
    
    // Update position
    // deltaTime = 0.17f;
    position.x += velocity.x * deltaTime + 0.5f * physicsSystem.getPlayerAcceleration()*deltaTime*deltaTime;
    position.y += velocity.y;
    collisionBox.setPosition(position + collisionOffset);
    
    // Keep player from going off the left edge only
    if (position.x < 0) {
        position.x = 0;
        collisionBox.setPosition(position + collisionOffset);
    }
    
    // Check for transitions from ladder to platforms
    if (onLadder) {
        for (const auto& platform : platforms) {
            // If player's head is at platform level when climbing up
            if (velocity.y < 0 && // Moving up
                position.y <= platform.getPosition().y &&
                position.y + 5.0f >= platform.getPosition().y &&
                position.x + collisionBox.getSize().x > platform.getPosition().x &&
                position.x < platform.getPosition().x + platform.getSize().x) {
                
                // Move player onto platform
                position.y = platform.getPosition().y - collisionBox.getSize().y - 1.0f;
                collisionBox.setPosition(position + collisionOffset);
                onLadder = false;
                onGround = true;
                break;
            }
        }
    }
    
    // Check collisions with platforms
    bool wasOnGround = onGround;
    onGround = false;
    
    // First do a simple ground check - are we exactly at ground level?
    if (position.y >= 540.0f && position.y <= 542.0f) {
        onGround = true;
        velocity.y = 0;
        std::cout << "Standing on main ground level" << std::endl;
    }
    
    // Now check platform collisions
    for (const auto& platform : platforms) {
        if (rectsIntersect(collisionBox.getGlobalBounds(), platform.getGlobalBounds())) {
            // Skip platform collisions when on ladder (can climb through)
            if (onLadder) {
                continue;
            }
            
            // Player is falling onto platform
            if (velocity.y >= 0 && prevPosition.y + collisionBox.getSize().y <= platform.getPosition().y + 5) {
                // Position player on top of platform with a larger offset to prevent sinking
                position.y = platform.getPosition().y - collisionBox.getSize().y - 1.0f;
                collisionBox.setPosition(position + collisionOffset);
                velocity.y = 0;
                onGround = true;
                std::cout << "Landed on platform at " << position.y << std::endl;
            }
            // Player is jumping into platform from below
            else if (velocity.y < 0 && prevPosition.y >= platform.getPosition().y + platform.getSize().y - 5) {
                position.y = platform.getPosition().y + platform.getSize().y + 0.5f;
                collisionBox.setPosition(position + collisionOffset);
                velocity.y = 0;
            }
            // Horizontal collision (side) - only check when not on ladder
            else if (!onLadder && (velocity.y == 0 || (prevPosition.y + collisionBox.getSize().y > platform.getPosition().y + 5 &&
                                         prevPosition.y < platform.getPosition().y + platform.getSize().y - 5))) {
                // Check which side we hit and push back
                if (velocity.x > 0) {  // Moving right
                    position.x = platform.getPosition().x - collisionBox.getSize().x - 0.5f;
                } else if (velocity.x < 0) {  // Moving left
                    position.x = platform.getPosition().x + platform.getSize().x + 0.5f;
                }
                collisionBox.setPosition(position + collisionOffset);
                velocity.x = 0;
            }
        }
    }
    
    // Additional check to prevent sinking through platforms when standing still
    if (onGround) {
        velocity.y = 0;
    }

}

void Player::draw(sf::RenderWindow& window) {
    // Only draw the collision box for debugging
    window.draw(collisionBox);
}

void Player::reset(float x, float y) {
    position = sf::Vector2f(x, y);
    collisionBox.setPosition(position + collisionOffset);
    velocity = sf::Vector2f(0.f, 0.f);
    isJumping = false;
    onGround = false;
    onLadder = false;
    facingLeft = false;
    health = maxHealth;
    
    // Debug output for reset position
    std::cout << "Player reset to: " << x << ", " << y << std::endl;
} 