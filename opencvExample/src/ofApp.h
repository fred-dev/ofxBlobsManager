#pragma once

#include "ofMain.h"

#include "ofxOpenCv.h"
#include "ofxBlobsManager.h"
#include "ofxGui.h"

//#define _USE_LIVE_VIDEO		// uncomment this to use a live camera
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
		  ofVideoGrabber 		vidGrabber;
		#else
		  ofVideoPlayer 		vidPlayer;
		#endif

        ofxCvColorImage			colorImg;

        ofxCvGrayscaleImage 	grayImage;
		ofxCvGrayscaleImage 	grayBg;
		ofxCvGrayscaleImage 	grayDiff;

        ofxCvContourFinder 	contourFinder;
		
		ofxBlobsManager		blobsManager;
	
		ofParameter<int> 				threshold;
		ofParameter<bool>				bLearnBakground;
    
        ofParameter<int> blur;
        ofParameter<int> minArea;
        ofParameter<int> maxArea;
        ofParameter<int> nConsidered;
        ofParameter<bool> bFindHoles;
        ofParameter<bool> bUseApproximation;
    
        ofxPanel panel;
        float width, height;
        float drawWidth, drawHeight;


};

