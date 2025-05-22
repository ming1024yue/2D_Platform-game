#include "../include/AssetManager.hpp"
#include <iostream>
#include <fstream>

void AssetManager::loadTexture(const std::string& name, const std::string& filename) {
    std::cout << "Attempting to load texture: " << filename << std::endl;
    
    // Check if file exists first
    std::ifstream file(filename.c_str());
    if (!file.good()) {
        std::cout << "File does not exist or is not accessible: " << filename << std::endl;
        throw std::runtime_error("AssetManager::loadTexture - File not found: " + filename);
    }
    file.close();
    
    // Check file size
    std::ifstream fileSize(filename.c_str(), std::ios::binary | std::ios::ate);
    std::streampos size = fileSize.tellg();
    fileSize.close();
    
    if (size <= 0) {
        std::cout << "File exists but is empty (0 bytes): " << filename << std::endl;
        throw std::runtime_error("AssetManager::loadTexture - Empty file: " + filename);
    }
    
    auto texture = std::make_unique<sf::Texture>();
    if (!texture->loadFromFile(filename)) {
        std::cout << "SFML failed to load texture from file: " << filename << std::endl;
        throw std::runtime_error("AssetManager::loadTexture - Failed to load texture: " + filename);
    }
    
    std::cout << "Successfully loaded texture: " << filename << std::endl;
    textures[name] = std::move(texture);
}

sf::Texture& AssetManager::getTexture(const std::string& name) {
    auto found = textures.find(name);
    if (found == textures.end()) {
        throw std::runtime_error("AssetManager::getTexture - Texture not found: " + name);
    }
    
    return *found->second;
}

void AssetManager::loadFont(const std::string& name, const std::string& filename) {
    auto font = std::make_unique<sf::Font>();
    if (!font->openFromFile(filename)) {
        throw std::runtime_error("AssetManager::loadFont - Failed to load font: " + filename);
    }
    
    fonts[name] = std::move(font);
}

sf::Font& AssetManager::getFont(const std::string& name) {
    auto found = fonts.find(name);
    if (found == fonts.end()) {
        throw std::runtime_error("AssetManager::getFont - Font not found: " + name);
    }
    
    return *found->second;
}

void AssetManager::loadSoundBuffer(const std::string& name, const std::string& filename) {
    auto buffer = std::make_unique<sf::SoundBuffer>();
    if (!buffer->loadFromFile(filename)) {
        throw std::runtime_error("AssetManager::loadSoundBuffer - Failed to load sound: " + filename);
    }
    
    soundBuffers[name] = std::move(buffer);
}

sf::SoundBuffer& AssetManager::getSoundBuffer(const std::string& name) {
    auto found = soundBuffers.find(name);
    if (found == soundBuffers.end()) {
        throw std::runtime_error("AssetManager::getSoundBuffer - Sound buffer not found: " + name);
    }
    
    return *found->second;
}

void AssetManager::clear() {
    textures.clear();
    fonts.clear();
    soundBuffers.clear();
} 