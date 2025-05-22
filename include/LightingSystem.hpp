#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <cstdint>

// Light struct to represent light sources
struct PointLight {
    sf::Vector2f position;
    float radius;
    sf::Color color;
    float intensity;
};

class LightingSystem {
public:
    LightingSystem();
    
    // Initialize with window dimensions and ambient color
    void initialize(int windowWidth, int windowHeight, const sf::Color& ambientColor);
    
    // Add a light to the system
    void addLight(const sf::Vector2f& position, float radius, 
                  const sf::Color& color, float intensity = 1.0f);
    
    // Clear all lights
    void clearLights();
    
    // Update light positions (e.g., for moving lights)
    void updateLight(int index, const sf::Vector2f& newPosition);
    
    // Draw all lights considering the current view
    void draw(sf::RenderWindow& window, const sf::View& view);
    
    // Set ambient color (darkness level)
    void setAmbientColor(const sf::Color& color);
    
    // Enable or disable the lighting system
    void setEnabled(bool enabled);
    
    // Check if the lighting system is enabled
    bool isEnabled() const;
    
private:
    std::vector<PointLight> lights;
    sf::Color ambientColor;
    sf::RectangleShape darkOverlay;
    int windowWidth;
    int windowHeight;
    bool enabled;
}; 