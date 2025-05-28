#include "../include/NPC.hpp"
#include <cmath>
#include <algorithm>

NPC::NPC() : nextId(0) {}

NPC::~NPC() {}

void NPC::addNPC(const std::string& name, float x, float y) {
    NPCData npc;
    npc.id = nextId++;
    npc.name = name;
    npc.x = x;
    npc.y = y;
    npc.health = 100.0f;
    npc.isActive = true;
    npc.currentState = "idle";
    npcs.push_back(npc);
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
        
        updateNPCState(npc);
        updateAI(deltaTime);
    }
}

void NPC::renderAll() {
    for (const auto& npc : npcs) {
        if (!npc.isActive) continue;
        
        // TODO: Implement rendering logic
        // This should be implemented according to your game's rendering system
    }
}

void NPC::setNPCPosition(int id, float x, float y) {
    if (auto* npc = getNPCById(id)) {
        npc->x = x;
        npc->y = y;
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

std::vector<NPC::NPCData> NPC::getNPCsInRange(float x, float y, float radius) {
    std::vector<NPCData> result;
    float radiusSquared = radius * radius;
    
    for (const auto& npc : npcs) {
        if (!npc.isActive) continue;
        
        if (calculateDistance(x, y, npc.x, npc.y) <= radiusSquared) {
            result.push_back(npc);
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

float NPC::calculateDistance(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return dx * dx + dy * dy;  // Return squared distance for efficiency
}

void NPC::updateNPCState(NPCData& npc) {
    // TODO: Implement state update logic
    // This should update the NPC's state based on:
    // - Current behavior
    // - Health status
    // - Player interaction
    // - Environmental factors
} 