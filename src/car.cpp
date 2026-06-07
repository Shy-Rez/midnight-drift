#include "car.h"

CarVisuals::CarVisuals(){
    chassis.setSize(sf::Vector2f(14.f,7.f));
    chassis.setOrigin(sf::Vector2f(7.f,3.5f));
    chassis.setFillColor(sf::Color(15,15,20));
    chassis.setOutlineThickness(1.f);
    chassis.setOutlineColor(sf::Color(0,255,255));

    hood.setSize(sf::Vector2f(4.f,5.f));
    hood.setOrigin(sf::Vector2f(1.f,2.5f));
    hood.setFillColor(sf::Color(25,25,30));
    hood.setOutlineThickness(1.f);
    hood.setOutlineColor(sf::Color(255,0,255,100));

    cabin.setSize(sf::Vector2f(6.f,5.f));
    cabin.setOrigin(sf::Vector2f(1.5f,2.5f));
    cabin.setFillColor(sf::Color(10,5,15,240));
    cabin.setOutlineThickness(1.f);
    cabin.setOutlineColor(sf::Color(255,0,255));

    spoiler.setSize(sf::Vector2f(2.5f,8.f));
    spoiler.setOrigin(sf::Vector2f(8.f,4.f));
    spoiler.setFillColor(sf::Color(10,10,15));
    spoiler.setOutlineThickness(1.f);
    spoiler.setOutlineColor(sf::Color(0,255,255));

    leftH.setSize(sf::Vector2f(1.5f,2.f));
    leftH.setOrigin(sf::Vector2f(-6.f,3.f));
    leftH.setFillColor(sf::Color::White);
    leftH.setOutlineThickness(1.f);
    leftH.setOutlineColor(sf::Color(0,255,255,150));

    rightH.setSize(sf::Vector2f(1.5f,2.f));
    rightH.setOrigin(sf::Vector2f(-6.f,-1.f));
    rightH.setFillColor(sf::Color::White);
    rightH.setOutlineThickness(1.f);
    rightH.setOutlineColor(sf::Color(0,255,255,150));
}

void CarVisuals::updateAndDraw(sf::Vector2f pos, float angle,sf::RenderWindow& window) {
    auto drawPart=[&](sf::RectangleShape& part) {
        part.setPosition(pos);
        part.setRotation(sf::degrees(angle));
        window.draw(part);
    };
    drawPart(chassis);drawPart(hood);
    drawPart(leftH);drawPart(rightH);
    drawPart(cabin);drawPart(spoiler);
}

void CarVisuals::makeGhost() {
    auto setGhost=[](sf::RectangleShape& r) {
        r.setFillColor(sf::Color(0,255,255,40));
        r.setOutlineColor(sf::Color(0,255,255,120));
    };
    setGhost(chassis);setGhost(hood);setGhost(cabin);setGhost(spoiler);
    setGhost(leftH);setGhost(rightH);
}