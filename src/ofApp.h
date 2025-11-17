#pragma once

#include "ofMain.h"
#include "ofxOpenCv.h"
#include "ofxGui.h"
#include "Particle.h"

enum GestureType { NONE, FIST, OPEN_HAND, THUMBS_UP };
enum ParticleMode { NORMAL, CHAOTIC_SPIN, CALM, FORM_CUBE };

class ofApp : public ofBaseApp {
public:
    void setup();
    void update();
    void draw();
    void keyPressed(int key);
    
    // Webcam and OpenCV
    ofVideoGrabber webcam;
    ofxCvColorImage colorImg;
    ofxCvGrayscaleImage grayImage;
    ofxCvGrayscaleImage grayBlurred;
    ofxCvGrayscaleImage backgroundImg;
    ofxCvContourFinder contourFinder;
    
    int camWidth;
    int camHeight;
    bool webcamInitialized;
    
    // Gesture detection
    GestureType currentGesture;
    GestureType previousGesture;
    bool useGestureDetection;
    void detectGesture();
    int countConvexityDefects(const vector<cv::Point>& contour, const vector<int>& hull);
    
    // Gesture smoothing
    vector<GestureType> gestureHistory;
    int gestureHistorySize;
    GestureType getSmoothedGesture();
    
    // Background learning
    bool learningBackground;
    int backgroundFrameCount;
    void captureBackground();
    
    // Particle modes
    ParticleMode currentMode;
    float modeBlend;
    float modeTransitionSpeed;
    void updateParticleMode();
    void applyModeToParticles(float dt);
    
    // Cube formation
    vector<glm::vec3> cubePoints;
    float cubeRotationAngle;
    void generateCubePoints();
    glm::vec2 project2D(const glm::vec3& p3);
    
    // Particles
    vector<Particle> particles;
    int numParticles;
    
    // Shader for glow effect
    ofShader glowShader;
    ofFbo particleFbo;
    
    // GUI
    ofxPanel gui;
    ofParameter<float> gestureThreshold;
    ofParameter<int> blurAmount;
    ofParameter<int> minContourArea;
    ofParameter<int> maxContourArea;
    ofParameter<bool> useBackgroundSubtraction;
    ofParameter<float> spinStrength;
    ofParameter<float> chaoticJitter;
    ofParameter<float> formationStrength;
    ofParameter<float> rotationSpeed;
    ofParameter<bool> showWebcam;
    ofParameter<bool> showDebug;
    
    bool showGui;
};

