#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

struct HUD{
    sf::RectangleShape hudBg;
    sf::Text seedText,lapText,timeText,bestTimeText,cdText;
    sf::RectangleShape mapBg;
    sf::View mapCamera;
    sf::VertexArray minimapTrack;
    sf::CircleShape minimapCar;

    float currLapTime;
    float bestLapTime;
    float startCountdown;

    HUD(const sf::Font& font,unsigned int seed);
    void loadFontAndSetup(const std::string& fontPath,unsigned int seed);
    void reset();
    void updateTimers(float dt);
    void recordLap(int currentLap);
    void updateMinimap(sf::Vector2f carPos,const std::vector<sf::Vector2f>& waypoints);
    void draw(sf::RenderWindow& window);
};