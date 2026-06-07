#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <vector>

#include "car.h"
#include "constants.h"
#include "fx.h"
#include "hud.h"
#include "physics.h"
#include "track.h"

enum class GameState { MainMenu,Racing };
enum class MenuFocus { None,TypingSeed,BrowsingFavorites,HowToPlay };

std::map<unsigned int,std::vector<float>>trackLeaderboards;
float lastLapTime=0.f;

void loadLeaderboards() {
    std::ifstream inFile("leaderboards.txt");
    unsigned int seed;
    float t1,t2,t3,t4,t5;
    while (inFile>>seed>>t1>>t2>>t3>>t4>>t5) {
        std::vector<float>times;
        if (t1>0.f) times.push_back(t1);
        if (t2>0.f) times.push_back(t2);
        if (t3>0.f) times.push_back(t3);
        if (t4>0.f) times.push_back(t4);
        if (t5>0.f) times.push_back(t5);
        trackLeaderboards[seed]=times;
    }
    inFile.close();
}

void saveLeaderboards() {
    std::ofstream outFile("leaderboards.txt");
    for (const auto& pair:trackLeaderboards) {
        outFile<<pair.first;
        for (size_t i=0;i<5;i++) {
            if (i<pair.second.size())
                outFile<<" "<<pair.second[i];
            else
                outFile<<" 0.0";
        }
        outFile<<"\n";
    }
    outFile.close();
}

void addTimeToLeaderboard(unsigned int seed,float time) {
    auto& times=trackLeaderboards[seed];
    times.push_back(time);
    std::sort(times.begin(),times.end());
    if (times.size()>5) times.resize(5);
    saveLeaderboards();
}

std::string formatTimeStr(float t) {
    int mins=static_cast<int>(t)/60;
    float secs=t-(mins*60);
    std::stringstream ss;
    ss<<mins<<":"<<std::setfill('0')<<std::setw(2)<<static_cast<int>(secs)<<"."<<std::setw(2)<<static_cast<int>((secs-static_cast<int>(secs))*100);
    return ss.str();
}

