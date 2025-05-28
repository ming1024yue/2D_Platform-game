#include "Player.hpp"
#include "Physics.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

// Helper function for rectangle intersection (SFML 3.x compatibility)
static bool rectsIntersect(const sf::FloatRect& a, const sf::FloatRect& b) {
    return a.position.x < b.position.x + b.size.x &&
           a.position.x + a.size.x > b.position.x &&
           a.position.y < b.position.y + b.size.y &&
           a.position.y + a.size.y > b.position.y;
}

// Add this helper method at the top of the file after the existing rectsIntersect helper
static bool checkGroundBelow(const sf::Vector2f& position, const sf::Vector2f& size, float groundY, float checkDistance) {
    float playerBottom = position.y + size.y;
    return (playerBottom >= groundY - checkDistance && playerBottom <= groundY + checkDistance);
}

Player::Player(float x, float y, PhysicsSystem& physics) : physicsSystem(physics), animationsLoaded(false) {
    position = sf::Vector2f(x, y);
    
    // Set up collision box to match sprite dimensions (scaled up for 4x sprite scale)
    collisionBox.setSize(sf::Vector2f(56.f, 56.f)); // Scaled up from 28x28 to match 4x scale
    
    // Calculate collision offset to center the box on the sprite (64x64 with 4x scale)
    collisionOffset = sf::Vector2f(
        (64.f - collisionBox.getSize().x) / 2.f,  // Center horizontally (64-56)/2 = 4
        (64.f - collisionBox.getSize().y) / 2.f   // Center vertically (64-56)/2 = 4
    );
    
    // Set up collision box with visible outline
    collisionBox.setPosition(position + collisionOffset);
    collisionBox.setFillColor(sf::Color(0, 255, 0, 32)); // Semi-transparent green fill
    collisionBox.setOutlineColor(sf::Color(0, 255, 0, 192)); // More opaque green outline
    collisionBox.setOutlineThickness(2.0f); // Thicker outline for better visibility
    
    velocity = sf::Vector2f(0.f, 0.f);
    mIsJumping = false;
    onGround = true; // Start on ground since we position the player there
    onLadder = false;
    facingLeft = false; // Start facing right
    
    // Initialize animations
    initializeAnimations();
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
        velocity.x = PLAYER_SPEED * physicsSystem.getPlayerAcceleration();
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
    }
    else if (spacePressed && onGround) {
        // Debug output before jump
        std::cout << "Jump initiated:" << std::endl;
        std::cout << "  Position before jump: (" << position.x << ", " << position.y << ")" << std::endl;
        std::cout << "  Ground state: " << (onGround ? "true" : "false") << std::endl;
        std::cout << "  Current velocity: (" << velocity.x << ", " << velocity.y << ")" << std::endl;
        
        velocity.y = JUMP_FORCE;
        mIsJumping = true;
        onGround = false;
        
        // Debug output after jump
        std::cout << "  New velocity: (" << velocity.x << ", " << velocity.y << ")" << std::endl;
        std::cout << "  Jump force applied: " << JUMP_FORCE << std::endl;
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

void Player::update(float deltaTime, const std::vector<sf::RectangleShape>& platforms, const std::vector<sf::RectangleShape>& ladders) {
    // Store previous position and state for collision resolution
    sf::Vector2f prevPosition = position;
    bool wasOnGround = onGround;
    
    // Update debug info before state changes
    debugInfo.timeInCurrentState += deltaTime;
    if (onGround != debugInfo.prevOnGround || mIsJumping != debugInfo.prevIsJumping) {
        debugInfo.stateChanges++;
        debugInfo.timeInCurrentState = 0.0f;
        
        // Debug output for state changes
        std::cout << "Player state changed - OnGround: " << onGround 
                  << ", Jumping: " << mIsJumping << std::endl;
    }
    debugInfo.prevOnGround = onGround;
    debugInfo.prevIsJumping = mIsJumping;
    
    if (onGround) {
        debugInfo.lastGroundY = position.y;
    }
    
    // Process input before movement
    handleInput();
    
    // Apply gravity only when not on ladder and not on ground
    if (!onLadder && !onGround) {
        velocity.y += GRAVITY;
    }
    
    // Update position
    position.x += velocity.x * deltaTime + 0.5f * physicsSystem.getPlayerAcceleration()*deltaTime*deltaTime;
    position.y += velocity.y;
    
    // Keep player from going off the left edge only
    if (position.x < 0) {
        position.x = 0;
    }
    
    // Update collision box position
    collisionBox.setPosition(position + collisionOffset);
    
    // Handle platform side collisions
    for (const auto& platform : platforms) {
        if (rectsIntersect(collisionBox.getGlobalBounds(), platform.getGlobalBounds())) {
            if (onLadder) continue;
            
            float platformTop = platform.getPosition().y;
            float playerBottom = position.y + collisionBox.getSize().y;
            
            // Only do side collision if we're not landing on top
            if (std::abs(playerBottom - platformTop) > 5.0f) {
                if (velocity.x > 0) {  // Moving right
                    position.x = platform.getPosition().x - collisionBox.getSize().x - 0.5f;
                } else if (velocity.x < 0) {  // Moving left
                    position.x = platform.getPosition().x + platform.getSize().x + 0.5f;
                }
                velocity.x = 0;
                collisionBox.setPosition(position + collisionOffset);
            }
        }
    }
    
    // Update animations
    updateAnimation(deltaTime);
}

void Player::draw(sf::RenderWindow& window) {
    // Draw the animated sprite if available
    if (animationsLoaded) {
        sf::Sprite animatedSprite = playerAnimation.getCurrentSprite();
        
        // Apply scale first
        if (facingLeft) {
            animatedSprite.setScale(sf::Vector2f(-4.0f, 4.0f));  // Negative X scale for flipping
        } else {
            animatedSprite.setScale(sf::Vector2f(4.0f, 4.0f));
        }
        
        // Position sprite to align with collision box
        // Since the sprite origin is at bottom center (16, 32) and scaled 4x,
        // we need to position it at the bottom center of the collision box
        sf::Vector2f spritePos;
        spritePos.x = position.x + collisionOffset.x + (collisionBox.getSize().x / 2.f);
        spritePos.y = position.y + collisionOffset.y + collisionBox.getSize().y - 4.0f; // Slight adjustment to align with ground
        animatedSprite.setPosition(spritePos);
        
        window.draw(animatedSprite);
        
        // Debug: Draw sprite bounds
        if (showDebugInfo) {
            sf::FloatRect spriteBounds = animatedSprite.getGlobalBounds();
            sf::RectangleShape spriteBoundsRect;
            spriteBoundsRect.setSize(sf::Vector2f(spriteBounds.size.x, spriteBounds.size.y));
            spriteBoundsRect.setPosition(sf::Vector2f(spriteBounds.position.x, spriteBounds.position.y));
            spriteBoundsRect.setFillColor(sf::Color::Transparent);
            spriteBoundsRect.setOutlineColor(sf::Color::Yellow);
            spriteBoundsRect.setOutlineThickness(1.0f);
            window.draw(spriteBoundsRect);
        }
    }
    
    // Draw debug visualization if enabled
    if (showDebugInfo) {
        // Draw collision box
        window.draw(collisionBox);
    }
}

void Player::reset(float x, float y) {
    position = sf::Vector2f(x, y);
    collisionBox.setPosition(position + collisionOffset);
    velocity = sf::Vector2f(0.f, 0.f);
    mIsJumping = false;
    onGround = false;
    onLadder = false;
    facingLeft = false;
    
    // Debug output for reset
    std::cout << "Player reset called - Position: (" << x << ", " << y << ")" << std::endl;
    
    // Reset animation to idle
    if (animationsLoaded) {
        playerAnimation.setState(AnimationState::Idle);
        playerAnimation.reset();
    }
}

void Player::initializeAnimations() {
    std::cout << "Initializing player animations..." << std::endl;
    
    // Load idle animation
    bool idleLoaded = playerAnimation.loadAnimation(AnimationState::Idle, 
        "assets/images/characters/separated_finn/idle");
    
    // Load walking animation
    bool walkingLoaded = playerAnimation.loadAnimation(AnimationState::Walking, 
        "assets/images/characters/separated_finn/walking");
    
    // Load jumping animation (if available)
    bool jumpingLoaded = playerAnimation.loadAnimation(AnimationState::Jumping, 
        "assets/images/characters/separated_finn/jump");
    
    // Set animation properties
    playerAnimation.setFrameTime(0.15f); // 150ms per frame for smooth animation
    playerAnimation.setScale(4.0f, 4.0f); // Scale up the sprites to match NPC size
    
    // Set the origin to the bottom center of the sprite (32x32 original size)
    // Adjust origin Y to account for the slight offset in the sprite art
    playerAnimation.setOrigin(sf::Vector2f(16.f, 20.f)); // Half width and slightly above full height
    
    // Start with idle animation
    playerAnimation.setState(AnimationState::Idle);
    
    animationsLoaded = idleLoaded || walkingLoaded || jumpingLoaded;
    
    if (animationsLoaded) {
        std::cout << "Player animations loaded successfully!" << std::endl;
        std::cout << "  Idle: " << (idleLoaded ? "✓" : "✗") << std::endl;
        std::cout << "  Walking: " << (walkingLoaded ? "✓" : "✗") << std::endl;
        std::cout << "  Jumping: " << (jumpingLoaded ? "✓" : "✗") << std::endl;
    } else {
        std::cout << "Failed to load any player animations!" << std::endl;
    }
}

void Player::updateAnimation(float deltaTime) {
    if (!animationsLoaded) {
        return;
    }
    
    // Determine which animation should be playing based on player state
    AnimationState targetState = AnimationState::Idle;
    
    // Check if player is moving horizontally (based on both input and actual velocity)
    bool isMovingHorizontally = (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) || 
                               sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) &&
                               std::abs(velocity.x) > 0.1f;
    
    // Update animation state based on player state
    if (mIsJumping && !onGround) {
        targetState = AnimationState::Jumping;
    } else if (isMovingHorizontally && onGround) {
        targetState = AnimationState::Walking;
    } else {
        targetState = AnimationState::Idle;
    }
    
    // Set the animation state
    playerAnimation.setState(targetState);
    
    // Update the animation
    playerAnimation.update(deltaTime);
}

