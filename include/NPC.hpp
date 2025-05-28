#pragma once

#include <vector>
#include <string>
#include <memory>
#include <SFML/Graphics.hpp>
#include "Physics.hpp"
#include "Animation.hpp"
#include "AssetManager.hpp"
#include "RenderingSystem.hpp"

// Forward declarations and types in NPCSystem namespace
namespace NPCSystem {
    struct NPCData {
        int id;
        std::string name;
        float x, y;           // Position
        float health;
        bool isActive;
        std::string currentState;  // e.g., "idle", "walking", "talking"
        std::unique_ptr<sf::Sprite> sprite;
        bool facingLeft;
        std::unique_ptr<Animation> animation;  // Add animation support
        bool isInteracting;  // Flag to indicate if NPC is interacting with player
        std::string currentMessage;  // Current message being displayed
        float messageTimer;  // Timer for how long to show the message
        std::unique_ptr<sf::RectangleShape> messageBox;  // Shape for message background
        std::unique_ptr<sf::Text> messageText;  // Text object for rendering message
        sf::FloatRect collisionBounds;  // Collision bounds for interaction
    };
}

class NPC {
public:
    using NPCData = NPCSystem::NPCData;  // Alias for convenience

    NPC(AssetManager& assetManager, RenderingSystem& renderSystem);
    ~NPC();

    // Instance management
    int createNPC(const std::string& name, const std::string& textureName, float x, float y);
    void addNPC(const std::string& name, float x, float y);
    void removeNPC(int id);
    void updateAll(float deltaTime);
    void renderAll();

    // Individual NPC controls
    void setNPCPosition(int id, float x, float y);
    void setNPCState(int id, const std::string& state);
    void setNPCHealth(int id, float health);
    void setNPCActive(int id, bool active);
    void setNPCTexture(int id, const std::string& textureName);
    void setNPCFacing(int id, bool facingLeft);
    void setFont(const sf::Font& font) { messageFont = font; }  // New method to set font

    // Getters
    const std::vector<NPCData>& getAllNPCs() const;
    NPCData* getNPCById(int id);
    std::vector<std::reference_wrapper<const NPCData>> getNPCsInRange(float x, float y, float radius) const;

    // AI and behavior
    void updateAI(float deltaTime);
    void handleInteraction(int npcId, const sf::FloatRect& playerBounds);  // Updated to use collision bounds
    void displayMessage(int npcId, const std::string& message, float duration = 3.0f);

private:
    std::vector<NPCData> npcs;
    int nextId;
    AssetManager& assetManager;
    RenderingSystem& renderSystem;
    sf::Font messageFont;  // Font for rendering messages

    // Helper functions
    void updateNPCState(NPCData& npc);
    void updateNPCAnimation(NPCData& npc, float deltaTime);
    void updateCollisionBounds(NPCData& npc);  // New helper function
    float calculateDistance(float x1, float y1, float x2, float y2) const;  // Added missing declaration
}; 