int main() {
    generateTrack(0);

    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(800,600)),"Midnight Drift");
    window.setFramerateLimit(60);
    sf::View camera(sf::Vector2f(0.f,0.f),sf::Vector2f(400.f,300.f));
    std::vector<std::vector<float>>tileGlow(ROWS,std::vector<float>(COLS,0.f));

    GameState gameState=GameState::MainMenu;
    MenuFocus menuFocus=MenuFocus::None;
    float menuTimer=0.f;
    std::string typedSeedBuffer="";

    std::vector<int>favoriteSeeds;
    std::size_t selectedFavoriteIndex=0;

    std::ifstream inFile("favorites.txt");
    int fav;
    while (inFile>>fav) {
        favoriteSeeds.push_back(fav);
    }
    inFile.close();
    loadLeaderboards();

    sf::Font mainFont;
    if (!mainFont.openFromFile("../bin/arial.ttf")) {
    }
    sf::Font titleFont;
    if (!titleFont.openFromFile("../bin/JOKERMAN.ttf")) {
        titleFont=mainFont;
    }

    sf::Text titleText(titleFont,"MIDNIGHT DRIFT",64);
    titleText.setFillColor(sf::Color(0,255,255));
    titleText.setOutlineThickness(4.f);
    titleText.setOutlineColor(sf::Color(60,30,80));
    sf::FloatRect tb=titleText.getLocalBounds();
    titleText.setOrigin(sf::Vector2f(tb.position.x+tb.size.x/2.f,tb.position.y+tb.size.y/2.f));
    titleText.setPosition(sf::Vector2f(400.f,130.f));

    sf::Text startText(mainFont,"Press ENTER to start Race",24);
    startText.setFillColor(sf::Color::White);
    sf::FloatRect sb=startText.getLocalBounds();
    startText.setOrigin(sf::Vector2f(sb.position.x+sb.size.x/2.f,
                                     sb.position.y+sb.size.y/2.f));
    startText.setPosition(sf::Vector2f(400.f,260.f));

    sf::Text seedInputText(mainFont,"Press [S] to type custom seed:",20);
    seedInputText.setFillColor(sf::Color(200,200,200));
    seedInputText.setPosition(sf::Vector2f(100.f,330.f));

    sf::Text favoritesText(mainFont,
                           "Press [F] to browse Favorite Tracks\nPress [M] while "
                           "racing to Save Track Seed",
                           18);
    favoritesText.setFillColor(sf::Color(255,105,195));
    favoritesText.setPosition(sf::Vector2f(100.f,410.f));

    sf::Vector2f position=trackWaypoints[0];
    sf::Vector2f toNext=trackWaypoints[1]-trackWaypoints[0];
    float angle=std::atan2(toNext.y,toNext.x)*(180.f/PI);
    sf::Vector2f velocity(0.f,0.f);
    float engineSpeed=0.f;

    int currentLap=0;
    int currentCheckpoint=1;

    CarVisuals myCar;
    struct FrameData {
        sf::Vector2f pos;
        float angle;
    };
    std::vector<FrameData>currLapData;
    std::vector<FrameData>bestLapData;
    float currLapTimer=0.f;
    float bestLapTime=99999.f;
    float recordPopupTimer=0.f;
    std::size_t ghostFrameIndex=0;

    float slowMoEnergy=100.f;
    float ghostFrameAccumulator=0.f;

    CarVisuals ghostCar;
    ghostCar.makeGhost();

    HUD myHud(mainFont,currentSeed);

    FXManager fx;
    float shakeIntensity=0.f;
    float boostTimer=0.f;
    sf::Vector2f cameraPos=position;
    float currentZoom=400.f;

    const int NUM_RAYS=180;
    const float CONE_ANGLE=70.f;
    const float MAX_LIGHT_DIST=150.f;
    sf::VertexArray leftLight(sf::PrimitiveType::TriangleFan,NUM_RAYS+2);
    sf::VertexArray rightLight(sf::PrimitiveType::TriangleFan,NUM_RAYS+2);

    sf::RectangleShape darkness(sf::Vector2f(2000.f,1500.f));
    darkness.setFillColor(sf::Color(10,5,15,180));

    sf::RectangleShape tileShape(sf::Vector2f(TILE,TILE));
    tileShape.setOutlineThickness(-1.f);

    sf::RectangleShape markShape(sf::Vector2f(8.f,3.f));
    markShape.setOrigin(sf::Vector2f(4.f,1.5f));

    float skidTimer=0.f;
    sf::Clock clock;

    auto launchGameWithSeed=[&](int targetedSeed) {
        gameState=GameState::Racing;
        menuFocus=MenuFocus::None;
        generateTrack(targetedSeed);
        myHud.reset();
        myHud.seedText.setString("Track Seed:"+std::to_string(currentSeed));
        tireMarks.clear();
        tileGlow.assign(ROWS,std::vector<float>(COLS,0.f));
        position=trackWaypoints[0];
        sf::Vector2f toNextR=trackWaypoints[1]-trackWaypoints[0];
        angle=std::atan2(toNextR.y,toNextR.x)*(180.f/PI);
        velocity=sf::Vector2f(0.f,0.f);
        engineSpeed=0.f;
        currentLap=0;
        currentCheckpoint=1;
        currLapData.clear();
        bestLapData.clear();
        currLapTimer=0.f;
        bestLapTime=99999.f;
        ghostFrameIndex=0;
        slowMoEnergy=100.f;
        ghostFrameAccumulator=0.f;
        cameraPos=position;
        currentZoom=300.f;
        recordPopupTimer=0.f;
    };

    while (window.isOpen()) {
        float rawDt=std::min(clock.restart().asSeconds(),0.05f);
        menuTimer+=rawDt;
        float timeScale=1.0f;
        if (recordPopupTimer>0.f) recordPopupTimer-=rawDt;

        if (gameState==GameState::Racing&&myHud.startCountdown<=0.f) {
            bool slowMoTriggered=
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);

            if (slowMoTriggered&&slowMoEnergy>0.f) {
                timeScale=0.35f;
                slowMoEnergy-=rawDt*33.f;
            } else {
                slowMoEnergy=std::min(100.f,slowMoEnergy+rawDt*15.f);
            }
        }

        float dt=rawDt*timeScale;

        while (const std::optional event=window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();

            if (const auto*textEvent=event->getIf<sf::Event::TextEntered>()) {
                if (gameState==GameState::MainMenu&&
                    menuFocus==MenuFocus::TypingSeed) {
                    if (textEvent->unicode==8) {
                        if (!typedSeedBuffer.empty())
                            typedSeedBuffer.pop_back();
                    } else if (textEvent->unicode>=48&&
                               textEvent->unicode<=57&&
                               typedSeedBuffer.size()<10) {
                        typedSeedBuffer+=static_cast<char>(textEvent->unicode);
                    }
                }
            }

            if (const auto*key=event->getIf<sf::Event::KeyPressed>()) {
                if (gameState==GameState::MainMenu) {
                    if (key->code==sf::Keyboard::Key::Escape) {
                        menuFocus=MenuFocus::None;
                    } else if (menuFocus==MenuFocus::None) {
                        if (key->code==sf::Keyboard::Key::Enter) {
                            launchGameWithSeed(0);
                        } else if (key->code==sf::Keyboard::Key::S) {
                            menuFocus=MenuFocus::TypingSeed;
                            typedSeedBuffer="";
                        } else if (key->code==sf::Keyboard::Key::F&&
                                   !favoriteSeeds.empty()) {
                            menuFocus=MenuFocus::BrowsingFavorites;
                            selectedFavoriteIndex=0;
                        } else if (key->code==sf::Keyboard::Key::H) {
                            menuFocus=MenuFocus::HowToPlay;
                        }
                    } else if (menuFocus==MenuFocus::TypingSeed) {
                        if (key->code==sf::Keyboard::Key::Enter&&
                            !typedSeedBuffer.empty()) {
                            launchGameWithSeed(
                                static_cast<int>(std::stoll(typedSeedBuffer)));
                        }
                    } else if (menuFocus==MenuFocus::BrowsingFavorites) {
                        if (key->code==sf::Keyboard::Key::Up&&
                            selectedFavoriteIndex>0) {
                            selectedFavoriteIndex--;
                        }
                        if (key->code==sf::Keyboard::Key::Down&&
                            selectedFavoriteIndex<favoriteSeeds.size()-1) {
                            selectedFavoriteIndex++;
                        }
                        if (key->code==sf::Keyboard::Key::Enter) {
                            launchGameWithSeed(
                                favoriteSeeds[selectedFavoriteIndex]);
                        }
                    }
                } else if (gameState==GameState::Racing) {
                    if (key->code==sf::Keyboard::Key::R) {
                        launchGameWithSeed(currentSeed);
                        slowMoEnergy=100.f;
                        ghostFrameAccumulator=0.f;
                    }
                    if (key->code==sf::Keyboard::Key::Escape) {
                        gameState=GameState::MainMenu;
                        menuFocus=MenuFocus::None;
                    }
                    if (key->code==sf::Keyboard::Key::M) {
                        if (std::find(favoriteSeeds.begin(),
                                      favoriteSeeds.end(),
                                      currentSeed)==favoriteSeeds.end()) {
                            favoriteSeeds.push_back(currentSeed);
                            std::ofstream outFile("favorites.txt",
                                                  std::ios::app);
                            outFile<<currentSeed<<"\n";
                            outFile.close();
                            window.setTitle("Seed Saved!");
                        }
                    }
                }
            }
        }

        if (gameState==GameState::Racing) {
            myHud.updateTimers(dt);

            if (myHud.startCountdown<=0.f) {
                currLapTimer+=dt;
                currLapData.push_back({position,angle});

                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)||
                    sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
                    angle-=180.f*dt;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)||
                    sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
                    angle+=180.f*dt;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)||
                    sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
                    engineSpeed+=400.f*dt;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)||
                    sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
                    engineSpeed-=400.f*dt;

                int currentTerrain=getTerrain(position.x,position.y);
                if (currentTerrain==6) {
                    boostTimer=0.3f;
                }

                float maxEngineSpeed=500.f;
                if (boostTimer>0.f) {
                    boostTimer-=dt;
                    maxEngineSpeed=500.f;
                    engineSpeed+=600.f*dt;

                    float rads=angle*(PI/180.f);
                    sf::Vector2f heading(std::cos(rads),std::sin(rads));
                    fx.Boost(position-heading*8.f,rads);
                }

                engineSpeed=
                    std::max(-300.f,std::min(engineSpeed,maxEngineSpeed));
                engineSpeed*=std::pow(0.95f,dt*60.f);
            }

            float rads=angle*(PI/180.f);
            sf::Vector2f heading(std::cos(rads),std::sin(rads));
            sf::Vector2f lateral(-heading.y,heading.x);

            if (myHud.startCountdown<=0.f) {
                int currentTerrain=getTerrain(position.x,position.y);
                float forwardGrip=(currentTerrain==3) ? 0.02f:0.1f;
                float lateralGrip=(currentTerrain==2)? 0.600f:((currentTerrain==3) ? 0.005f:0.030f);

                float fwdVel=velocity.x*heading.x+velocity.y*heading.y;
                float latVel=velocity.x*lateral.x+lateral.y*velocity.y;

                fwdVel+=(engineSpeed-fwdVel)*forwardGrip;
                latVel+=(0.f-latVel)*lateralGrip;
                velocity=heading*fwdVel+lateral*latVel;

                sf::Vector2f nextPos=position+(velocity*dt);

                float preCrashSpeed=vecLength(velocity);
                bool hardCrash=false;
                float bounciness=-0.4f;

                if (getTerrain(nextPos.x,position.y)==1) {
                    velocity.x*=bounciness;
                    nextPos.x=position.x;
                    hardCrash=true;
                }
                if (getTerrain(position.x,nextPos.y)==1) {
                    velocity.y*=bounciness;
                    nextPos.y=position.y;
                    hardCrash=true;
                }
                position=nextPos;

                if (hardCrash) {
                    engineSpeed*=bounciness;
                    if (preCrashSpeed>60.f) {
                        shakeIntensity=preCrashSpeed*0.08f;
                        fx.Sparks(position,preCrashSpeed);
                    }
                }
            }

            float distToCheckpoint=vecLength(trackWaypoints[currentCheckpoint]-position);
            if (distToCheckpoint<TILE*15.f) {
                currentCheckpoint+=3;
                if (currentCheckpoint>=static_cast<int>(trackWaypoints.size())) {
                    currentLap++;
                    currentCheckpoint=currentCheckpoint % trackWaypoints.size();
                    if (currentCheckpoint==0) currentCheckpoint=3;

                    lastLapTime=currLapTimer;
                    addTimeToLeaderboard(currentSeed,currLapTimer);

                    if (currLapTimer<bestLapTime) {
                        if (bestLapTime<99999.f) {
                            recordPopupTimer=3.0f;
                        }
                        bestLapTime=currLapTimer;
                        bestLapData=currLapData;
                    }

                    currLapData.clear();
                    currLapTimer=0.f;
                    ghostFrameIndex=0;
                    myHud.recordLap(currentLap);
                    window.setTitle("Midnight Drift-Lap:"+
                                    std::to_string(currentLap+1));
                }
            }

            int cR=static_cast<int>(position.y/TILE);
            int cC=static_cast<int>(position.x/TILE);
            int glowRadius=5;
            for (int i=-glowRadius;i<=glowRadius;i++) {
                for (int j=-glowRadius;j<=glowRadius;j++) {
                    if (cR+i>=0&&cR+i<ROWS&&cC+j>=0&&cC+j<COLS) {
                        float dist=std::sqrt(i*i+j*j);
                        if (dist<=glowRadius) {
                            float intensity=1.0f-(dist/glowRadius);
                            tileGlow[cR+i][cC+j]=
                                std::max(tileGlow[cR+i][cC+j],intensity);
                        }
                    }
                }
            }

            float speed=vecLength(velocity);
            float slip=(speed>20.f)? 
            (1.f-std::abs((velocity.x/speed)*heading.x+(velocity.y/speed)*heading.y)):0.f;

            skidTimer+=dt;
            if (slip>0.2f&&speed>40.f&&
                getTerrain(position.x,position.y)==0&&
                skidTimer>=0.025f) {
                skidTimer=0.f;
                sf::Vector2f perp(std::cos(rads+PI/2.f),
                                  std::sin(rads+PI/2.f));
                tireMarks.push_back({position-heading*10.f+perp*5.f,200.f,angle});
                tireMarks.push_back({position-heading*10.f-perp*5.f,200.f,angle});
                if (tireMarks.size()>MAX_TIRE_MARKS) {
                    tireMarks.pop_front();
                    tireMarks.pop_front();
                }
            }

            for (auto& m:tireMarks) m.alpha-=dt*15.f;
            while (!tireMarks.empty()&&tireMarks.front().alpha<=0.f)
                tireMarks.pop_front();

            sf::Vector2f leftOrigin=position+heading*6.f+lateral*2.5f;
            sf::Vector2f rightOrigin=position+heading*6.f-lateral*2.5f;
            leftLight[0].position=leftOrigin;
            leftLight[0].color=sf::Color(255,255,255,50);
            rightLight[0].position=rightOrigin;
            rightLight[0].color=sf::Color(255,255,255,50);
            float startAngle=angle-(CONE_ANGLE/2.f);
            float angleStep=CONE_ANGLE/NUM_RAYS;

            for (int i=0;i<=NUM_RAYS;i++) {
                float rayAngleRads=(startAngle+i*angleStep)*(PI/180.f);
                sf::Vector2f rayDir(std::cos(rayAngleRads),std::sin(rayAngleRads));
                auto castRay=[&](sf::Vector2f origin,sf::VertexArray& lightArr) {
                    float dist=0.f;
                    while(dist<MAX_LIGHT_DIST&&
                        getTerrain(origin.x+rayDir.x*dist,origin.y+rayDir.y*dist)!=1) dist+=5.f;
                    dist=std::min(dist,MAX_LIGHT_DIST);
                    lightArr[i+1].position=origin+rayDir*dist;
                    lightArr[i+1].color=
                        sf::Color(255,255,255,static_cast<std::uint8_t>(50.f*(1.f-dist/MAX_LIGHT_DIST)));
                };
                castRay(leftOrigin,leftLight);
                castRay(rightOrigin,rightLight);
            }
        }

        sf::Vector2f shakeOffset(0.f,0.f);

        if (gameState==GameState::Racing) {
            float speed=vecLength(velocity);

            sf::Vector2f targetCamPos=position+(velocity*0.35f);
            cameraPos=lerp(cameraPos,targetCamPos,5.f*dt);

            float targetZoom=400.f+(speed*0.9f);
            if (boostTimer>0.f) targetZoom+=350.f;

            currentZoom=lerp(currentZoom,targetZoom,3.5f*dt);
            camera.setSize(sf::Vector2f(currentZoom,currentZoom*0.75f));

            if (shakeIntensity>0.f) {
                shakeOffset.x=((std::rand() % 100)/100.f-0.5f)*shakeIntensity;
                shakeOffset.y=((std::rand() % 100)/100.f-0.5f)*shakeIntensity;
                shakeIntensity-=dt*60.f;
                if (shakeIntensity<0.f) shakeIntensity=0.f;
            }
        } else {
            float panRadius=TILE*50.f;
            cameraPos.x=(COLS*TILE/2.f)+std::cos(menuTimer*0.15f)*panRadius;
            cameraPos.y=(ROWS*TILE/2.f)+std::sin(menuTimer*0.20f)*panRadius;
            camera.setSize(sf::Vector2f(600.f,450.f));
        }

        camera.setCenter(cameraPos+shakeOffset);
        window.setView(camera);
        window.clear(sf::Color::Black);

        int startR=std::max(0,(int)(cameraPos.y-600.f)/TILE);
        int endR=std::min(ROWS,(int)(cameraPos.y+600.f)/TILE+1);
        int startC=std::max(0,(int)(cameraPos.x-800.f)/TILE);
        int endC=std::min(COLS,(int)(cameraPos.x+800.f)/TILE+1);

        for (int r=startR;r<endR;++r) {
            for (int c=startC;c<endC;++c) {
                int t=trackMap[r][c];
                if (t==1) continue;
                sf::Color baseColor,edgeColor=sf::Color(25,15,30);
                if (t==2)
                    baseColor=sf::Color(60,30,80);
                else if (t==3)
                    baseColor=sf::Color(30,40,70);
                else if (t==4) {
                    float timeSec=clock.getElapsedTime().asSeconds();
                    float pulse=std::abs(std::sin(timeSec*6.f));

                    baseColor=sf::Color(255,255,255);
                    edgeColor=sf::Color(static_cast<std::uint8_t>(180+75*pulse),255,255);
                } else if (t==7) {
                    float timeSec=clock.getElapsedTime().asSeconds();
                    float pulse=std::abs(std::sin(timeSec*6.f));

                    baseColor=sf::Color(15,12,22);
                    edgeColor=sf::Color(static_cast<std::uint8_t>(130+125*pulse),10,
                        static_cast<std::uint8_t>(100+100*pulse));
                } else if (t==5) {
                    baseColor=sf::Color(40,40,40);
                    edgeColor=sf::Color(255,50,0);
                } else if (t==6) {
                    float timeSec=clock.getElapsedTime().asSeconds();
                    int scrollOffset=static_cast<int>(timeSec*40.f);
                    int pattern=((r+c-scrollOffset) % 4+4) % 4;
                    if (pattern==0) {
                        baseColor=sf::Color(0,255,255);
                        edgeColor=sf::Color(255,255,255);
                    } else if (pattern==1) {
                        baseColor=sf::Color(0,100,255);
                        edgeColor=sf::Color(0,150,255);
                    } else {
                        baseColor=sf::Color(10,5,20);
                        edgeColor=sf::Color(40,10,60);
                    }
                } else
                    baseColor=sf::Color(45,35,55);

                float glow=tileGlow[r][c];
                if (glow>0.f) {
                    tileGlow[r][c]=std::max(0.f,glow-dt*0.4f);
                    int baseR=255;int baseG=140;int baseB=210;
                    int edgeR=255;int edgeG=245;int edgeB=250;

                    baseColor.r=std::min(255,baseColor.r+static_cast<int>(baseR*glow));
                    baseColor.g=std::min(255,baseColor.g+static_cast<int>(baseG*glow));
                    baseColor.b=std::min(255,baseColor.b+static_cast<int>(baseB*glow));
                    edgeColor.r=std::min(255,edgeColor.r+static_cast<int>(edgeR*glow));
                    edgeColor.g=std::min(255,edgeColor.g+static_cast<int>(edgeG*glow));
                    edgeColor.b=std::min(255,edgeColor.b+static_cast<int>(edgeB*glow));
                }
                tileShape.setFillColor(baseColor);
                tileShape.setOutlineColor(edgeColor);
                tileShape.setPosition(sf::Vector2f(c*TILE,r*TILE));
                window.draw(tileShape);
            }
        }

        if (gameState==GameState::Racing) {
            sf::Vector2f cpPos=trackWaypoints[currentCheckpoint];
            int nextCP=(currentCheckpoint+4) % trackWaypoints.size();
            sf::Vector2f toNextCP=trackWaypoints[nextCP]-cpPos;
            float angleToNext=std::atan2(toNextCP.y,toNextCP.x);

            sf::ConvexShape chevron;
            chevron.setPointCount(4);
            chevron.setPoint(0,sf::Vector2f(-TILE*2.f,-TILE*4.f));
            chevron.setPoint(1,sf::Vector2f(TILE*2.f,0.f));
            chevron.setPoint(2,sf::Vector2f(-TILE*2.f,TILE*4.f));
            chevron.setPoint(3,sf::Vector2f(-TILE*0.5f,0.f));
            chevron.setOrigin(sf::Vector2f(0.f,0.f));
            chevron.setRotation(sf::degrees(angleToNext*(180.f/PI)));

            for (int i=0;i<3;i++) {
                chevron.setPosition(cpPos+
                    sf::Vector2f(std::cos(angleToNext)*(i*TILE*3.f),
                                 std::sin(angleToNext)*(i*TILE*3.f)));
                float pulse=std::abs(std::sin(clock.getElapsedTime().asSeconds()*5.f-(i*1.5f)));
                chevron.setFillColor(
                    sf::Color(0,static_cast<std::uint8_t>(255*pulse),
                              static_cast<std::uint8_t>(255*pulse),150));
                chevron.setOutlineThickness(2.f);
                chevron.setOutlineColor(sf::Color(
                    0,255,255,static_cast<std::uint8_t>(255*pulse)));
                window.draw(chevron);
            }

            for (const auto& m:tireMarks) {
                if (m.alpha>0.f) {
                    markShape.setFillColor(sf::Color(10,5,15,(std::uint8_t)std::min(m.alpha,180.f)));
                    markShape.setPosition(m.pos);
                    markShape.setRotation(sf::degrees(m.angle));
                    window.draw(markShape);
                }
            }

            if (!bestLapData.empty()&&myHud.startCountdown<=0.f) {
                if (ghostFrameIndex<bestLapData.size()) {
                    FrameData fd=bestLapData[ghostFrameIndex];
                    ghostCar.updateAndDraw(fd.pos,fd.angle,window);

                    ghostFrameAccumulator+=timeScale;
                    while (ghostFrameAccumulator>=1.0f&&ghostFrameIndex<bestLapData.size()) {
                        ghostFrameIndex++;
                        ghostFrameAccumulator-=1.0f;
                    }
                } else {
                    FrameData finalFrame=bestLapData.back();
                    ghostCar.updateAndDraw(finalFrame.pos,finalFrame.angle,window);
                }
            }

            myCar.updateAndDraw(position,angle,window);
            fx.updateAndDraw(dt,window);

            darkness.setPosition(cameraPos-sf::Vector2f(1000.f,750.f));
            window.draw(darkness);
            window.draw(leftLight,sf::BlendAdd);
            window.draw(rightLight,sf::BlendAdd);
        } else {
            darkness.setPosition(cameraPos-sf::Vector2f(1000.f,750.f));
            window.draw(darkness);
        }

        window.setView(window.getDefaultView());

        auto drawStylizedText=[&](sf::Text& text,sf::Color coreColor,
                                   sf::Color glowColor,sf::Vector2f pos) {
            text.setFillColor(glowColor);
            text.setOutlineColor(sf::Color(15,5,25,200));
            text.setPosition(pos+sf::Vector2f(2.f,3.f));
            window.draw(text);
            text.setFillColor(coreColor);
            text.setPosition(pos);
            window.draw(text);
        };

        if (gameState==GameState::Racing) {
            myHud.updateMinimap(position,trackWaypoints);
            myHud.draw(window);
            window.setView(window.getDefaultView());

            sf::Vector2f barPos(328.f,15.f);

            sf::RectangleShape moBg(sf::Vector2f(144.f,8.f));
            moBg.setPosition(barPos-sf::Vector2f(1.f,1.f));
            moBg.setFillColor(sf::Color(10,12,18,240));
            moBg.setOutlineThickness(1.f);
            moBg.setOutlineColor(sf::Color(0,255,255,40));
            window.draw(moBg);

            int totalSegments=20;
            int activeSegments=static_cast<int>(
                (std::max(0.f,slowMoEnergy)/100.f)*totalSegments);

            for (int i=0;i<totalSegments;i++) {
                sf::RectangleShape segment(sf::Vector2f(4.f,6.f));
                segment.setPosition(barPos+sf::Vector2f(i*7.f+2.f,0.f));

                if (i<activeSegments) {
                    if (slowMoEnergy<25.f) segment.setFillColor(sf::Color(220,40,60,180));
                    else segment.setFillColor(sf::Color(0,200,255,150));
                } else {
                    segment.setFillColor(sf::Color(20,24,32,255));
                }
                window.draw(segment);
            }
            if (recordPopupTimer>0.f) {
                std::uint8_t alpha=(recordPopupTimer<1.0f) ? 
                    static_cast<std::uint8_t>(recordPopupTimer*255.f):255;
                sf::Text recordText(titleFont,"NEW RECORD!",28);
                sf::FloatRect rb=recordText.getLocalBounds();
                recordText.setOrigin(sf::Vector2f(rb.position.x/2.f,rb.position.y/2.f));
                drawStylizedText(recordText,sf::Color(255,255,255,alpha),
                                 sf::Color(255,0,128,alpha),sf::Vector2f(400.f,90.f));
            }
        } else {
            sf::FloatRect tb=titleText.getLocalBounds();
            titleText.setOrigin(sf::Vector2f(tb.position.x+tb.size.x/2.f,tb.position.y+tb.size.y/2.f));
            drawStylizedText(titleText,sf::Color(0,255,255),sf::Color(255,0,128),sf::Vector2f(400.f,110.f));

            if (menuFocus==MenuFocus::None) {
                float pulse=std::abs(std::sin(menuTimer*3.f));

                startText.setString("::[ENTER] RACE RANDOM TRACK");
                sf::FloatRect sb=startText.getLocalBounds();
                startText.setOrigin(sf::Vector2f(sb.position.x+sb.size.x/2.f,sb.position.y+sb.size.y/2.f));
                drawStylizedText(
                    startText,
                    sf::Color(255,255,255,static_cast<std::uint8_t>(100+155*pulse)),
                        sf::Color(50,20,80),sf::Vector2f(400.f,220.f));

                seedInputText.setString("::[S] TYPE CUSTOM SEED");
                sf::FloatRect sib=seedInputText.getLocalBounds();
                seedInputText.setOrigin(
                    sf::Vector2f(sib.position.x+sib.size.x/2.f,
                                 sib.position.y+sib.size.y/2.f));
                drawStylizedText(seedInputText,sf::Color(200,200,200),sf::Color(40,20,60),sf::Vector2f(400.f,280.f));

                favoritesText.setString("::[F] BROWSE FAVORITE TRACKS");
                sf::FloatRect fab=favoritesText.getLocalBounds();
                favoritesText.setOrigin(
                    sf::Vector2f(fab.position.x+fab.size.x/2.f,fab.position.y+fab.size.y/2.f));
                drawStylizedText(favoritesText,sf::Color(255,105,195),
                                 sf::Color(80,10,50),sf::Vector2f(400.f,340.f));

                sf::Text helpPrompt(
                    mainFont,"::[H] HOW TO DRIFT & OPERATE SYSTEMS",20);
                sf::FloatRect hpb=helpPrompt.getLocalBounds();
                helpPrompt.setOrigin(
                    sf::Vector2f(hpb.position.x+hpb.size.x/2.f,hpb.position.y+hpb.size.y/2.f));
                drawStylizedText(helpPrompt,sf::Color(0,255,150),sf::Color(0,60,30),sf::Vector2f(400.f,400.f));
            } else if (menuFocus==MenuFocus::TypingSeed) {
                std::string cursor=(static_cast<int>(menuTimer*2.f) % 2==0) ? "_":"";
                seedInputText.setString(
                    "ENTER VIRTUAL SEED:"+typedSeedBuffer+cursor+
                    "\n\n[ENTER] TO REPLICATE WORLD  |  [ESC] CANCEL");
                sf::FloatRect sib=seedInputText.getLocalBounds();
                seedInputText.setOrigin(sf::Vector2f(sib.position.x+sib.size.x/2.f,sib.position.y+sib.size.y/2.f));
                drawStylizedText(seedInputText,sf::Color(0,255,255),sf::Color(0,80,150),sf::Vector2f(400.f,280.f));
            } else if (menuFocus==MenuFocus::BrowsingFavorites) {
                sf::Text listRender(mainFont,
                                    "---BROWSE SAVED TRACKS---\n\n",18);
                listRender.setPosition(sf::Vector2f(200.f,220.f));

                std::string listStr="---BROWSE SAVED TRACKS---\n\n";
                for (std::size_t i=0;i<favoriteSeeds.size();i++) {
                    if (i==selectedFavoriteIndex)
                        listStr+=">SYSTEM SEED:"+std::to_string(favoriteSeeds[i])+"<\n";
                    else
                        listStr+="     System Seed:"+std::to_string(favoriteSeeds[i])+"\n";
                }
                listStr+=
                    "\n[UP/DOWN] Nav  |  [ENTER] Deploy Track  |  [ESC] Exit";
                listRender.setString(listStr);
                drawStylizedText(listRender,sf::Color(255,0,255),sf::Color(40,0,50),listRender.getPosition());
            } else if (menuFocus==MenuFocus::HowToPlay) {
                sf::Text manualTitle(mainFont,"---CONTROL MANUAL---",22);
                manualTitle.setPosition(sf::Vector2f(150.f,180.f));
                drawStylizedText(manualTitle,sf::Color(0,255,150),sf::Color::Black,manualTitle.getPosition());

                sf::Text manualBody(
                    mainFont,
                    "  [W/ARROW UP]->ENGAGE THROTTLE/ACCELERATION\n"
                    "  [S/ARROW DOWN]->ENGAGE REVERSE/REAR BRAKE\n"
                    "  [A/D or LEFT/RIGHT]->ROTATE STEERING LEFT/RIGHT\n\n"
                    "---RACING PHYSICS & LOGIC---\n"
                    "*DRIFTING:Swing hard into corners at speed to slide.\n"
                    "*REACTION TRAIL:Driving over track tiles charges them into neon pink.\n"
                    "*NITROUS PADS:Run over cyan arrows to take your engine cap to HIGH speeds.\n"
                    "*GHOST CAR:Beat your lap record to deploy a clone rival.\n"
                    "*FAVOURITES:Press [M] during a race to save the track to your favourites.\n"
                    "*SLOW-MO:Hold [L-SHIFT] to SLOW time and navigate sharp corners.\n"
                    " Press [ESC] to Return to Root Command Menu",
                    14);
                manualBody.setPosition(sf::Vector2f(150.f,220.f));
                drawStylizedText(manualBody,sf::Color(230,230,250),sf::Color(30,20,50),manualBody.getPosition());
            }
        }
        window.display();
    }
    return 0;
}