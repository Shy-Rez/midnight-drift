#pragma once
#include <SFML/Graphics.hpp>

struct CarVisuals {
    sf::RectangleShape chassis;
    sf::RectangleShape hood;
    sf::RectangleShape cabin;
    sf::RectangleShape spoiler;
    sf::RectangleShape leftH, rightH;

    CarVisuals();
    void updateAndDraw(sf::Vector2f pos, float angle,sf::RenderWindow& window);
    void makeGhost();
};