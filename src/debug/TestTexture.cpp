#include <SFML/Graphics.hpp>
#include <iostream>
#include <fstream>

// Simple test program to check if SFML can load textures from your files
int main() {
    std::cout << "SFML Texture Loading Test" << std::endl;
    
    // Test background
    {
        std::string filename = "../../../assets/images/backgrounds/background.png";
        std::cout << "Testing: " << filename << std::endl;
        
        // Check if file exists
        std::ifstream file(filename.c_str());
        if (!file.good()) {
            std::cout << "  ERROR: File does not exist or is not accessible!" << std::endl;
            return 1;
        }
        file.close();
        
        // Check file size
        std::ifstream fileSize(filename.c_str(), std::ios::binary | std::ios::ate);
        std::streampos size = fileSize.tellg();
        fileSize.close();
        
        std::cout << "  File size: " << size << " bytes" << std::endl;
        
        if (size <= 0) {
            std::cout << "  ERROR: File exists but is empty (0 bytes)" << std::endl;
            return 1;
        }
        
        // Try to load with SFML
        sf::Texture texture;
        if (!texture.loadFromFile(filename)) {
            std::cout << "  ERROR: SFML failed to load texture from file" << std::endl;
            return 1;
        }
        
        sf::Vector2u dimensions = texture.getSize();
        std::cout << "  SUCCESS: Texture loaded. Dimensions: " << dimensions.x << "x" << dimensions.y << std::endl;
    }
    
    // Test player
    {
        std::string filename = "../../../assets/images/characters/player.png";
        std::cout << "Testing: " << filename << std::endl;
        
        // Check if file exists
        std::ifstream file(filename.c_str());
        if (!file.good()) {
            std::cout << "  ERROR: File does not exist or is not accessible!" << std::endl;
            return 1;
        }
        file.close();
        
        // Check file size
        std::ifstream fileSize(filename.c_str(), std::ios::binary | std::ios::ate);
        std::streampos size = fileSize.tellg();
        fileSize.close();
        
        std::cout << "  File size: " << size << " bytes" << std::endl;
        
        if (size <= 0) {
            std::cout << "  ERROR: File exists but is empty (0 bytes)" << std::endl;
            return 1;
        }
        
        // Try to load with SFML
        sf::Texture texture;
        if (!texture.loadFromFile(filename)) {
            std::cout << "  ERROR: SFML failed to load texture from file" << std::endl;
            return 1;
        }
        
        sf::Vector2u dimensions = texture.getSize();
        std::cout << "  SUCCESS: Texture loaded. Dimensions: " << dimensions.x << "x" << dimensions.y << std::endl;
    }
    
    // Test enemy
    {
        std::string filename = "../../../assets/images/enemies/enemy.png";
        std::cout << "Testing: " << filename << std::endl;
        
        // Check if file exists
        std::ifstream file(filename.c_str());
        if (!file.good()) {
            std::cout << "  ERROR: File does not exist or is not accessible!" << std::endl;
            return 1;
        }
        file.close();
        
        // Check file size
        std::ifstream fileSize(filename.c_str(), std::ios::binary | std::ios::ate);
        std::streampos size = fileSize.tellg();
        fileSize.close();
        
        std::cout << "  File size: " << size << " bytes" << std::endl;
        
        if (size <= 0) {
            std::cout << "  ERROR: File exists but is empty (0 bytes)" << std::endl;
            return 1;
        }
        
        // Try to load with SFML
        sf::Texture texture;
        if (!texture.loadFromFile(filename)) {
            std::cout << "  ERROR: SFML failed to load texture from file" << std::endl;
            return 1;
        }
        
        sf::Vector2u dimensions = texture.getSize();
        std::cout << "  SUCCESS: Texture loaded. Dimensions: " << dimensions.x << "x" << dimensions.y << std::endl;
    }
    
    std::cout << "All tests passed successfully!" << std::endl;
    return 0;
} 