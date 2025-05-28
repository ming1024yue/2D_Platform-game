#include "../include/NPC.hpp"
#include <cmath>
#include <algorithm>
#include <unordered_map>

NPC::NPC(AssetManager& assetManager, RenderingSystem& renderSystem) 
    : nextId(0), assetManager(assetManager), renderSystem(renderSystem) {}

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
    
    // Create sprite with texture
    sf::Texture& texture = assetManager.getTexture(textureName);
    npc.sprite = std::make_unique<sf::Sprite>(texture);
    
    // Center the sprite origin
    sf::FloatRect bounds = npc.sprite->getLocalBounds();
    npc.sprite->setOrigin(sf::Vector2f(bounds.size.x / 2.f, bounds.size.y / 2.f));
    
    // Scale up the sprite (32x32 is quite small)
    npc.sprite->setScale(sf::Vector2f(2.0f, 2.0f));
    
    // Set initial position
    npc.sprite->setPosition(sf::Vector2f(x, y));
    
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
        
        // Update NPC state and position
        updateNPCState(npc);
        
        // Simple walking behavior: move back and forth
        static const float WALK_SPEED = 50.0f; // pixels per second
        static const float WALK_DISTANCE = 100.0f; // pixels
        
        // Store initial position if not set
        static std::unordered_map<int, float> initialPositions;
        if (initialPositions.find(npc.id) == initialPositions.end()) {
            initialPositions[npc.id] = npc.x;
        }
        
        float initialX = initialPositions[npc.id];
        
        // Update position based on current state
        if (npc.currentState == "walking") {
            // Move left or right based on facing direction
            float moveAmount = WALK_SPEED * deltaTime;
            if (npc.facingLeft) {
                npc.x -= moveAmount;
                // Check if we need to turn around
                if (npc.x < initialX - WALK_DISTANCE) {
                    npc.x = initialX - WALK_DISTANCE;
                    npc.facingLeft = false;
                    setNPCTexture(npc.id, "npc_walking");
                }
            } else {
                npc.x += moveAmount;
                // Check if we need to turn around
                if (npc.x > initialX + WALK_DISTANCE) {
                    npc.x = initialX + WALK_DISTANCE;
                    npc.facingLeft = true;
                    setNPCTexture(npc.id, "npc_walking");
                }
            }
        }
        
        // Update sprite position if it exists
        if (npc.sprite) {
            npc.sprite->setPosition(sf::Vector2f(npc.x, npc.y));
            
            // Update sprite direction
            sf::Vector2f scale = npc.sprite->getScale();
            scale.x = std::abs(scale.x) * (npc.facingLeft ? -1.f : 1.f);
            npc.sprite->setScale(scale);
        }
    }
}

void NPC::renderAll() {
    for (const auto& npc : npcs) {
        if (!npc.isActive || !npc.sprite) continue;
        
        renderSystem.renderEntity(*renderSystem.getRenderTarget(), *npc.sprite, sf::Vector2f(npc.x, npc.y));
    }
}

void NPC::setNPCPosition(int id, float x, float y) {
    if (auto* npc = getNPCById(id)) {
        npc->x = x;
        npc->y = y;
        if (npc->sprite) {
            npc->sprite->setPosition(sf::Vector2f(x, y));
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
    std::vector<std::reference_wrapper<const NPCData>> result;
    float radiusSquared = radius * radius;
    
    for (const auto& npc : npcs) {
        if (!npc.isActive) continue;
        
        if (calculateDistance(x, y, npc.x, npc.y) <= radiusSquared) {
            result.push_back(std::cref(npc));
        }
    }
    
    return result;
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

void NPC::handleInteraction(int npcId, float playerX, float playerY) {
    auto* npc = getNPCById(npcId);
    if (!npc || !npc->isActive) return;

    float distance = calculateDistance(playerX, playerY, npc->x, npc->y);
    
    // Example interaction logic
    if (distance < 2.0f) {  // Interaction range
        // TODO: Implement interaction logic
        // - Start dialogue
        // - Trade
        // - Quest giving
        // etc.
    }
}

float NPC::calculateDistance(float x1, float y1, float x2, float y2) const {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return dx * dx + dy * dy;  // Return squared distance for efficiency
}

void NPC::updateNPCState(NPCData& npc) {
    // Simple state machine for NPC behavior
    static std::unordered_map<int, float> stateTimers;
    static const float IDLE_DURATION = 2.0f;  // seconds
    static const float WALK_DURATION = 4.0f;  // seconds
    
    // Initialize timer if not set
    if (stateTimers.find(npc.id) == stateTimers.end()) {
        stateTimers[npc.id] = 0.0f;
        npc.currentState = "idle";
        setNPCTexture(npc.id, "npc_idle");
    }
    
    // Update timer
    stateTimers[npc.id] += 1.0f/60.0f;  // Assuming 60 FPS
    
    // State transitions
    if (npc.currentState == "idle" && stateTimers[npc.id] >= IDLE_DURATION) {
        npc.currentState = "walking";
        setNPCTexture(npc.id, "npc_walking");
        stateTimers[npc.id] = 0.0f;
    }
    else if (npc.currentState == "walking" && stateTimers[npc.id] >= WALK_DURATION) {
        npc.currentState = "idle";
        setNPCTexture(npc.id, "npc_idle");
        stateTimers[npc.id] = 0.0f;
    }
} 