#pragma once

#include "ofMain.h"

#include "ofxCv.h"
#include "ofxOpenCv.h"
#include "ofxBlobsManager.h"
#include "ofxGui.h"

//#define _USE_LIVE_VIDEO        // uncomment this to use a live camera
                                // otherwise, we'll use a movie file

class ofApp : public ofBaseApp{

    public:
        void setup();
        void update();
        void draw();
        
        void keyPressed(int key);
        void keyReleased(int key);
        void mouseMoved(int x, int y );
        void mouseDragged(int x, int y, int button);
        void mousePressed(int x, int y, int button);
        void mouseReleased(int x, int y, int button);
        void windowResized(int w, int h);
        void dragEvent(ofDragInfo dragInfo);
        void gotMessage(ofMessage msg);

        #ifdef _USE_LIVE_VIDEO
          ofVideoGrabber         vidGrabber;
        #else
          ofVideoPlayer         vidPlayer;
        #endif

        ofxCv::ContourFinder contourFinder;
        
        ofxCv::TrackingColorMode trackingColorMode;
        ofColor targetColor;
        
        ofxBlobsManager        blobsManager;
    
        ofParameterGroup openCVParameters;
        ofParameter<int>                 threshold;
        ofParameter<bool>                bLearnBakground;
        ofParameter<int> blur;
        ofParameter<int> minArea;
        ofParameter<int> maxArea;
        ofParameter<int> nConsidered;
        ofParameter<bool> bFindHoles;
        ofParameter<bool> bUseApproximation;
        ofParameter<bool> trackHs;

        ofxPanel panel;
        float width, height;
        float drawWidth, drawHeight;

    bool paused;
};

