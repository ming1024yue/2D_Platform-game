#include <SFML/Graphics.hpp>
#include <iostream>

// Function to create a simple player sprite
void createPlayerSprite() {
    // Create a 64x64 render texture for the player sprite
    sf::RenderTexture renderTexture;
    if (!renderTexture.create(64, 64)) {
        std::cerr << "Failed to create render texture for player sprite" << std::endl;
        return;
    }
    
    // Clear with transparent background
    renderTexture.clear(sf::Color::Transparent);
    
    // Create body (blue rectangle)
    sf::RectangleShape body(sf::Vector2f(32.f, 48.f));
    body.setFillColor(sf::Color(0, 100, 255)); // Blue
    body.setOutlineColor(sf::Color::Black);
    body.setOutlineThickness(2.f);
    body.setPosition(16.f, 8.f);
    renderTexture.draw(body);
    
    // Create head (circle)
    sf::CircleShape head(14.f);
    head.setFillColor(sf::Color(255, 220, 180)); // Skin color
    head.setOutlineColor(sf::Color::Black);
    head.setOutlineThickness(2.f);
    head.setPosition(18.f, 2.f);
    renderTexture.draw(head);
    
    // Create eyes
    sf::CircleShape leftEye(3.f);
    leftEye.setFillColor(sf::Color::White);
    leftEye.setOutlineColor(sf::Color::Black);
    leftEye.setOutlineThickness(1.f);
    leftEye.setPosition(24.f, 8.f);
    renderTexture.draw(leftEye);
    
    sf::CircleShape rightEye(3.f);
    rightEye.setFillColor(sf::Color::White);
    rightEye.setOutlineColor(sf::Color::Black);
    rightEye.setOutlineThickness(1.f);
    rightEye.setPosition(36.f, 8.f);
    renderTexture.draw(rightEye);
    
    // Create pupils (for direction)
    sf::CircleShape leftPupil(1.5f);
    leftPupil.setFillColor(sf::Color::Black);
    leftPupil.setPosition(27.f, 10.f);
    renderTexture.draw(leftPupil);
    
    sf::CircleShape rightPupil(1.5f);
    rightPupil.setFillColor(sf::Color::Black);
    rightPupil.setPosition(39.f, 10.f);
    renderTexture.draw(rightPupil);
    
    // Create smile
    sf::ConvexShape smile;
    smile.setPointCount(4);
    smile.setPoint(0, sf::Vector2f(25.f, 20.f));
    smile.setPoint(1, sf::Vector2f(39.f, 20.f));
    smile.setPoint(2, sf::Vector2f(35.f, 25.f));
    smile.setPoint(3, sf::Vector2f(29.f, 25.f));
    smile.setFillColor(sf::Color(255, 150, 150)); // Light red
    smile.setOutlineColor(sf::Color::Black);
    smile.setOutlineThickness(1.f);
    renderTexture.draw(smile);
    
    // Create arms
    sf::RectangleShape leftArm(sf::Vector2f(10.f, 30.f));
    leftArm.setFillColor(sf::Color(0, 100, 255)); // Blue
    leftArm.setOutlineColor(sf::Color::Black);
    leftArm.setOutlineThickness(2.f);
    leftArm.setPosition(6.f, 18.f);
    renderTexture.draw(leftArm);
    
    sf::RectangleShape rightArm(sf::Vector2f(10.f, 30.f));
    rightArm.setFillColor(sf::Color(0, 100, 255)); // Blue
    rightArm.setOutlineColor(sf::Color::Black);
    rightArm.setOutlineThickness(2.f);
    rightArm.setPosition(48.f, 18.f);
    renderTexture.draw(rightArm);
    
    // Create legs
    sf::RectangleShape leftLeg(sf::Vector2f(10.f, 20.f));
    leftLeg.setFillColor(sf::Color(50, 50, 150)); // Darker blue
    leftLeg.setOutlineColor(sf::Color::Black);
    leftLeg.setOutlineThickness(2.f);
    leftLeg.setPosition(20.f, 56.f);
    renderTexture.draw(leftLeg);
    
    sf::RectangleShape rightLeg(sf::Vector2f(10.f, 20.f));
    rightLeg.setFillColor(sf::Color(50, 50, 150)); // Darker blue
    rightLeg.setOutlineColor(sf::Color::Black);
    rightLeg.setOutlineThickness(2.f);
    rightLeg.setPosition(34.f, 56.f);
    renderTexture.draw(rightLeg);
    
    // Display and save to file
    renderTexture.display();
    
    if (renderTexture.getTexture().copyToImage().saveToFile("assets/images/characters/player.png")) {
        std::cout << "Player sprite saved successfully!" << std::endl;
    } else {
        std::cerr << "Failed to save player sprite" << std::endl;
    }
}

