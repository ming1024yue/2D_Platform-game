#include "../include/NPC.hpp"
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <iostream>

// Constants
static const float VERTICAL_OFFSET = 50.0f;  // Distance above NPC for message box

// Helper function for rectangle intersection (for SFML 3.x compatibility)
static bool rectsIntersect(const sf::FloatRect& a, const sf::FloatRect& b) {
    return a.position.x < b.position.x + b.size.x &&
           a.position.x + a.size.x > b.position.x &&
           a.position.y < b.position.y + b.size.y &&
           a.position.y + a.size.y > b.position.y;
}

NPC::NPC(AssetManager& assetManager, RenderingSystem& renderSystem) 
    : nextId(0), assetManager(assetManager), renderSystem(renderSystem) {
    
    // Try to load a font that supports Chinese characters
    std::vector<std::string> fontPaths = {
        "assets/fonts/NotoSansSC-Regular.ttf",  // Noto Sans SC font (supports Chinese)
        "/System/Library/Fonts/PingFang.ttc",   // System Chinese font on macOS
        "/System/Library/Fonts/STHeiti Light.ttc", // Alternative system Chinese font
        "assets/fonts/pixel.ttf"  // Fallback to pixel font
    };
    
    bool fontLoaded = false;
    for (const auto& path : fontPaths) {
        if (messageFont.openFromFile(path)) {
            std::cout << "Successfully loaded font: " << path << std::endl;
            fontLoaded = true;
            break;
        }
    }
    
    if (!fontLoaded) {
        std::cerr << "Error: Could not load any suitable font!" << std::endl;
    }
}

NPC::~NPC() {}

int NPC::createNPC(const std::string& name, const std::string& textureName, float x, float y) {
    NPCData npc;
    npc.id = nextId++;
    npc.name = name;
    npc.x = x;
    npc.y = y;
    npc.health = 100.0f;
    npc.isActive = true;
    npc.currentState = "idle";
    npc.facingLeft = false;
    npc.isInteracting = false;
    
    // Initialize animation
    npc.animation = std::make_unique<Animation>();
    npc.animation->loadAnimation(AnimationState::Idle, "assets/images/npc/separated/idle");
    npc.animation->loadAnimation(AnimationState::Walking, "assets/images/npc/separated/walking");
    npc.animation->setFrameTime(0.2f); // 200ms per frame
    npc.animation->setScale(2.0f, 2.0f);
    npc.animation->setOrigin(sf::Vector2f(16.f, 16.f)); // Center of 32x32 sprite
    npc.animation->setState(AnimationState::Idle);
    
    // Create sprite for collision and initial setup
    sf::Texture& texture = assetManager.getTexture(textureName);
    npc.sprite = std::make_unique<sf::Sprite>(texture);
    
    // Center the sprite origin
    sf::FloatRect bounds = npc.sprite->getLocalBounds();
    npc.sprite->setOrigin(sf::Vector2f(bounds.size.x / 2.f, bounds.size.y / 2.f));
    npc.sprite->setScale(sf::Vector2f(2.0f, 2.0f));
    npc.sprite->setPosition(sf::Vector2f(x, y));
    
    // Initialize collision bounds
    updateCollisionBounds(npc);
    
    // Add to NPCs vector
    int id = npc.id;
    npcs.push_back(std::move(npc));
    return id;
}

void NPC::addNPC(const std::string& name, float x, float y) {
    NPCData npc;
    npc.id = nextId++;
    npc.name = name;
    npc.x = x;
    npc.y = y;
    npc.health = 100.0f;
    npc.isActive = true;
    npc.currentState = "idle";
    npc.facingLeft = false;
    npcs.push_back(std::move(npc));
}

void NPC::removeNPC(int id) {
    npcs.erase(
        std::remove_if(npcs.begin(), npcs.end(),
            [id](const NPCData& npc) { return npc.id == id; }),
        npcs.end()
    );
}

