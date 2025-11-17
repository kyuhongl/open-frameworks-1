#include "ofApp.h"

void ofApp::setup() {
    ofSetFrameRate(60);
    ofEnableAlphaBlending();
    ofEnableSmoothing();
    ofSetCircleResolution(32);
    ofFill();
    ofDisableArbTex();
    
    // Setup webcam with better error handling
    camWidth = 640;
    camHeight = 480;
    webcamInitialized = false;
    
    // List available devices
    vector<ofVideoDevice> devices = webcam.listDevices();
    
    if (devices.size() > 0) {
        ofLogNotice("ofApp") << "Found " << devices.size() << " video device(s):";
        for (size_t i = 0; i < devices.size(); i++) {
            ofLogNotice("ofApp") << "  [" << i << "] " << devices[i].deviceName;
        }
        
        // Try to initialize the webcam
        webcam.setDesiredFrameRate(30);
        webcam.setDeviceID(0);
        
        if (webcam.initGrabber(camWidth, camHeight)) {
            webcam.setUseTexture(true);
            webcamInitialized = true;
            ofLogNotice("ofApp") << "Webcam initialized successfully at " << camWidth << "x" << camHeight;
        } else {
            ofLogError("ofApp") << "Failed to initialize webcam";
        }
    } else {
        ofLogError("ofApp") << "No video devices found";
    }
    
    // Allocate OpenCV images
    colorImg.allocate(camWidth, camHeight);
    grayImage.allocate(camWidth, camHeight);
    grayBlurred.allocate(camWidth, camHeight);
    backgroundImg.allocate(camWidth, camHeight);
    
    // Initialize gesture and mode
    currentGesture = NONE;
    previousGesture = NONE;
    currentMode = NORMAL;
    modeBlend = 0.0f;
    modeTransitionSpeed = 3.0f;
    cubeRotationAngle = 0.0f;
    useGestureDetection = true;  // Enabled by default with improved detection
    
    // Initialize gesture smoothing
    gestureHistorySize = 5;  // Smooth over 5 frames
    gestureHistory.clear();
    
    // Initialize background learning
    learningBackground = false;
    backgroundFrameCount = 0;
    
    // Create particles
    numParticles = 20000;
    particles.resize(numParticles);
    for (int i = 0; i < numParticles; i++) {
        particles[i] = Particle(0, 0, ofGetWidth(), ofGetHeight());
    }
    
    // Generate cube formation points
    generateCubePoints();
    
    // Setup FBO for particles
    particleFbo.allocate(ofGetWidth(), ofGetHeight(), GL_RGBA);
    
    // Load glow shader
    glowShader.load("shaders/glow");
    
    // Setup GUI
    gui.setup("Controls");
    gui.add(gestureThreshold.set("Gesture Threshold", 80.0f, 30.0f, 255.0f));
    gui.add(blurAmount.set("Blur Amount", 5, 1, 15));
    gui.add(minContourArea.set("Min Contour Area", 5000, 1000, 20000));
    gui.add(maxContourArea.set("Max Contour Area", (camWidth * camHeight) / 2, 10000, camWidth * camHeight));
    gui.add(useBackgroundSubtraction.set("Use Background Sub", false));
    gui.add(spinStrength.set("Spin Strength", 0.5f, 0.0f, 2.0f));
    gui.add(chaoticJitter.set("Chaotic Jitter", 0.3f, 0.0f, 1.0f));
    gui.add(formationStrength.set("Formation Strength", 0.15f, 0.0f, 0.5f));
    gui.add(rotationSpeed.set("Rotation Speed", 0.5f, 0.0f, 2.0f));
    gui.add(showWebcam.set("Show Webcam", true));
    gui.add(showDebug.set("Show Debug", true));
    
    showGui = true;
    
    ofSetBackgroundColor(240, 234, 214);  // Eggshell white
}

void ofApp::update() {
    if (!webcamInitialized) return;
    
    webcam.update();
    
    if (webcam.isFrameNew()) {
        colorImg.setFromPixels(webcam.getPixels());
        colorImg.mirror(false, true);
        grayImage = colorImg;
        
        // Apply blur to reduce noise
        int blur = blurAmount.get();
        if (blur % 2 == 0) blur++;  // Blur amount must be odd
        grayBlurred = grayImage;
        grayBlurred.blur(blur);
        
        // Update background if learning
        if (learningBackground) {
            captureBackground();
        }
        
        if (useGestureDetection) {
            detectGesture();
            updateParticleMode();
        }
    }
    
    // Update all particles
    float deltaTime = ofGetLastFrameTime();
    applyModeToParticles(deltaTime);
    
    for (auto &p : particles) {
        p.update(deltaTime);
    }
}

