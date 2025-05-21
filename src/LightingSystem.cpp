#include "../include/LightingSystem.hpp"

LightingSystem::LightingSystem() 
    : windowWidth(800), windowHeight(600), ambientColor(30, 30, 40, 200) {
    // Default initialization
    darkOverlay.setFillColor(ambientColor);
}

void LightingSystem::initialize(int width, int height, const sf::Color& color) {
    windowWidth = width;
    windowHeight = height;
    ambientColor = color;
    
    // Initialize the dark overlay
    darkOverlay.setSize(sf::Vector2f(windowWidth, windowHeight));
    darkOverlay.setFillColor(ambientColor);
}

void LightingSystem::addLight(const sf::Vector2f& position, float radius, 
                            const sf::Color& color, float intensity) {
    PointLight light;
    light.position = position;
    light.radius = radius;
    light.color = color;
    light.intensity = intensity;
    lights.push_back(light);
}

void LightingSystem::clearLights() {
    lights.clear();
}

void LightingSystem::updateLight(int index, const sf::Vector2f& newPosition) {
    if (index >= 0 && index < lights.size()) {
        lights[index].position = newPosition;
    }
}

void LightingSystem::setAmbientColor(const sf::Color& color) {
    ambientColor = color;
    darkOverlay.setFillColor(ambientColor);
}

void LightingSystem::draw(sf::RenderWindow& window, const sf::View& view) {
    // Get view center to position lights relative to the view
    sf::Vector2f viewCenter = view.getCenter();
    sf::Vector2f viewSize = view.getSize();
    sf::Vector2f viewTopLeft = viewCenter - viewSize / 2.f;
    
    // Make ambient darkness semi-transparent to ensure visibility
    // Higher alpha = darker, lower alpha = more visible background
    sf::Color transparentAmbient = ambientColor;
    transparentAmbient.a = 120; // Reduce from default 200 to 120 for more transparency
    
    // Create a fullscreen shape for darkness/ambient light
    darkOverlay.setPosition(viewTopLeft);
    darkOverlay.setSize(viewSize);
    darkOverlay.setFillColor(transparentAmbient);
    
    // Draw darkness overlay with alpha blending to ensure visibility
    sf::RenderStates darknessState;
    darknessState.blendMode = sf::BlendMode(
        sf::BlendMode::Factor::SrcAlpha,
        sf::BlendMode::Factor::OneMinusSrcAlpha
    );
    window.draw(darkOverlay, darknessState);
    
    // Draw each light with enhanced non-circular appearance
    for (const auto& light : lights) {
        // Skip lights that are too far from the view
        if (light.position.x < viewTopLeft.x - light.radius * 1.2f || 
            light.position.x > viewTopLeft.x + viewSize.x + light.radius * 1.2f ||
            light.position.y < viewTopLeft.y - light.radius * 1.2f || 
            light.position.y > viewTopLeft.y + viewSize.y + light.radius * 1.2f) {
            continue;
        }
        
        // Use many more layers for an even smoother gradient
        const int LAYERS = 15; // Increased from 8 to 15
        
        // Draw from outermost (largest, most transparent) to innermost layer
        for (int i = 0; i < LAYERS; i++) {
            // Create a more extreme non-linear size progression
            // This creates a very gradual falloff at the edges
            float t = static_cast<float>(i) / (LAYERS - 1); // Normalized 0.0 to 1.0
            
            // Apply a custom easing function for a more natural radius progression
            // This creates very slow falloff at the outer edges and faster changes near center
            float sizeFactor = 1.0f - std::pow(t, 0.75f); // Non-linear sizing
            float currentRadius = light.radius * sizeFactor;
            
            // Create extra layers at the outer edges
            if (i < 3) {
                // Extremely subtle outer layers that extend slightly beyond the normal radius
                currentRadius = light.radius * (1.0f + 0.1f * (3 - i) / 3.0f);
            }
            
            // Create a convolution of two circles with slight offset for each layer
            // This helps eliminate the perfect circular appearance
            
            // First circle - main light
            sf::CircleShape mainShape(currentRadius);
            mainShape.setOrigin(sf::Vector2f(currentRadius, currentRadius));
            mainShape.setPosition(sf::Vector2f(light.position));
            
            // Create a very complex alpha falloff function that has a much more
            // natural light appearance with extremely subtle edges
            
            // Base alpha calculation with stronger falloff at edges
            float baseAlpha;
            if (i < 3) {
                // Ultra-soft edges for the outermost layers
                baseAlpha = 30.0f * (1.0f - static_cast<float>(i) / 3.0f);
            } else {
                // Non-linear alpha distribution for middle and inner layers
                float normalizedPos = static_cast<float>(i - 3) / (LAYERS - 3);
                baseAlpha = 30.0f + 70.0f * std::pow(normalizedPos, 1.8f);
            }
            
            // Scale by intensity
            baseAlpha *= light.intensity;
            
            // Clamp to valid range and convert to uint8_t
            baseAlpha = std::min(100.0f, baseAlpha); // Keep max alpha lower for transparency
            
            // Set color with calculated alpha
            sf::Color layerColor = light.color;
            
            // Brighten inner layers
            if (i > LAYERS / 2) {
                float brightnessFactor = 1.0f + 0.4f * ((float)(i - LAYERS/2) / (LAYERS/2));
                layerColor.r = static_cast<uint8_t>(std::min(255, (int)(layerColor.r * brightnessFactor)));
                layerColor.g = static_cast<uint8_t>(std::min(255, (int)(layerColor.g * brightnessFactor)));
                layerColor.b = static_cast<uint8_t>(std::min(255, (int)(layerColor.b * brightnessFactor)));
            }
            
            layerColor.a = static_cast<uint8_t>(baseAlpha);
            mainShape.setFillColor(layerColor);
            
            // Use a blend mode that adds light while preserving background visibility
            sf::RenderStates states;
            states.blendMode = sf::BlendMode(
                sf::BlendMode::Factor::SrcAlpha,
                sf::BlendMode::Factor::One, 
                sf::BlendMode::Equation::Add,
                sf::BlendMode::Factor::SrcAlpha,
                sf::BlendMode::Factor::OneMinusSrcAlpha,
                sf::BlendMode::Equation::Add
            );
            
            // Draw the main shape
            window.draw(mainShape, states);
            
            // If this is one of the middle layers, add a subtle offset secondary glow
            // This creates a non-uniform appearance that breaks the perfect circle
            if (i > 1 && i < LAYERS - 2) {
                // Create a secondary glow with slight offset
                // The offset direction alternates based on layer index
                float offsetX = ((i % 3) - 1) * (currentRadius * 0.15f);
                float offsetY = (((i+1) % 3) - 1) * (currentRadius * 0.15f);
                
                sf::CircleShape secondaryShape(currentRadius * 0.85f);
                secondaryShape.setOrigin(sf::Vector2f(currentRadius * 0.85f, currentRadius * 0.85f));
                secondaryShape.setPosition(sf::Vector2f(light.position.x + offsetX, light.position.y + offsetY));
                
                // Make secondary shape slightly more transparent
                sf::Color secondaryColor = layerColor;
                secondaryColor.a = static_cast<uint8_t>(baseAlpha * 0.7f);
                secondaryShape.setFillColor(secondaryColor);
                
                // Draw the secondary shape with the same blend mode
                window.draw(secondaryShape, states);
            }
        }
    }
} 