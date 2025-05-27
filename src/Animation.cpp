#include "Animation.hpp"
#include <filesystem>
#include <algorithm>
#include <iostream>

namespace fs = std::filesystem;

Animation::Animation() 
    : currentState(AnimationState::Idle)
    , previousState(AnimationState::Idle)
    , frameTime(0.1f)  // Default 100ms per frame
    , currentTime(0.0f)
    , currentFrame(0)
    , shouldLoop(true)
    , isPlaying(true) {
    
    createEmptySprite();
}

void Animation::createEmptySprite() {
    // Try to load a simple texture file, or create a minimal texture
    emptyTexture = std::make_unique<sf::Texture>();
    
    // Try to load a placeholder texture first
    if (!emptyTexture->loadFromFile("assets/images/characters/player.png")) {
        // If that fails, we'll just use an uninitialized texture
        // The sprite will be invisible but won't crash
        std::cerr << "Warning: Could not load placeholder texture for animation" << std::endl;
    }
    
    emptySprite = std::make_unique<sf::Sprite>(*emptyTexture);
}

Animation::~Animation() {
    // Cleanup handled by RAII
}

bool Animation::loadAnimation(AnimationState state, const std::string& directory) {
    if (!fs::exists(directory) || !fs::is_directory(directory)) {
        std::cerr << "Animation directory does not exist: " << directory << std::endl;
        return false;
    }
    
    AnimationData& animData = animations[state];
    
    if (loadFramesFromDirectory(directory, animData)) {
        animData.isLoaded = true;
        std::cout << "Successfully loaded " << animData.textures.size() 
                  << " frames for animation state " << static_cast<int>(state) << std::endl;
        return true;
    }
    
    return false;
}

bool Animation::loadFramesFromDirectory(const std::string& directory, AnimationData& animData) {
    std::vector<std::string> frameFiles = getFrameFiles(directory);
    
    if (frameFiles.empty()) {
        std::cerr << "No frame files found in directory: " << directory << std::endl;
        return false;
    }
    
    // Sort files to ensure correct frame order
    std::sort(frameFiles.begin(), frameFiles.end());
    
    animData.textures.clear();
    animData.sprites.clear();
    
    for (const auto& filename : frameFiles) {
        auto texture = std::make_unique<sf::Texture>();
        if (texture->loadFromFile(filename)) {
            // Create sprite with the texture
            auto sprite = std::make_unique<sf::Sprite>(*texture);
            
            // Add texture and sprite to vectors
            animData.sprites.push_back(std::move(sprite));
            animData.textures.push_back(std::move(texture));
            
            std::cout << "Loaded frame: " << filename << std::endl;
        } else {
            std::cerr << "Failed to load texture: " << filename << std::endl;
        }
    }
    
    return !animData.textures.empty();
}

std::vector<std::string> Animation::getFrameFiles(const std::string& directory) {
    std::vector<std::string> frameFiles;
    
    try {
        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                std::string extension = entry.path().extension().string();
                
                // Convert extension to lowercase for comparison
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
                
                // Check if it's an image file and not a spritesheet
                if ((extension == ".png" || extension == ".jpg" || extension == ".jpeg") &&
                    filename.find("spritesheet") == std::string::npos) {
                    frameFiles.push_back(entry.path().string());
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    }
    
    return frameFiles;
}

void Animation::update(float deltaTime) {
    if (!isPlaying || !hasAnimation(currentState)) {
        return;
    }
    
    const AnimationData& animData = animations[currentState];
    if (animData.sprites.empty()) {
        return;
    }
    
    currentTime += deltaTime;
    
    // Debug animation timing
    static int debugCounter = 0;
    if (debugCounter++ % 60 == 0) {
        std::cout << "Animation timing: currentTime=" << currentTime 
                  << ", frameTime=" << frameTime 
                  << ", currentFrame=" << currentFrame 
                  << ", totalFrames=" << animData.sprites.size() << std::endl;
    }
    
    if (currentTime >= frameTime) {
        currentTime = 0.0f;
        currentFrame++;
        
        if (currentFrame >= static_cast<int>(animData.sprites.size())) {
            if (shouldLoop) {
                currentFrame = 0;
            } else {
                currentFrame = static_cast<int>(animData.sprites.size()) - 1;
                isPlaying = false;
            }
        }
        
        // Debug frame change
        std::cout << "Switching to frame " << currentFrame << std::endl;
    }
}

void Animation::setState(AnimationState newState) {
    if (newState != currentState) {
        switchToState(newState);
    }
}

void Animation::switchToState(AnimationState newState) {
    previousState = currentState;
    currentState = newState;
    currentFrame = 0;
    currentTime = 0.0f;
    isPlaying = true;
    
    // Set appropriate loop settings for different animation types
    switch (newState) {
        case AnimationState::Idle:
        case AnimationState::Walking:
            shouldLoop = true;
            break;
        case AnimationState::Jumping:
        case AnimationState::Attack:
        case AnimationState::GetHit:
        case AnimationState::Die:
            shouldLoop = false;
            break;
    }
}

const sf::Sprite& Animation::getCurrentSprite() const {
    if (!hasAnimation(currentState)) {
        return *emptySprite;
    }
    
    const AnimationData& animData = animations.at(currentState);
    if (animData.sprites.empty()) {
        return *emptySprite;
    }
    
    int frameIndex = std::min(currentFrame, static_cast<int>(animData.sprites.size()) - 1);
    return *animData.sprites[frameIndex];
}

bool Animation::hasAnimation(AnimationState state) const {
    auto it = animations.find(state);
    return it != animations.end() && it->second.isLoaded && !it->second.sprites.empty();
}

void Animation::setScale(float scaleX, float scaleY) {
    for (auto& [state, animData] : animations) {
        for (auto& sprite : animData.sprites) {
            sprite->setScale(sf::Vector2f(scaleX, scaleY));
        }
    }
    emptySprite->setScale(sf::Vector2f(scaleX, scaleY));
}

void Animation::reset() {
    currentFrame = 0;
    currentTime = 0.0f;
    isPlaying = true;
}

bool Animation::isFinished() const {
    if (shouldLoop || !hasAnimation(currentState)) {
        return false;
    }
    
    const AnimationData& animData = animations.at(currentState);
    return currentFrame >= static_cast<int>(animData.sprites.size()) - 1 && !isPlaying;
} 