void ofApp::detectGesture() {
    ofxCvGrayscaleImage processedImage;
    processedImage.allocate(camWidth, camHeight);
    
    // Use blurred image for better noise handling
    processedImage = grayBlurred;
    
    // Apply background subtraction if enabled
    if (useBackgroundSubtraction && backgroundFrameCount > 30) {
        ofxCvGrayscaleImage diff;
        diff.allocate(camWidth, camHeight);
        diff.absDiff(processedImage, backgroundImg);
        processedImage = diff;
    }
    
    // Threshold the image
    processedImage.threshold(gestureThreshold.get());
    
    // Apply morphological operations to clean up the image
    processedImage.erode();
    processedImage.dilate();
    processedImage.dilate();
    processedImage.erode();
    
    // Find contours with improved parameters
    int minArea = minContourArea.get();
    int maxArea = maxContourArea.get();
    contourFinder.findContours(processedImage, minArea, maxArea, 10, false);
    
    if (contourFinder.nBlobs > 0) {
        // Find the largest blob (likely the hand)
        int largestIdx = 0;
        float largestArea = 0;
        for (int i = 0; i < contourFinder.nBlobs; i++) {
            if (contourFinder.blobs[i].area > largestArea) {
                largestArea = contourFinder.blobs[i].area;
                largestIdx = i;
            }
        }
        
        ofxCvBlob& blob = contourFinder.blobs[largestIdx];
        
        // Convert ofxCv points to cv::Point
        vector<cv::Point> contourPoints;
        for (size_t i = 0; i < blob.pts.size(); i++) {
            contourPoints.push_back(cv::Point(blob.pts[i].x, blob.pts[i].y));
        }
        
        if (contourPoints.size() < 5) {
            gestureHistory.push_back(NONE);
        } else {
            cv::Mat contourMat(contourPoints.size(), 1, CV_32SC2);
            for (size_t i = 0; i < contourPoints.size(); i++) {
                contourMat.at<cv::Point>(i) = contourPoints[i];
            }
            
            vector<int> hull;
            cv::convexHull(contourMat, hull, false, false);
            
            int defectCount = countConvexityDefects(contourPoints, hull);
            float aspectRatio = blob.boundingRect.height / (blob.boundingRect.width + 0.01f);
            
            // Calculate hull area for better solidity calculation
            vector<cv::Point> hullPoints;
            for (size_t i = 0; i < hull.size(); i++) {
                hullPoints.push_back(contourPoints[hull[i]]);
            }
            float hullArea = cv::contourArea(hullPoints);
            float solidity = blob.area / (hullArea + 0.01f);
            
            // Improved gesture classification
            GestureType detectedGesture = NONE;
            
            if (defectCount == 0 && solidity > 0.8f) {
                // Very compact, high solidity = FIST
                detectedGesture = FIST;
            } else if (defectCount >= 4) {
                // Many defects = fingers spread = OPEN_HAND
                detectedGesture = OPEN_HAND;
            } else if (defectCount >= 2 && defectCount <= 3 && solidity < 0.7f) {
                // Multiple defects but not too many = OPEN_HAND
                detectedGesture = OPEN_HAND;
            } else if (defectCount == 1 && aspectRatio > 1.3f) {
                // One defect + tall shape = THUMBS_UP
                detectedGesture = THUMBS_UP;
            } else if (defectCount <= 1 && solidity > 0.75f) {
                // Few defects, high solidity = FIST
                detectedGesture = FIST;
            } else {
                // Ambiguous - keep previous gesture
                if (gestureHistory.size() > 0) {
                    detectedGesture = gestureHistory.back();
                } else {
                    detectedGesture = NONE;
                }
            }
            
            gestureHistory.push_back(detectedGesture);
        }
    } else {
        gestureHistory.push_back(NONE);
    }
    
    // Maintain history size
    if (gestureHistory.size() > gestureHistorySize) {
        gestureHistory.erase(gestureHistory.begin());
    }
    
    // Apply temporal smoothing
    currentGesture = getSmoothedGesture();
}

