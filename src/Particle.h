#pragma once

#include "ofMain.h"

class Particle {
public:
    Particle();
    Particle(float x, float y, float w, float h);
    
    void update(float deltaTime);
    void draw();
    void applyForce(const glm::vec2 &force);
    void wrapAround(float w, float h);
    
    glm::vec2 pos;
    glm::vec2 vel;
    glm::vec2 acc;
    
    float size;
    float baseSize;
    float jitterAmount;
    float colorPhase;
    
    glm::vec2 cubeTarget;
    float currentJitter;
    
private:
    float windowWidth;
    float windowHeight;
    float noiseOffsetX;
    float noiseOffsetY;
};