// Function to create a simple enemy sprite
void createEnemySprite() {
    // Create a 64x64 render texture for the enemy sprite
    sf::RenderTexture renderTexture;
    if (!renderTexture.create(64, 64)) {
        std::cerr << "Failed to create render texture for enemy sprite" << std::endl;
        return;
    }
    
    // Clear with transparent background
    renderTexture.clear(sf::Color::Transparent);
    
    // Create body (round blob)
    sf::CircleShape body(24.f);
    body.setFillColor(sf::Color(220, 50, 50)); // Red
    body.setOutlineColor(sf::Color::Black);
    body.setOutlineThickness(2.f);
    body.setPosition(8.f, 8.f);
    renderTexture.draw(body);
    
    // Create eyes
    sf::CircleShape leftEye(6.f);
    leftEye.setFillColor(sf::Color::White);
    leftEye.setOutlineColor(sf::Color::Black);
    leftEye.setOutlineThickness(1.f);
    leftEye.setPosition(16.f, 16.f);
    renderTexture.draw(leftEye);
    
    sf::CircleShape rightEye(6.f);
    rightEye.setFillColor(sf::Color::White);
    rightEye.setOutlineColor(sf::Color::Black);
    rightEye.setOutlineThickness(1.f);
    rightEye.setPosition(36.f, 16.f);
    renderTexture.draw(rightEye);
    
    // Create pupils (for direction)
    sf::CircleShape leftPupil(3.f);
    leftPupil.setFillColor(sf::Color::Black);
    leftPupil.setPosition(21.f, 20.f);
    renderTexture.draw(leftPupil);
    
    sf::CircleShape rightPupil(3.f);
    rightPupil.setFillColor(sf::Color::Black);
    rightPupil.setPosition(41.f, 20.f);
    renderTexture.draw(rightPupil);
    
    // Create mouth (angry)
    sf::ConvexShape mouth;
    mouth.setPointCount(3);
    mouth.setPoint(0, sf::Vector2f(20.f, 36.f));
    mouth.setPoint(1, sf::Vector2f(40.f, 36.f));
    mouth.setPoint(2, sf::Vector2f(30.f, 46.f));
    mouth.setFillColor(sf::Color(100, 0, 0)); // Dark red
    mouth.setOutlineColor(sf::Color::Black);
    mouth.setOutlineThickness(1.f);
    renderTexture.draw(mouth);
    
    // Create spikes
    for (int i = 0; i < 8; i++) {
        float angle = i * 45.f;
        float radians = angle * 3.14159f / 180.f;
        float x = 32.f + 24.f * std::cos(radians);
        float y = 32.f + 24.f * std::sin(radians);
        
        sf::ConvexShape spike;
        spike.setPointCount(3);
        spike.setPoint(0, sf::Vector2f(32.f, 32.f));
        spike.setPoint(1, sf::Vector2f(x + 5.f * std::cos(radians - 0.3f), y + 5.f * std::sin(radians - 0.3f)));
        spike.setPoint(2, sf::Vector2f(x + 5.f * std::cos(radians + 0.3f), y + 5.f * std::sin(radians + 0.3f)));
        spike.setFillColor(sf::Color(180, 30, 30)); // Lighter red
        spike.setOutlineColor(sf::Color::Black);
        spike.setOutlineThickness(1.f);
        renderTexture.draw(spike);
    }
    
    // Display and save to file
    renderTexture.display();
    
    if (renderTexture.getTexture().copyToImage().saveToFile("assets/images/enemies/enemy.png")) {
        std::cout << "Enemy sprite saved successfully!" << std::endl;
    } else {
        std::cerr << "Failed to save enemy sprite" << std::endl;
    }
}

int main() {
    std::cout << "Creating game sprites..." << std::endl;
    
    createPlayerSprite();
    createEnemySprite();
    
    std::cout << "Sprite creation complete!" << std::endl;
    return 0;
} 