int ofApp::countConvexityDefects(const vector<cv::Point>& contour, const vector<int>& hull) {
    if (hull.size() < 3 || contour.size() < 3) return 0;
    
    vector<cv::Vec4i> defects;
    try {
        cv::Mat contourMat(contour.size(), 1, CV_32SC2);
        for (size_t i = 0; i < contour.size(); i++) {
            contourMat.at<cv::Point>(i) = contour[i];
        }
        cv::convexityDefects(contourMat, hull, defects);
    } catch (...) {
        return 0;
    }
    
    int significantDefects = 0;
    for (const auto& defect : defects) {
        float depth = defect[3] / 256.0f;
        if (depth > 15.0f) {  // Slightly lower threshold for better finger detection
            significantDefects++;
        }
    }
    
    return significantDefects;
}

GestureType ofApp::getSmoothedGesture() {
    if (gestureHistory.empty()) return NONE;
    
    // Count occurrences of each gesture type in history
    map<GestureType, int> counts;
    for (const auto& gesture : gestureHistory) {
        counts[gesture]++;
    }
    
    // Find the most common gesture
    GestureType mostCommon = NONE;
    int maxCount = 0;
    for (const auto& pair : counts) {
        if (pair.second > maxCount) {
            maxCount = pair.second;
            mostCommon = pair.first;
        }
    }
    
    // Only change if we have strong consensus (at least 60% of frames)
    if (maxCount >= gestureHistorySize * 0.6f) {
        return mostCommon;
    }
    
    // Otherwise keep the current gesture to avoid flickering
    return currentGesture;
}

void ofApp::captureBackground() {
    // Capture current frame as background
    // User should remove their hand before pressing 'b'
    backgroundImg = grayBlurred;
    backgroundFrameCount = 30;  // Mark as complete
    learningBackground = false;  // Auto-stop after capture
    ofLogNotice("ofApp") << "Background captured!";
}

void ofApp::updateParticleMode() {
    ParticleMode targetMode = NORMAL;
    
    switch (currentGesture) {
        case FIST:
            targetMode = CHAOTIC_SPIN;
            break;
        case OPEN_HAND:
            targetMode = CALM;
            break;
        case THUMBS_UP:
            targetMode = FORM_CUBE;
            break;
        default:
            targetMode = NORMAL;
            break;
    }
    
    if (targetMode != currentMode) {
        currentMode = targetMode;
        modeBlend = 0.0f;
    } else {
        modeBlend = ofClamp(modeBlend + modeTransitionSpeed * ofGetLastFrameTime(), 0.0f, 1.0f);
    }
}

void ofApp::applyModeToParticles(float dt) {
    glm::vec2 center(ofGetWidth() / 2, ofGetHeight() / 2);
    float time = ofGetElapsedTimef();
    
    switch (currentMode) {
        case CHAOTIC_SPIN: {
            for (auto &p : particles) {
                glm::vec2 toCenter = p.pos - center;
                float dist = glm::length(toCenter);
                
                if (dist > 0.01f) {
                    glm::vec2 perpendicular(-toCenter.y, toCenter.x);
                    perpendicular = glm::normalize(perpendicular);
                    glm::vec2 spinForce = perpendicular * spinStrength.get() * modeBlend;
                    p.applyForce(spinForce);
                }
                
                glm::vec2 jitterForce(
                    ofSignedNoise(p.pos.x * 0.02f + time * 5.0f),
                    ofSignedNoise(p.pos.y * 0.02f + time * 5.0f)
                );
                p.applyForce(jitterForce * chaoticJitter.get() * modeBlend);
                
                p.currentJitter = ofLerp(p.jitterAmount, p.jitterAmount * 2.0f, modeBlend);
            }
            break;
        }
        
        case CALM: {
            for (auto &p : particles) {
                p.vel *= ofLerp(1.0f, 0.90f, modeBlend);
                p.currentJitter = ofLerp(p.jitterAmount, 0.0f, modeBlend);
                
                glm::vec2 drift(
                    ofSignedNoise(p.pos.x * 0.01f + time * 0.5f) * 0.02f,
                    ofSignedNoise(p.pos.y * 0.01f + time * 0.5f) * 0.02f
                );
                p.applyForce(drift * modeBlend);
            }
            break;
        }
        
        case FORM_CUBE: {
            cubeRotationAngle += rotationSpeed.get() * dt;
            
            for (size_t i = 0; i < particles.size(); i++) {
                int cubeIdx = i % cubePoints.size();
                
                glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), cubeRotationAngle, glm::vec3(1, 1, 0));
                glm::vec3 rotated = glm::vec3(rotation * glm::vec4(cubePoints[cubeIdx], 1.0f));
                glm::vec2 target = project2D(rotated);
                
                particles[i].cubeTarget = target;
                
                glm::vec2 dir = target - particles[i].pos;
                particles[i].applyForce(dir * formationStrength.get() * modeBlend);
                particles[i].vel *= ofLerp(1.0f, 0.92f, modeBlend);
            }
            break;
        }
        
        default:
            for (auto &p : particles) {
                p.currentJitter = p.jitterAmount;
            }
            break;
    }
}

