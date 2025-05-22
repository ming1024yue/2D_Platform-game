#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <unordered_map>
#include <string>
#include <memory>
#include <stdexcept>

class AssetManager {
public:
    AssetManager() = default;
    ~AssetManager() = default;
    
    // Disable copying
    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;
    
    // Load a texture from a file
    void loadTexture(const std::string& name, const std::string& filename);
    
    // Get a texture by name
    sf::Texture& getTexture(const std::string& name);
    
    // Load a font from a file
    void loadFont(const std::string& name, const std::string& filename);
    
    // Get a font by name
    sf::Font& getFont(const std::string& name);
    
    // Load a sound buffer from a file
    void loadSoundBuffer(const std::string& name, const std::string& filename);
    
    // Get a sound buffer by name
    sf::SoundBuffer& getSoundBuffer(const std::string& name);
    
    // Clear all resources
    void clear();
    
    // Make containers public for error handling
    std::unordered_map<std::string, std::unique_ptr<sf::Texture>> textures;
    
private:
    std::unordered_map<std::string, std::unique_ptr<sf::Font>> fonts;
    std::unordered_map<std::string, std::unique_ptr<sf::SoundBuffer>> soundBuffers;
}; 