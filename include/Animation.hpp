#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

enum class AnimationState {
    Idle,
    Walking,
    Jumping,
    Attack,
    GetHit,
    Die
};

class Animation {
public:
    Animation();
    ~Animation();
    
    // Load animation frames from directory
    bool loadAnimation(AnimationState state, const std::string& directory);
    
    // Update animation timing
    void update(float deltaTime);
    
    // Set current animation state
    void setState(AnimationState newState);
    
    // Get current animation state
    AnimationState getState() const { return currentState; }
    
    // Get current frame sprite
    const sf::Sprite& getCurrentSprite() const;
    
    // Check if animation has frames loaded
    bool hasAnimation(AnimationState state) const;
    
    // Animation settings
    void setFrameTime(float time) { frameTime = time; }
    void setLoop(bool loop) { shouldLoop = loop; }
    void setScale(float scaleX, float scaleY);
    
    // Reset animation to first frame
    void reset();
    
    // Check if animation is finished (for non-looping animations)
    bool isFinished() const;

private:
    struct AnimationData {
        std::vector<std::unique_ptr<sf::Texture>> textures;
        std::vector<std::unique_ptr<sf::Sprite>> sprites;
        bool isLoaded = false;
    };
    
    std::unordered_map<AnimationState, AnimationData> animations;
    
    AnimationState currentState;
    AnimationState previousState;
    
    float frameTime;        // Time per frame in seconds
    float currentTime;      // Current elapsed time
    int currentFrame;       // Current frame index
    bool shouldLoop;        // Whether animation should loop
    bool isPlaying;         // Whether animation is currently playing
    
    std::unique_ptr<sf::Sprite> emptySprite; // Fallback sprite when no animation is loaded
    std::unique_ptr<sf::Texture> emptyTexture; // Texture for empty sprite
    
    // Helper methods
    void switchToState(AnimationState newState);
    std::vector<std::string> getFrameFiles(const std::string& directory);
    bool loadFramesFromDirectory(const std::string& directory, AnimationData& animData);
    void createEmptySprite();
}; 