void ofApp::generateCubePoints() {
    cubePoints.clear();
    float cubeSize = 200.0f;
    int gridRes = 15;
    
    for (int i = 0; i < gridRes; i++) {
        for (int j = 0; j < gridRes; j++) {
            float u = (float)i / (gridRes - 1) - 0.5f;
            float v = (float)j / (gridRes - 1) - 0.5f;
            
            cubePoints.push_back(glm::vec3(u * cubeSize, v * cubeSize, -cubeSize * 0.5f));
            cubePoints.push_back(glm::vec3(u * cubeSize, v * cubeSize, cubeSize * 0.5f));
            cubePoints.push_back(glm::vec3(u * cubeSize, -cubeSize * 0.5f, v * cubeSize));
            cubePoints.push_back(glm::vec3(u * cubeSize, cubeSize * 0.5f, v * cubeSize));
            cubePoints.push_back(glm::vec3(-cubeSize * 0.5f, u * cubeSize, v * cubeSize));
            cubePoints.push_back(glm::vec3(cubeSize * 0.5f, u * cubeSize, v * cubeSize));
        }
    }
}

glm::vec2 ofApp::project2D(const glm::vec3& p3) {
    float scale = 800.0f / (800.0f + p3.z);
    glm::vec2 p2d(p3.x * scale, p3.y * scale);
    p2d += glm::vec2(ofGetWidth() / 2, ofGetHeight() / 2);
    return p2d;
}