void NPC::updateAll(float deltaTime) {
    for (auto& npc : npcs) {
        if (!npc.isActive) continue;
        
        // Update message timer
        if (npc.messageTimer > 0) {
            npc.messageTimer -= deltaTime;
            if (npc.messageTimer <= 0) {
                // Clear both message and visual elements together
                npc.currentMessage = "";
                npc.messageBox.reset();
                npc.messageText.reset();
            }
        }
        
        // If NPC is interacting, force idle state and skip movement
        if (npc.isInteracting) {
            npc.currentState = "idle";
            updateNPCAnimation(npc, deltaTime);
            
            // Update sprite position and scale even when idle
            if (npc.sprite) {
                npc.sprite->setPosition(sf::Vector2f(npc.x, npc.y));
                sf::Vector2f scale = npc.sprite->getScale();
                scale.x = std::abs(scale.x) * (npc.facingLeft ? -1.f : 1.f);
                npc.sprite->setScale(scale);
                // Update collision bounds even when idle
                updateCollisionBounds(npc);
            }
            continue;  // Skip the rest of the update for this NPC
        }
        
        // Update NPC state
        updateNPCState(npc);
        
        // Store initial position if not set
        static std::unordered_map<int, float> initialPositions;
        if (initialPositions.find(npc.id) == initialPositions.end()) {
            initialPositions[npc.id] = npc.x;
        }
        
        float initialX = initialPositions[npc.id];
        
        // Only move if in walking state
        if (npc.currentState == "walking") {
            static const float WALK_SPEED = 50.0f; // pixels per second
            static const float WALK_DISTANCE = 100.0f; // pixels
            
            // Move left or right based on facing direction
            float moveAmount = WALK_SPEED * deltaTime;
            if (npc.facingLeft) {
                npc.x -= moveAmount;
                // Check if we need to turn around
                if (npc.x < initialX - WALK_DISTANCE) {
                    npc.x = initialX - WALK_DISTANCE;
                    npc.facingLeft = false;
                }
            } else {
                npc.x += moveAmount;
                // Check if we need to turn around
                if (npc.x > initialX + WALK_DISTANCE) {
                    npc.x = initialX + WALK_DISTANCE;
                    npc.facingLeft = true;
                }
            }
        }
        
        // Update animation state
        updateNPCAnimation(npc, deltaTime);
        
        // Update sprite position and scale
        if (npc.sprite) {
            npc.sprite->setPosition(sf::Vector2f(npc.x, npc.y));
            sf::Vector2f scale = npc.sprite->getScale();
            scale.x = std::abs(scale.x) * (npc.facingLeft ? -1.f : 1.f);
            npc.sprite->setScale(scale);
            // Always update collision bounds after moving
            updateCollisionBounds(npc);
        }
    }
}

void NPC::renderAll() {
    for (const auto& npc : npcs) {
        if (!npc.isActive || !npc.animation) continue;
        
        // Get the current animation frame sprite
        const sf::Sprite& animatedSprite = npc.animation->getCurrentSprite();
        
        // Create a copy of the sprite to modify position and scale
        sf::Sprite renderSprite = animatedSprite;
        renderSprite.setPosition(sf::Vector2f(npc.x, npc.y));
        
        // Set the scale based on facing direction
        sf::Vector2f scale = renderSprite.getScale();
        scale.x = std::abs(scale.x) * (npc.facingLeft ? -1.f : 1.f);
        renderSprite.setScale(scale);
        
        // Render the animated sprite
        renderSystem.renderEntity(*renderSystem.getRenderTarget(), renderSprite, sf::Vector2f(npc.x, npc.y));
        
        // Render message box and text if there is one
        if (!npc.currentMessage.empty() && npc.messageBox && npc.messageText) {
            // Position the message box above the NPC
            sf::Vector2f boxPos(npc.x, npc.y);
            npc.messageBox->setPosition(boxPos);
            
            // Draw the box first
            renderSystem.getRenderTarget()->draw(*npc.messageBox);
            
            // Get the actual box bounds
            sf::FloatRect boxBounds = npc.messageBox->getGlobalBounds();
            sf::FloatRect textBounds = npc.messageText->getLocalBounds();
            
            // Calculate the box's actual position (accounting for origin offset)
            float actualBoxTop = boxPos.y - (boxBounds.size.y + VERTICAL_OFFSET);
            
            // Add padding inside the box
            const float PADDING = 10.0f;
            
            // Calculate text position to be centered inside the box with padding
            float textX = boxPos.x - (textBounds.size.x / 2.0f);
            float textY = actualBoxTop + PADDING + (textBounds.size.y / 2.0f);
            
            // Set text position and draw it
            npc.messageText->setPosition(sf::Vector2f(textX, textY));
            renderSystem.getRenderTarget()->draw(*npc.messageText);
        }
    }
}

void NPC::setNPCPosition(int id, float x, float y) {
    if (auto* npc = getNPCById(id)) {
        npc->x = x;
        npc->y = y;
        if (npc->sprite) {
            npc->sprite->setPosition(sf::Vector2f(x, y));
            updateCollisionBounds(*npc);  // Update collision bounds when position changes
        }
    }
}

