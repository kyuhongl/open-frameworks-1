#include "Particle.h"

Particle::Particle() {
    windowWidth = ofGetWidth();
    windowHeight = ofGetHeight();
    
    pos.x = ofRandom(windowWidth);
    pos.y = ofRandom(windowHeight);
    
    vel.x = ofRandom(-0.5f, 0.5f);
    vel.y = ofRandom(-0.5f, 0.5f);
    
    acc = glm::vec2(0, 0);
    
    baseSize = 1.0f;
    size = baseSize;
    
    jitterAmount = ofRandom(0.05f, 0.15f);
    colorPhase = ofRandom(0.0f, 1.0f);
    
    noiseOffsetX = ofRandom(1000.0f);
    noiseOffsetY = ofRandom(1000.0f);
    
    cubeTarget = glm::vec2(0, 0);
    currentJitter = jitterAmount;
}

Particle::Particle(float x, float y, float w, float h) {
    windowWidth = w;
    windowHeight = h;
    
    pos.x = ofRandom(w);
    pos.y = ofRandom(h);
    
    vel.x = ofRandom(-0.5f, 0.5f);
    vel.y = ofRandom(-0.5f, 0.5f);
    
    acc = glm::vec2(0, 0);
    
    baseSize = 1.0f;
    size = baseSize;
    
    jitterAmount = ofRandom(0.05f, 0.15f);
    colorPhase = ofRandom(0.0f, 1.0f);
    
    noiseOffsetX = ofRandom(1000.0f);
    noiseOffsetY = ofRandom(1000.0f);
    
    cubeTarget = glm::vec2(0, 0);
    currentJitter = jitterAmount;
}

void Particle::update(float deltaTime) {
    float time = ofGetElapsedTimef();
    
    // Apply smooth jitter using signed noise (use currentJitter for mode-based control)
    float jitterX = ofSignedNoise(noiseOffsetX + time * 0.5f, pos.x * 0.01f) * currentJitter;
    float jitterY = ofSignedNoise(noiseOffsetY + time * 0.5f, pos.y * 0.01f) * currentJitter;
    
    acc += glm::vec2(jitterX, jitterY);
    
    // Update velocity with acceleration
    vel += acc;
    
    // Apply damping (less damping = more responsive)
    vel *= 0.92f;
    
    // Update position
    pos += vel;
    
    // Reset acceleration
    acc = glm::vec2(0, 0);
    
    // Wrap around screen edges
    wrapAround(windowWidth, windowHeight);
}

void Particle::draw() {
    // Navy bluish ink color with red increasing based on velocity
    float velMag = glm::length(vel);
    float alpha = 180.0f + velMag * 30.0f;
    alpha = ofClamp(alpha, 120, 255);
    
    // Map velocity to add red hue
    float redAmount = ofMap(velMag, 0.0f, 3.0f, 0.0f, 1.0f, true);
    
    // Base navy ink color (15, 25, 45) with red increasing as velocity increases
    float r = 15 + (redAmount * 100);  // Goes from 15 (dark) to 115 (red-tinted)
    float g = 25 - (redAmount * 10);   // Slightly decreases green
    float b = 45 - (redAmount * 20);   // Decreases blue as red increases
    
    ofFill();
    ofSetColor(r, g, b, alpha);
    ofDrawCircle(pos, size);
}

void Particle::applyForce(const glm::vec2 &force) {
    acc += force;
}

void Particle::wrapAround(float w, float h) {
    if (pos.x < 0) pos.x = w;
    if (pos.x > w) pos.x = 0;
    if (pos.y < 0) pos.y = h;
    if (pos.y > h) pos.y = 0;
}

