#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <deque>

struct TireMark{
    sf::Vector2f pos;
    float alpha;
    float angle;
};

extern std::vector<std::vector<int>> trackMap;
extern std::deque<TireMark> tireMarks;
extern std::vector<sf::Vector2f> trackWaypoints;
extern unsigned int currentSeed;

void generateTrack(unsigned int specificSeed=0);