void NPC::setNPCTexture(int id, const std::string& textureName) {
    if (auto* npc = getNPCById(id)) {
        sf::Texture& texture = assetManager.getTexture(textureName);
        if (!npc->sprite) {
            npc->sprite = std::make_unique<sf::Sprite>(texture);
        } else {
            npc->sprite->setTexture(texture);
        }
        
        // Maintain the origin at center
        sf::FloatRect bounds = npc->sprite->getLocalBounds();
        npc->sprite->setOrigin(sf::Vector2f(bounds.size.x / 2.f, bounds.size.y / 2.f));
        npc->sprite->setPosition(sf::Vector2f(npc->x, npc->y));
    }
}

void NPC::setNPCFacing(int id, bool facingLeft) {
    if (auto* npc = getNPCById(id)) {
        npc->facingLeft = facingLeft;
        if (npc->sprite) {
            sf::Vector2f scale = npc->sprite->getScale();
            scale.x = std::abs(scale.x) * (facingLeft ? -1.f : 1.f);
            npc->sprite->setScale(scale);
        }
    }
}

void NPC::setNPCState(int id, const std::string& state) {
    if (auto* npc = getNPCById(id)) {
        npc->currentState = state;
    }
}

void NPC::setNPCHealth(int id, float health) {
    if (auto* npc = getNPCById(id)) {
        npc->health = std::max(0.0f, std::min(100.0f, health));
    }
}

void NPC::setNPCActive(int id, bool active) {
    if (auto* npc = getNPCById(id)) {
        npc->isActive = active;
    }
}

const std::vector<NPC::NPCData>& NPC::getAllNPCs() const {
    return npcs;
}

NPC::NPCData* NPC::getNPCById(int id) {
    auto it = std::find_if(npcs.begin(), npcs.end(),
        [id](const NPCData& npc) { return npc.id == id; });
    return it != npcs.end() ? &(*it) : nullptr;
}

std::vector<std::reference_wrapper<const NPC::NPCData>> NPC::getNPCsInRange(float x, float y, float radius) const {
    std::vector<std::reference_wrapper<const NPCData>> nearbyNPCs;
    float radiusSquared = radius * radius;  // Pre-calculate squared radius
    
    for (const auto& npc : npcs) {
        if (!npc.isActive) continue;
        
        // Use the calculateDistance helper function
        float distance = calculateDistance(x, y, npc.x, npc.y);
        if (distance <= radius) {
            nearbyNPCs.push_back(std::ref(npc));
        }
    }
    
    return nearbyNPCs;
}

void NPC::updateAI(float deltaTime) {
    // TODO: Implement AI behavior
    // This should be customized based on your game's requirements
    // Example behaviors:
    // - Patrolling
    // - Following paths
    // - Responding to player proximity
    // - Engaging in dialogue
}

void NPC::updateCollisionBounds(NPCData& npc) {
    if (!npc.sprite) return;
    
    // Get the sprite's bounds
    sf::FloatRect spriteBounds = npc.sprite->getGlobalBounds();
    
    // Calculate collision box dimensions (80% of sprite size)
    float width = spriteBounds.size.x * 0.8f;
    float height = spriteBounds.size.y * 0.8f;
    
    // Calculate the center of the sprite
    float spriteCenterX = npc.x;
    float spriteCenterY = npc.y;
    
    // Position the collision box centered on the sprite
    float collisionX = spriteCenterX - width/2.0f;
    float collisionY = spriteCenterY - height/2.0f;
    
    // Create collision bounds centered on the sprite
    npc.collisionBounds = sf::FloatRect(
        sf::Vector2f(collisionX, collisionY),
        sf::Vector2f(width, height)
    );
}

