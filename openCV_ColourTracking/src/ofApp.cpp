#include "ofApp.h"

using namespace ofxCv;
using namespace cv;
//--------------------------------------------------------------
void ofApp::setup(){
    
    #ifdef _USE_LIVE_VIDEO
    vidGrabber.setDeviceID(0);
    vidGrabber.initGrabber(1280, 720);
    width = vidGrabber.getWidth();
    height = vidGrabber.getHeight();
    #else
        vidPlayer.load("Untitled 06-Apple ProRes 422 LT.mov");
        vidPlayer.play();
    width = vidPlayer.getWidth();
    height = vidPlayer.getHeight();
    #endif
    drawWidth = 400;
    
    drawHeight = drawWidth / width * height;

    

    bLearnBakground.set("learn background", true);
    threshold.set("threshold", 55, 0, 255);
    blur.set("blur", 5, 0, 255);
    minArea.set("minArea", 1, 1, width*height/3);
    maxArea.set("maxArea", width*height/3, 1, width*height/3);
    nConsidered.set("nConsidered", 10, 0, 1000);
    bFindHoles.set("bFindHoles", true);
    bUseApproximation.set("bUseApproximation", true);
    
    openCVParameters.add(bLearnBakground);
    openCVParameters.add(threshold);
    openCVParameters.add(blur);
    openCVParameters.add(minArea);
    openCVParameters.add(maxArea);
    openCVParameters.add(nConsidered);
    openCVParameters.add(bFindHoles);
    openCVParameters.add(bUseApproximation);
    openCVParameters.add(trackHs.set("Track Hue/Saturation", false));

    openCVParameters.setName("Open CV Parameters");
    
    panel.setup("Blobs manager", "settings.xml", 10, 100);
    panel.add(blobsManager.params);
    panel.add(openCVParameters);
    
    panel.loadFromFile("settings.xml");
    panel.setPosition(drawWidth * 2 + 50, 20);
    
    ofSetBackgroundColor(0, 0, 0);
    ofSetFrameRate(30);
    ofSetLogLevel(OF_LOG_VERBOSE);
    
    contourFinder.setMinAreaRadius(minArea);
    contourFinder.setMaxAreaRadius(maxArea);
    trackingColorMode = TRACK_COLOR_RGB;
}

//--------------------------------------------------------------
void ofApp::update(){
    

    bool bNewFrame = false;

    #ifdef _USE_LIVE_VIDEO
       vidGrabber.update();
       bNewFrame = vidGrabber.isFrameNew();
    #else
        vidPlayer.update();
        bNewFrame = vidPlayer.isFrameNew();
    #endif

    if (bNewFrame){
        contourFinder.setTargetColor(targetColor, trackHs ? TRACK_COLOR_HS : TRACK_COLOR_RGB);
        contourFinder.setThreshold(threshold);
        #ifdef _USE_LIVE_VIDEO
        contourFinder.findContours(vidGrabber);
        #else
        contourFinder.findContours(vidPlayer);
        #endif
    }
    vector<ofxCvBlob> blobsConvert;
    for (int i =0; i<contourFinder.getContours().size(); i++) {
        ofxCvBlob tempBlob;
        for (int j=0; j<contourFinder.getContours()[i].size(); j++) {
            tempBlob.pts.push_back(ofVec3f(contourFinder.getContours()[i][j].x, contourFinder.getContours()[i][j].y));
        }
        tempBlob.centroid.x = contourFinder.getCenter(i).x;
        tempBlob.centroid.y = contourFinder.getCenter(i).y;
        tempBlob.boundingRect.width = contourFinder.getBoundingRect(i).width;
        tempBlob.boundingRect.height = contourFinder.getBoundingRect(i).height;
        blobsConvert.push_back(tempBlob);
        
    }
    
    blobsManager.update(blobsConvert);
    
}

//--------------------------------------------------------------
void ofApp::draw(){
    
    ofPushStyle();
    ofSetColor(255, 0, 0);
    ofNoFill();
    ofDrawRectangle(20,20,drawWidth,drawHeight);
    ofPopStyle();
    ofSetColor(255, 255, 255);
    #ifdef _USE_LIVE_VIDEO
    vidGrabber.draw(20,20,drawWidth,drawHeight);
    #else
    vidPlayer.draw(20,20,drawWidth,drawHeight);
    #endif
    
    
    ofPushStyle();
    ofSetColor(255, 0, 0);
    ofNoFill();
    ofDrawRectangle(drawWidth + 40,20,drawWidth,drawHeight);
    ofPopStyle();
    ofSetColor(255, 255, 255);
    #ifdef _USE_LIVE_VIDEO
    vidGrabber.draw(drawWidth + 40,20,drawWidth,drawHeight);
    #else
    vidPlayer.draw(drawWidth + 40,20,drawWidth,drawHeight);
    #endif
    ofPushView();
    ofTranslate(20, 20);
    ofScale(drawWidth/width, drawHeight/height);
    contourFinder.draw();
    ofPopView();
    // then draw the contours we can draw each blob individually,
    // this is how to get access to them:
//    for (int i = 0; i < contourFinder.nBlobs; i++){
//        ofPushView();
//        ofPushStyle();
//        ofTranslate(drawWidth + 40, 20);
//        ofScale(drawWidth/width, drawWidth/width);
//        contourFinder.blobs[i].draw(0,0);
//        ofPopStyle();
//        ofPopView();
//    }

    // debug draw the filtered blobs
    blobsManager.debugDraw(drawWidth + 40 , drawHeight + 40, width, height, drawWidth, drawHeight);
    
    for(int i=0;i<blobsManager.blobs.size();i++)
    {
        ofPushView();
        ofTranslate(20, 20);
        ofxCvBlob blob = blobsManager.blobs.at(i);
        ofPushStyle();
        ofNoFill();
        ofSetColor(0,255,0);
        ofScale(drawWidth/width, drawWidth/width);
        ofDrawCircle(blob.centroid.x,blob.centroid.y,40);
        ofPopStyle();
        ofPopView();
    }
    
    
    for(int i=0;i<blobsManager.candidateBlobs.size();i++)
    {
        ofxCvBlob candidateBlob = blobsManager.candidateBlobs.at(i);
        ofPushView();
        ofTranslate(20, 20);
        ofPushStyle();
        ofNoFill();
        ofSetColor(255,0,0);
        ofScale(drawWidth/width, drawWidth/width);
        ofDrawCircle(candidateBlob.centroid.x,candidateBlob.centroid.y,40);
        ofPopStyle();
        ofPopView();
    }
    
    
    // finally, a report:
    
    ofSetColor(targetColor);
    char reportStr[1024];
    sprintf(reportStr, "bg subtraction and blob detection\npress ' ' to capture bg\nnum blobs found %i\nfps: %f", blobsManager.blobs.size(), ofGetFrameRate());
    ofDrawBitmapString(reportStr, 20, 600);

    panel.draw();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

    switch (key){
        case ' ':
             #ifdef _USE_LIVE_VIDEO
               #else
            paused =!paused;
            vidPlayer.setPaused(paused);
                #endif
            break;
    }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    
    #ifdef _USE_LIVE_VIDEO
    targetColor = vidGrabber.getPixelsRef().getColor(x, y);
    #else
    targetColor = vidPlayer.getPixels().getColor(ofMap(x, 20, drawWidth+20, 0, width), ofMap(y, 20, drawHeight+20, 0, height));
    #endif
    contourFinder.setTargetColor(targetColor, trackingColorMode);
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){

}
