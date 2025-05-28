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

    // Getters
    const std::vector<NPCData>& getAllNPCs() const;
    NPCData* getNPCById(int id);
    std::vector<std::reference_wrapper<const NPCData>> getNPCsInRange(float x, float y, float radius) const;

    // AI and behavior
    void updateAI(float deltaTime);
    void handleInteraction(int npcId, float playerX, float playerY);

private:
    std::vector<NPCData> npcs;
    int nextId;
    AssetManager& assetManager;
    RenderingSystem& renderSystem;

    // Helper functions
    float calculateDistance(float x1, float y1, float x2, float y2) const;
    void updateNPCState(NPCData& npc);
    void updateNPCAnimation(NPCData& npc, float deltaTime);  // New helper function
}; 