void NPC::handleInteraction(int npcId, const sf::FloatRect& playerBounds) {
    NPCData* npc = getNPCById(npcId);
    if (!npc || !npc->isActive || !npc->sprite) return;
    
    // Update collision bounds to ensure they're current
    updateCollisionBounds(*npc);
    
    // Add a small tolerance to the collision check (5 pixels)
    const float INTERACTION_TOLERANCE = 5.0f;
    sf::FloatRect expandedNPCBounds = npc->collisionBounds;
    expandedNPCBounds.position.x -= INTERACTION_TOLERANCE;
    expandedNPCBounds.position.y -= INTERACTION_TOLERANCE;
    expandedNPCBounds.size.x += INTERACTION_TOLERANCE * 2;
    expandedNPCBounds.size.y += INTERACTION_TOLERANCE * 2;
    
    // Check for collision using the helper function with expanded bounds
    if (rectsIntersect(expandedNPCBounds, playerBounds)) {
        // If not already interacting, start interaction
        if (!npc->isInteracting) {
            npc->isInteracting = true;
            displayMessage(npcId, "你好", 3.0f);
            
            // Update facing direction based on player position relative to NPC center
            float npcCenterX = npc->x;
            float playerCenterX = playerBounds.position.x + playerBounds.size.x/2;
            npc->facingLeft = (playerCenterX < npcCenterX);
            
            // Update sprite scale immediately to face player
            if (npc->sprite) {
                sf::Vector2f scale = npc->sprite->getScale();
                scale.x = std::abs(scale.x) * (npc->facingLeft ? -1.f : 1.f);
                npc->sprite->setScale(scale);
            }
        }
    } else {
        // Add a larger tolerance for maintaining interaction (10 pixels)
        const float MAINTAIN_INTERACTION_TOLERANCE = 10.0f;
        expandedNPCBounds = npc->collisionBounds;
        expandedNPCBounds.position.x -= MAINTAIN_INTERACTION_TOLERANCE;
        expandedNPCBounds.position.y -= MAINTAIN_INTERACTION_TOLERANCE;
        expandedNPCBounds.size.x += MAINTAIN_INTERACTION_TOLERANCE * 2;
        expandedNPCBounds.size.y += MAINTAIN_INTERACTION_TOLERANCE * 2;
        
        // Only end interaction if player is outside the expanded maintenance bounds
        if (npc->isInteracting && !rectsIntersect(expandedNPCBounds, playerBounds)) {
            npc->isInteracting = false;
            npc->currentMessage = "";
            npc->messageTimer = 0;
        }
    }
}

void NPC::displayMessage(int npcId, const std::string& message, float duration) {
    if (auto* npc = getNPCById(npcId)) {
        npc->currentMessage = message;
        npc->messageTimer = duration;
        
        // Create message box with appropriate size for text
        const float PADDING = 20.0f;  // Increased padding for Chinese characters
        const float MIN_BOX_WIDTH = 180.0f;
        const float MIN_BOX_HEIGHT = 60.0f;  // Increased height for Chinese characters
        
        // Create text object with font and UTF-8 string
        npc->messageText = std::make_unique<sf::Text>(messageFont, "", 24);  // Initialize with font
        npc->messageText->setString(sf::String::fromUtf8(message.begin(), message.end()));
        npc->messageText->setFillColor(sf::Color::Black);
        npc->messageText->setLineSpacing(1.2f);
        
        // Get text bounds for sizing the box
        sf::FloatRect textBounds = npc->messageText->getLocalBounds();
        float boxWidth = std::max(MIN_BOX_WIDTH, textBounds.size.x + PADDING * 2);
        float boxHeight = std::max(MIN_BOX_HEIGHT, textBounds.size.y + PADDING * 2);
        
        // Create and configure message box
        npc->messageBox = std::make_unique<sf::RectangleShape>(sf::Vector2f(boxWidth, boxHeight));
        npc->messageBox->setFillColor(sf::Color(255, 255, 255, 230));
        npc->messageBox->setOutlineColor(sf::Color::Black);
        npc->messageBox->setOutlineThickness(2.0f);
        
        // Set the origin for the message box (center bottom)
        npc->messageBox->setOrigin(sf::Vector2f(boxWidth / 2.0f, boxHeight + VERTICAL_OFFSET));
    }
}

float NPC::calculateDistance(float x1, float y1, float x2, float y2) const {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return std::sqrt(dx * dx + dy * dy);
}

void NPC::updateNPCAnimation(NPCData& npc, float deltaTime) {
    if (!npc.animation) return;
    
    // Update animation state based on NPC state
    if (npc.currentState == "walking") {
        npc.animation->setState(AnimationState::Walking);
    } else {
        npc.animation->setState(AnimationState::Idle);
    }
    
    // Update animation timing
    npc.animation->update(deltaTime);
}

void NPC::updateNPCState(NPCData& npc) {
    // Skip state updates if interacting
    if (npc.isInteracting) {
        return;
    }

    // Simple state machine for NPC behavior
    static std::unordered_map<int, float> stateTimers;
    static const float IDLE_DURATION = 2.0f;  // seconds
    static const float WALK_DURATION = 4.0f;  // seconds
    
    // Initialize timer if not set
    if (stateTimers.find(npc.id) == stateTimers.end()) {
        stateTimers[npc.id] = 0.0f;
        npc.currentState = "idle";
    }
    
    // Update timer
    stateTimers[npc.id] += 1.0f/60.0f;  // Assuming 60 FPS
    
    // State transitions
    if (npc.currentState == "idle" && stateTimers[npc.id] >= IDLE_DURATION) {
        npc.currentState = "walking";
        stateTimers[npc.id] = 0.0f;
    }
    else if (npc.currentState == "walking" && stateTimers[npc.id] >= WALK_DURATION) {
        npc.currentState = "idle";
        stateTimers[npc.id] = 0.0f;
    }
} 