void ofApp::draw() {
    ofBackground(240, 234, 214);  // Eggshell white
    
    // Draw particles with normal alpha blending for ink-on-paper effect
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    
    for (auto &p : particles) {
        p.draw();
    }
    
    ofDisableBlendMode();
    
    // Debug: Show webcam and processing pipeline
    if (showWebcam && webcamInitialized) {
        float scale = 0.25f;
        int xPos = 10;
        int yPos = 10;
        int imgWidth = camWidth * scale;
        int imgHeight = camHeight * scale;
        int spacing = 5;
        
        // Draw color image
        ofSetColor(255);
        colorImg.draw(xPos, yPos, imgWidth, imgHeight);
        ofDrawBitmapString("Color", xPos + 5, yPos + 15);
        
        // Draw processed/thresholded image
        yPos += imgHeight + spacing;
        
        // Recreate the processed image to show what detectGesture sees
        ofxCvGrayscaleImage processedImage;
        processedImage.allocate(camWidth, camHeight);
        processedImage = grayBlurred;
        
        if (useBackgroundSubtraction && backgroundFrameCount > 30) {
            ofxCvGrayscaleImage diff;
            diff.allocate(camWidth, camHeight);
            diff.absDiff(processedImage, backgroundImg);
            processedImage = diff;
        }
        
        processedImage.threshold(gestureThreshold.get());
        processedImage.draw(xPos, yPos, imgWidth, imgHeight);
        ofDrawBitmapString("Processed", xPos + 5, yPos + 15);
        
        // Draw contours on top
        ofPushMatrix();
        ofTranslate(xPos, yPos);
        ofScale(scale, scale);
        
        ofSetColor(0, 255, 0);
        ofNoFill();
        ofSetLineWidth(2);
        
        for (int i = 0; i < contourFinder.nBlobs; i++) {
            contourFinder.blobs[i].draw(0, 0);
            
            // Draw bounding box for largest blob
            if (i == 0) {
                ofSetColor(255, 0, 0);
                ofDrawRectangle(contourFinder.blobs[i].boundingRect);
                ofSetColor(0, 255, 0);
            }
        }
        
        ofSetLineWidth(1);
        ofFill();
        ofPopMatrix();
        
        // Show background if learning
        if (learningBackground && backgroundFrameCount > 0) {
            yPos += imgHeight + spacing;
            ofSetColor(255);
            backgroundImg.draw(xPos, yPos, imgWidth, imgHeight);
            ofDrawBitmapString("Background", xPos + 5, yPos + 15);
        }
    }
    
    // Draw GUI
    if (showGui) {
        gui.draw();
    }
    
    // Draw debug info
    if (showDebug && showGui) {
        ofSetColor(255);
        stringstream ss;
        ss << "FPS: " << ofToString(ofGetFrameRate(), 0) << endl;
        ss << "Particles: " << particles.size() << endl;
        ss << endl;
        
        // Webcam status
        if (!webcamInitialized) {
            ss << "WARNING: WEBCAM NOT INITIALIZED!" << endl;
            ss << "Check permissions and camera connection" << endl;
        } else {
            ss << "Webcam: OK" << endl;
        }
        ss << endl;
        
        ss << "CONTROL MODE: " << (useGestureDetection ? "GESTURE" : "KEYBOARD") << endl;
        
        if (useGestureDetection && webcamInitialized) {
            ss << "Blobs detected: " << contourFinder.nBlobs << endl;
            
            if (contourFinder.nBlobs > 0) {
                float largestArea = 0;
                for (int i = 0; i < contourFinder.nBlobs; i++) {
                    if (contourFinder.blobs[i].area > largestArea) {
                        largestArea = contourFinder.blobs[i].area;
                    }
                }
                ss << "Largest blob area: " << (int)largestArea << endl;
            }
            
            ss << "Background subtraction: " << (useBackgroundSubtraction ? "ON" : "OFF") << endl;
            if (useBackgroundSubtraction) {
                ss << "Background frames: " << backgroundFrameCount << endl;
            }
        }
        
        ss << endl;
        ss << "DETECTED GESTURE: ";
        switch (currentGesture) {
            case FIST: ss << "FIST"; break;
            case OPEN_HAND: ss << "OPEN HAND"; break;
            case THUMBS_UP: ss << "THUMBS UP"; break;
            default: ss << "NONE"; break;
        }
        ss << endl;
        
        ss << "CURRENT MODE: ";
        switch (currentMode) {
            case CHAOTIC_SPIN: ss << "CHAOTIC SPIN"; break;
            case CALM: ss << "CALM"; break;
            case FORM_CUBE: ss << "FORM CUBE"; break;
            default: ss << "NORMAL"; break;
        }
        ss << endl;
        ss << "Mode Blend: " << ofToString(modeBlend, 2) << endl;
        ss << endl;
        
        ss << "CONTROLS:" << endl;
        ss << "[1-4] Manual gestures" << endl;
        ss << "[b] Capture background" << endl;
        ss << "[c] Toggle webcam" << endl;
        ss << "[d] Toggle debug" << endl;
        ss << "[g] Toggle gesture mode" << endl;
        ss << "[s] Save screenshot" << endl;
        ss << "[TAB] Toggle GUI" << endl;
        
        ofDrawBitmapString(ss.str(), ofGetWidth() - 320, 20);
    }
}

void ofApp::keyPressed(int key) {
    if (key == OF_KEY_TAB) {
        showGui = !showGui;
        ofLogNotice("ofApp") << "GUI " << (showGui ? "SHOWN" : "HIDDEN");
    }
    if (key == 's') {
        // Save screenshot
        string timestamp = ofGetTimestampString();
        string filename = "screenshot_" + timestamp + ".png";
        ofSaveScreen(filename);
        ofLogNotice("ofApp") << "Screenshot saved: " << filename;
    }
    if (key == 'b') {
        // Start/stop background learning
        learningBackground = !learningBackground;
        if (learningBackground) {
            backgroundFrameCount = 0;
            ofLogNotice("ofApp") << "Started background capture";
        } else {
            ofLogNotice("ofApp") << "Stopped background capture with " << backgroundFrameCount << " frames";
        }
    }
    if (key == 'c') {
        showWebcam = !showWebcam;
    }
    if (key == 'd') {
        showDebug = !showDebug;
    }
    if (key == 'g') {
        useGestureDetection = !useGestureDetection;
        if (useGestureDetection) {
            ofLogNotice("ofApp") << "Gesture detection ENABLED";
        } else {
            ofLogNotice("ofApp") << "Gesture detection DISABLED - using keyboard control";
        }
    }
    
    // Manual mode control with number keys
    if (key == '1') {
        currentGesture = FIST;
        updateParticleMode();
    }
    if (key == '2') {
        currentGesture = OPEN_HAND;
        updateParticleMode();
    }
    if (key == '3') {
        currentGesture = THUMBS_UP;
        updateParticleMode();
    }
    if (key == '4' || key == '0') {
        currentGesture = NONE;
        updateParticleMode();
    }
}

