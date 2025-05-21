#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

class Enemy {
public:
    Enemy(float x, float y, float patrolWidth = 100.0f);
    void update(const std::vector<sf::RectangleShape>& platforms);
    void draw(sf::RenderWindow& window) const;
    sf::FloatRect getGlobalBounds() const { return shape.getGlobalBounds(); }

private:
    sf::RectangleShape shape;
    sf::Vector2f velocity;
    float startX;
    float patrolWidth;
    bool movingRight;
    
    static constexpr float ENEMY_SPEED = 2.0f;
    static constexpr float GRAVITY = 0.8f;
}; 