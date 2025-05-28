#pragma once

#include <vector>
#include <string>
#include <memory>
#include "Physics.hpp"
#include "Animation.hpp"

class NPC {
public:
    struct NPCData {
        int id;
        std::string name;
        float x, y;           // Position
        float health;
        bool isActive;
        std::string currentState;  // e.g., "idle", "walking", "talking"
    };

    NPC();
    ~NPC();

    // Instance management
    void addNPC(const std::string& name, float x, float y);
    void removeNPC(int id);
    void updateAll(float deltaTime);
    void renderAll();

    // Individual NPC controls
    void setNPCPosition(int id, float x, float y);
    void setNPCState(int id, const std::string& state);
    void setNPCHealth(int id, float health);
    void setNPCActive(int id, bool active);

    // Getters
    const std::vector<NPCData>& getAllNPCs() const;
    NPCData* getNPCById(int id);
    std::vector<NPCData> getNPCsInRange(float x, float y, float radius);

    // AI and behavior
    void updateAI(float deltaTime);
    void handleInteraction(int npcId, float playerX, float playerY);

private:
    std::vector<NPCData> npcs;
    int nextId;

    // Helper functions
    float calculateDistance(float x1, float y1, float x2, float y2);
    void updateNPCState(NPCData& npc);
}; 