const sf::Sprite& Player::getAnimatedSprite() const {
    if (!animationsLoaded) {
        // Return the empty sprite from the animation system
        return playerAnimation.getCurrentSprite();
    }
    
    return playerAnimation.getCurrentSprite();
}

bool Player::hasAnimations() const {
    return animationsLoaded;
}

void Player::drawDebugInfo(sf::RenderWindow& window) {
    if (!showDebugInfo) return;
    
    // Create debug text
    sf::Font debugFont;
    if (!debugFont.openFromFile("assets/fonts/Arial.ttf")) {
        std::cout << "Error loading debug font!" << std::endl;
        return;
    }
    
    // Create debug overlay
    sf::RectangleShape overlay(sf::Vector2f(200, 150));
    overlay.setFillColor(sf::Color(0, 0, 0, 180));
    overlay.setPosition(sf::Vector2f(10, 10));
    window.draw(overlay);
    
    // Prepare debug text
    std::ostringstream debugText;
    debugText << std::fixed << std::setprecision(2)
              << "Ground: " << (onGround ? "Yes" : "No") << "\n"
              << "Jumping: " << (mIsJumping ? "Yes" : "No") << "\n"
              << "Velocity: (" << velocity.x << ", " << velocity.y << ")\n"
              << "Position: (" << position.x << ", " << position.y << ")\n"
              << "Last Ground Y: " << debugInfo.lastGroundY << "\n"
              << "State Changes: " << debugInfo.stateChanges << "\n"
              << "Time in State: " << debugInfo.timeInCurrentState << "s";
    
    // Draw debug text
    sf::Text text(debugFont, debugText.str(), 14);
    text.setFillColor(sf::Color::White);
    text.setPosition(sf::Vector2f(15, 15));
    window.draw(text);
    
    // Draw state transition indicators
    if (onGround != debugInfo.prevOnGround || mIsJumping != debugInfo.prevIsJumping) {
        // Draw a marker at state change position
        sf::CircleShape stateMarker(5);
        stateMarker.setFillColor(sf::Color::Yellow);
        stateMarker.setPosition(position + collisionOffset);
        window.draw(stateMarker);
    }
    
    // Draw ground detection zone
    sf::RectangleShape groundZone;
    // Make ground zone width match collision box width
    groundZone.setSize(sf::Vector2f(collisionBox.getSize().x, 4.0f)); // 4 pixels high for visibility
    // Position ground zone at the bottom of collision box
    groundZone.setPosition(position + collisionOffset + sf::Vector2f(0, collisionBox.getSize().y));
    groundZone.setFillColor(sf::Color(0, 255, 0, 80));
    groundZone.setOutlineColor(sf::Color::Green);
    groundZone.setOutlineThickness(1);
    window.draw(groundZone);
    
    // Draw collision box outline for reference
    sf::RectangleShape collisionBoxOutline;
    collisionBoxOutline.setSize(collisionBox.getSize());
    collisionBoxOutline.setPosition(position + collisionOffset);
    collisionBoxOutline.setFillColor(sf::Color::Transparent);
    collisionBoxOutline.setOutlineColor(sf::Color::Yellow);
    collisionBoxOutline.setOutlineThickness(1);
    window.draw(collisionBoxOutline);
} 