#pragma once
#include<SFML/Graphics.hpp>
#include<cmath>
#include "constants.h"
#include "track.h"

inline float vecLength(sf::Vector2f v){ 
    return std::sqrt(v.x*v.x+v.y*v.y);
}

inline int getTerrain(float x,float y){
    int gX=static_cast<int>(x/TILE);
    int gY=static_cast<int>(y/TILE);
    if (gX<0||gX>=COLS||gY<0||gY>=ROWS) return 1;
    return trackMap[gY][gX];
}

inline float lerp(float a,float b,float t) {
    return a+t*(b-a);
}

inline sf::Vector2f lerp(sf::Vector2f a,sf::Vector2f b,float t) {
    return sf::Vector2f(lerp(a.x,b.x,t),lerp(a.y,b.y,t));
}