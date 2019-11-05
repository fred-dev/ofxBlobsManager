#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    
	#ifdef _USE_LIVE_VIDEO
    vidGrabber.setDeviceID(0);
    vidGrabber.initGrabber(1280, 720);
    width = vidGrabber.getWidth();
    height = vidGrabber.getHeight();
	#else
        vidPlayer.load("4 people slow-close-freestle.mov");
        vidPlayer.play();
    width = vidPlayer.getWidth();
    height = vidPlayer.getHeight();
	#endif
    drawWidth = 384;
    
    drawHeight = drawWidth / width * height;

    colorImg.allocate(width,height);
	grayImage.allocate(width,height);
	grayBg.allocate(width,height);
	grayDiff.allocate(width,height);

    bLearnBakground.set("learn background", true);
    threshold.set("threshold", 55, 0, 255);
    blur.set("blur", 5, 0, 255);
    minArea.set("minArea", 1, 1, width*height/3);
    maxArea.set("maxArea", width*height/3, 1, width*height/3);
    nConsidered.set("nConsidered", 10, 0, 1000);
    bFindHoles.set("bFindHoles", true);
    bUseApproximation.set("bUseApproximation", true);
    
    panel.setup("Blob manager", "settings.xml", 10, 100);
    panel.add(blobsManager.params);
    panel.add(bLearnBakground);
    panel.add(threshold);
    panel.add(blur);
    panel.add(minArea);
    panel.add(maxArea);
    panel.add(nConsidered);
    panel.add(bFindHoles);
    panel.add(bUseApproximation);
    panel.loadFromFile("settings.xml");
    panel.setPosition(drawWidth * 2 + 50, 20);
    
    ofSetBackgroundColor(0, 0, 0);
	ofSetFrameRate(30);
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

		#ifdef _USE_LIVE_VIDEO
            colorImg.setFromPixels(vidGrabber.getPixels().getData(), width,height);
	    #else
            colorImg.setFromPixels(vidPlayer.getPixels().getData(), width,height);
        #endif

        grayImage = colorImg;
		if (bLearnBakground == true){
			grayBg = grayImage;		// the = sign copys the pixels from grayImage into grayBg (operator overloading)
			bLearnBakground = false;
		}
        grayImage.blurGaussian(blur.get());
		// take the abs value of the difference between background and incoming and then threshold:
		grayDiff.absDiff(grayBg, grayImage);
		grayDiff.threshold(threshold);

		// find contours which are between the size of 20 pixels and 1/3 the w*h pixels.
		// also, find holes is set to true so we will get interior contours as well....
		contourFinder.findContours(grayDiff, minArea, maxArea, nConsidered, bFindHoles, bUseApproximation);	// find holes
	}
	
	blobsManager.update(contourFinder.blobs);
	
}

//--------------------------------------------------------------
void ofApp::draw(){
    
    ofPushStyle();
    ofSetColor(255, 0, 0);
    ofNoFill();
    ofDrawRectangle(20,20,drawWidth,drawHeight);
    ofPopStyle();
    ofSetColor(255, 255, 255);
	colorImg.draw(20,20,drawWidth,drawHeight);
    
    ofPushStyle();
    ofSetColor(255, 0, 0);
    ofNoFill();
    ofDrawRectangle(drawWidth + 40,20,drawWidth,drawHeight);
    ofPopStyle();
    ofSetColor(255, 255, 255);
	grayImage.draw(drawWidth + 40 ,20,drawWidth,drawHeight);
    
    ofPushStyle();
    ofSetColor(255, 0, 0);
    ofNoFill();
    ofDrawRectangle(20,drawHeight + 40 ,drawWidth,drawHeight);
    ofPopStyle();
    ofSetColor(255, 255, 255);
	grayBg.draw(20 ,drawHeight + 40 ,drawWidth,drawHeight);
    
    ofPushStyle();
    ofSetColor(255, 0, 0);
    ofNoFill();
    ofDrawRectangle(drawWidth + 40,drawHeight + 40,drawWidth,drawHeight);
    ofPopStyle();
    ofSetColor(255, 255, 255);
	grayDiff.draw(drawWidth + 40,drawHeight + 40,drawWidth,drawHeight);


	// then draw the contours we can draw each blob individually,
	// this is how to get access to them:
    for (int i = 0; i < contourFinder.nBlobs; i++){
        ofPushView();
        ofPushStyle();
        ofTranslate(drawWidth + 40, 20);
        ofScale(drawWidth/width, drawWidth/width);
        contourFinder.blobs[i].draw(0,0);
        ofPopStyle();
        ofPopView();
    }

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
    
	ofSetHexColor(0xffffff);
	char reportStr[1024];
	sprintf(reportStr, "bg subtraction and blob detection\npress ' ' to capture bg\nnum blobs found %i\nfps: %f", contourFinder.nBlobs, ofGetFrameRate());
	ofDrawBitmapString(reportStr, 20, 600);

    panel.draw();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

	switch (key){
		case ' ':
			bLearnBakground = true;
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
