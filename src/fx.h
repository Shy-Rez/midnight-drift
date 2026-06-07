#pragma once
#include<SFML/Graphics.hpp>
#include<deque>
#include<cstdlib>
#include<algorithm>

struct Particle{
    sf::Vector2f pos;
    sf::Vector2f vel;
    sf::Color color;
    float life;
};

struct FXManager{
    std::deque<Particle>particles;
    sf::RectangleShape pShape;

    FXManager(){
        pShape.setSize(sf::Vector2f(4.f,4.f));
        pShape.setOrigin(sf::Vector2f(2.f,2.f));
    }

    void Sparks(sf::Vector2f pos,float intensity){
        int count=std::min(static_cast<int>(intensity/8.f),30);
        for (int i=0;i<count;i++){
            float vx=((std::rand()%200)-100)/0.5f;
            float vy=((std::rand()%200)-100)/0.5f;
            particles.push_back({pos,sf::Vector2f(vx,vy),sf::Color(255,200+(std::rand()%55),0),0.3f+(std::rand()%30)/100.f});
        }
    }

    void Boost(sf::Vector2f pos,float ang){
        for (int i=0;i<3;i++){
            float spread=ang+((std::rand()%100)/100.f-0.5f)*0.5f;
            sf::Vector2f vel(-std::cos(spread)*(200.f+std::rand()%100),-std::sin(spread)*(200.f+std::rand()%100));
            
            particles.push_back({pos,vel,sf::Color(0,200+(std::rand()%55),255),0.2f+(std::rand()%20)/100.f});
        }
    }

    void updateAndDraw(float dt,sf::RenderWindow& window){
        for (auto& p:particles){
            p.pos+=p.vel*dt;
            p.life-=dt;
            p.color.a=static_cast<std::uint8_t>(std::max(0.f,std::min(255.f,p.life*800.f)));
        }
        while(!particles.empty()&&particles.front().life<=0.f){
            particles.pop_front();
        }
        for(const auto& p:particles){
            pShape.setPosition(p.pos);
            pShape.setFillColor(p.color);
            window.draw(pShape);
        }
    }
};