#include "track.h"
#include "constants.h"
#include "physics.h"
#include<cstdlib>
#include<ctime>
#include<algorithm>

std::vector<std::vector<int>>trackMap;
std::deque<TireMark>tireMarks;
std::vector<sf::Vector2f>trackWaypoints;
unsigned int currentSeed=0;

void generateTrack(unsigned int seed){
    trackMap.assign(ROWS,std::vector<int>(COLS,1));
    trackWaypoints.clear();
    
    if(seed>0) currentSeed=seed;
    else currentSeed=std::rand()%900000+100000;
    std::srand(currentSeed);

    float centerX=COLS*TILE/2.f;
    float centerY=ROWS*TILE/2.f;

    const int POINTS=24;
    float angleStep=2.f*PI/POINTS;

    std::vector<sf::Vector2f>basePoints;
    int currentCurveType=1;
    
    for(int i=0;i<POINTS;i++){
        float angleNoise=((std::rand()%100)/100.f-0.5f)*(angleStep*0.7f);
        float angle=i*angleStep+angleNoise;
        
        if(std::rand()%100<40) currentCurveType=std::rand()%3;

        float radius;
        if(currentCurveType==0)      radius=(30.f+std::rand()%10)*TILE;
        else if(currentCurveType==1) radius=(45.f+std::rand()%15)*TILE;
        else                         radius=(55.f+std::rand()%15)*TILE;

        sf::Vector2f pt(centerX+std::cos(angle)*radius,centerY+std::sin(angle)*radius);
        pt.x=std::max(TILE*15.f,std::min(pt.x,(COLS-15)*TILE*1.f));
        pt.y=std::max(TILE*15.f,std::min(pt.y,(ROWS-15)*TILE*1.f));
        basePoints.push_back(pt);
    }

    for(size_t i=0;i<basePoints.size();i++){
        sf::Vector2f A=basePoints[i];
        sf::Vector2f B=basePoints[(i+1)%basePoints.size()];

        trackWaypoints.push_back(A);

        if(std::rand()%100<35){
            sf::Vector2f seg=B-A;
            float len=vecLength(seg);
            sf::Vector2f perp(-seg.y/len,seg.x/len);

            float mag=len*(0.15f+(std::rand()%15)/100.f);
            float sign=(std::rand()%2==0) ?1.f:-1.f;

            sf::Vector2f p1=A+seg*0.33f+perp*(mag*sign);
            sf::Vector2f p2=A+seg*0.66f+perp*(-mag*sign);

            auto clampPt=[](sf::Vector2f p){
                p.x=std::max(TILE*15.f,std::min(p.x,(COLS-15)*TILE*1.f));
                p.y=std::max(TILE*15.f,std::min(p.y,(ROWS-15)*TILE*1.f));
                return p;
            };
            trackWaypoints.push_back(clampPt(p1));
            trackWaypoints.push_back(clampPt(p2));
        }
    }
    trackWaypoints.push_back(trackWaypoints[0]);

    std::vector<float>waypointWidths(trackWaypoints.size());
    for (size_t i=0;i<trackWaypoints.size();i++) {
        size_t prev=(i==0) ? trackWaypoints.size()-2:i-1;
        size_t next=(i+1)%trackWaypoints.size();

        sf::Vector2f v1=trackWaypoints[i]-trackWaypoints[prev];
        sf::Vector2f v2=trackWaypoints[next]-trackWaypoints[i];

        float len1=vecLength(v1);
        float len2=vecLength(v2);
        float turnSharpness=0.f;
        if (len1>0&&len2>0){
            float dot=(v1.x*v2.x+v1.y*v2.y)/(len1*len2);
            turnSharpness=1.f-dot;
        }
        float chokeMultiplier=1.0f;
        if (i>0&&std::rand()%100<15){
            chokeMultiplier=0.45f;
        }

        waypointWidths[i]=((1.5f+(std::rand() % 10)/10.f+(turnSharpness*1.5f))*2.f)*chokeMultiplier;
    }
    waypointWidths.back()=waypointWidths.front();

    int currentMaterial=0;
    for (size_t k=0;k+1<trackWaypoints.size();++k) {
        sf::Vector2f startP=trackWaypoints[k];
        sf::Vector2f endP=trackWaypoints[k+1];
        sf::Vector2f segVec=endP-startP;
        float distance=vecLength(segVec);

        float widthStart=waypointWidths[k];
        float widthEnd=waypointWidths[k+1];

        for (float d=0.f;d<distance;d+=1.5f) {
            float t=d/distance;
            
            float currentBrushRadius=widthStart+(widthEnd-widthStart)*t;
            int brushSize=static_cast<int>(std::ceil(currentBrushRadius));

            int centerC=static_cast<int>((startP.x+segVec.x*t)/TILE);
            int centerR=static_cast<int>((startP.y+segVec.y*t)/TILE);

            if (k>0 && static_cast<int>(d) % 150==0) {
                int roll=std::rand() % 100;
                currentMaterial=(roll<15) ? 3 : ((roll<35) ? 2 : 0);
            }

            int paintMat=currentMaterial;
            
            for (int i=-brushSize;i<=brushSize;++i) {
                for (int j=-brushSize;j<=brushSize;++j) {
                    if (i*i+j*j<=currentBrushRadius*currentBrushRadius) {
                        int r=centerR+i, c=centerC+j;
                        if (r>=0 && r<ROWS && c>=1 && c<COLS-1) {
                            trackMap[r][c]=paintMat;
                        }
                    }
                }
            }
        }
    }

    if (trackWaypoints.size()>1) {
        sf::Vector2f startP=trackWaypoints[0];
        sf::Vector2f nextP=trackWaypoints[1];
        
        sf::Vector2f dir=nextP-startP;
        float dirLen=std::sqrt(dir.x*dir.x+dir.y*dir.y);
        if (dirLen>0.f) dir/=dirLen;
        
        sf::Vector2f perp(-dir.y, dir.x);
        float trackWidthPixels=waypointWidths[0]*TILE;
        
        int gridX=0;
        for (float offset=-trackWidthPixels*0.5f;offset<=trackWidthPixels*0.5f;offset+=TILE*0.5f) {
            sf::Vector2f linePoint=startP+perp*offset;
            gridX++;

            for (int thickness=-1;thickness<=1;++thickness) {
                sf::Vector2f thickPoint=linePoint+dir*(static_cast<float>(thickness)*static_cast<float>(TILE));
                
                int finalR=static_cast<int>(thickPoint.y/TILE);
                int finalC=static_cast<int>(thickPoint.x/TILE);
                
                if (finalR>=0 && finalR<ROWS && finalC>=0 && finalC<COLS) {
                    if (trackMap[finalR][finalC]==0 || trackMap[finalR][finalC]==2 || trackMap[finalR][finalC]==3) {
                        if ((gridX+(thickness+1)) % 2==0) {
                            trackMap[finalR][finalC]=4;
                        } else {
                            trackMap[finalR][finalC]=7;
                        }
                    }
                }
            }
        }
    }

    for (size_t k=1;k<trackWaypoints.size()-1;++k) {
        if (waypointWidths[k]>5.5f && std::rand() % 100<15) { 
            sf::Vector2f startP=trackWaypoints[k];
            sf::Vector2f endP=trackWaypoints[k+1];
            sf::Vector2f segVec=endP-startP;
            float len=vecLength(segVec);
            if (len<20.f) continue;

            sf::Vector2f perp(-segVec.y/len, segVec.x/len);
            float offsetDir=(std::rand() % 2==0) ? 1.f :-1.f;
            float offsetMag=waypointWidths[k]*0.4f*TILE;
            
            sf::Vector2f obsPos=startP+(segVec*0.5f)+(perp*(offsetMag*offsetDir));
            
            int centerC=static_cast<int>(obsPos.x/TILE);
            int centerR=static_cast<int>(obsPos.y/TILE);
            
            for (int i=-1;i<=1;++i) {
                for (int j=-1;j<=1;++j) {
                    if (centerR+i>=0 && centerR+i<ROWS && centerC+j>=0 && centerC+j<COLS) {
                        trackMap[centerR+i][centerC+j]=1;
                    }
                }
            }
        }
    }

    for (size_t k=2;k<trackWaypoints.size()-2;++k) {
        sf::Vector2f startP=trackWaypoints[k];
        sf::Vector2f endP=trackWaypoints[k+1];
        float len=vecLength(endP-startP);
        
        if (len>45.f && std::rand() % 100<5) {
            sf::Vector2f dir=(endP-startP)/len;
            sf::Vector2f perp(-dir.y, dir.x);
            
            float width=waypointWidths[k]*TILE;
            float offset=(std::rand() % 2==0) ? (width*0.45f) : (-width*0.45f);
            
            sf::Vector2f padCenter=startP+dir*(len*0.5f)+perp*offset;
            int cR=static_cast<int>(padCenter.y/TILE);
            int cC=static_cast<int>(padCenter.x/TILE);
            
            for(int i=-1;i<=1;++i) {
                for(int j=-1;j<=1;++j) {
                    if (cR+i>=0 && cR+i<ROWS && cC+j>=0 && cC+j<COLS) {
                        if (trackMap[cR+i][cC+j]==0) { 
                            trackMap[cR+i][cC+j]=6;
                        }
                    }
                }
            }
        }
    }
}