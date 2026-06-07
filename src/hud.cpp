#include "hud.h"
#include "constants.h"
#include <cstdio>
#include <cmath>

HUD::HUD(const sf::Font& font,unsigned int seed) 
    : seedText(font,"Track Seed: "+std::to_string(seed),16),
      lapText(font,"Lap: 1",24),
      timeText(font,"Current: 0:00.00",18),
      bestTimeText(font,"Best:--",18),
      cdText(font,"",80),
      currLapTime(0.f), 
      bestLapTime(0.f), 
      startCountdown(2.99f)
{

    hudBg.setSize(sf::Vector2f(200.f,115.f));
    hudBg.setPosition(sf::Vector2f(10.f,10.f));
    hudBg.setFillColor(sf::Color(10,5,15,220));
    hudBg.setOutlineThickness(2.f);
    hudBg.setOutlineColor(sf::Color(0,255,255,100));

    seedText.setFillColor(sf::Color::White);
    seedText.setPosition(sf::Vector2f(20.f,15.f));
    lapText.setFillColor(sf::Color(0,255,255));
    lapText.setPosition(sf::Vector2f(20.f,35.f));
    timeText.setFillColor(sf::Color::White);
    timeText.setPosition(sf::Vector2f(20.f,70.f));
    bestTimeText.setFillColor(sf::Color(255,215,0));
    bestTimeText.setPosition(sf::Vector2f(20.f,95.f));

    mapBg.setSize(sf::Vector2f(200.f,200.f));
    mapBg.setPosition(sf::Vector2f(800.f-210.f,10.f));
    mapBg.setFillColor(sf::Color(0,0,0,200));
    mapBg.setOutlineThickness(2.f);
    mapBg.setOutlineColor(sf::Color(100,100,100));

    mapCamera.setCenter(sf::Vector2f(COLS*TILE/2.f,ROWS*TILE/2.f));
    mapCamera.setSize(sf::Vector2f(COLS*TILE*0.18f,ROWS*TILE*0.18f));
    mapCamera.setViewport(sf::FloatRect(sf::Vector2f(0.7375f,0.016f),sf::Vector2f(0.25f,0.33f)));
    minimapCar.setRadius(TILE*2.f);
    minimapCar.setFillColor(sf::Color::Red);
    minimapCar.setOrigin(sf::Vector2f(TILE*2.f,TILE*2.f));
}

void HUD::reset(){
    currLapTime=0.f;
    bestLapTime=0.f;
    startCountdown=2.99f;
    lapText.setString("Lap: 1");
    bestTimeText.setString("Best:--");
}

void HUD::updateTimers(float dt){
    if (startCountdown>-1.f) startCountdown-=dt;
    if (startCountdown<=0.f) currLapTime+=dt;
}

void HUD::recordLap(int currLap){
    if (bestLapTime==0.f||currLapTime<bestLapTime){
        bestLapTime=currLapTime;
    }
    currLapTime=0.f;
    lapText.setString("Lap: "+std::to_string(currLap+1));
}

void HUD::updateMinimap(sf::Vector2f carPos,const std::vector<sf::Vector2f>& waypoints){
    minimapCar.setPosition(carPos);
    if(minimapTrack.getVertexCount()!=waypoints.size()){
        minimapTrack.setPrimitiveType(sf::PrimitiveType::LineStrip);
        minimapTrack.resize(waypoints.size());
        for(size_t i=0;i<waypoints.size();i++) {
            minimapTrack[i].position=waypoints[i];
            minimapTrack[i].color=sf::Color(0,255,255,150);
        }
    }
}

void HUD::draw(sf::RenderWindow& window) {
    char timeBuffer[32];
    int mins=static_cast<int>(currLapTime)/60;
    float secs=currLapTime-(mins*60);
    snprintf(timeBuffer,sizeof(timeBuffer),"Current: %d:%05.2f",mins,secs);
    timeText.setString(timeBuffer);

    if (bestLapTime>0.f) {
        int bMins=static_cast<int>(bestLapTime)/60;
        float bSecs=bestLapTime-(bMins*60);
        snprintf(timeBuffer,sizeof(timeBuffer),"Best: %d:%05.2f",bMins,bSecs);
        bestTimeText.setString(timeBuffer);
    }

    window.setView(window.getDefaultView());
    window.draw(hudBg);
    window.draw(seedText);
    window.draw(lapText);
    window.draw(timeText);
    window.draw(bestTimeText);

    if(startCountdown>-1.f) {
    if(startCountdown>0.f){
        float currentSecondFraction=startCountdown-std::floor(startCountdown);
        if(currentSecondFraction==0.f&&startCountdown>0.f){
            currentSecondFraction=1.f;
        }

        if(startCountdown>1.f){
            cdText.setString(std::to_string(static_cast<int>(std::ceil(startCountdown-1.f))));
            cdText.setFillColor(sf::Color(0,255,255));
            cdText.setOutlineColor(sf::Color(20,10,50,150));
        }else{
            cdText.setString("GO!");
            cdText.setFillColor(sf::Color(0,255,128));
            cdText.setOutlineColor(sf::Color(0,50,20,150));
        }

        float progress=1.0f-currentSecondFraction;
        float scaleMod=1.0f+(progress*progress*1.8f);
        cdText.setScale(sf::Vector2f(scaleMod,scaleMod));

        std::uint8_t alphaFade=static_cast<std::uint8_t>(255.f*(1.0f-progress));
        
        sf::Color coreColor=cdText.getFillColor();
        sf::Color edgeColor=cdText.getOutlineColor();
        
        coreColor.a=alphaFade;
        edgeColor.a=static_cast<std::uint8_t>(alphaFade*0.6f);
        cdText.setFillColor(coreColor);
        cdText.setOutlineColor(edgeColor);

        sf::FloatRect cb=cdText.getLocalBounds();
        cdText.setOrigin(sf::Vector2f(cb.position.x+cb.size.x/2.f,cb.position.y+cb.size.y/2.f));
        cdText.setPosition(sf::Vector2f(400.f,250.f));

        window.draw(cdText);
    }
}
    window.draw(mapBg);
    window.setView(mapCamera);
    window.draw(minimapTrack);
    window.draw(minimapCar);
}