/*
 *  BlobsManager.h
 *  A manager for openCV blobs. By remembering blobs and comparing them 
 *  it will identify blobs, even though they are moving. 
 *  It's basicly a lot of things you need to do when working with blobs. 
 *  
 *  Created by Peter Uithoven on 5/6/11.
 */

#ifndef _ofxBlobsManager
#define _ofxBlobsManager

#include "ofMain.h"
#include "ofxStoredBlobVO.h"
#include "ofxOpenCv.h"

class ofxBlobsManager
{
public:

	//the closest stored blob that are withing this distance, will be merged into new found blob 
	ofParameterGroup params;
    
    ofParameter<int> maxMergeDis;
    void mergeDisCallback(int & mergeDistanceChanged);
	// by what percentage the blob movement will be normalized
	ofParameter<float> normalizePercentage;
    void normalizePercentageCallback( float & normalisePercentageChanged);

	// enable a filter for blobs that show up shorter than minDetectedTime
	ofParameter<bool> enableMinDetectedTimeFilter;
    void useMinDetectedTimeFilterCallack(bool & useMinDetectedTimeChanged);
	// the minimum time a blob has to be detected before the blob manager will add the blob 
	// (last time detected - first time detection) 
	// not necessarily continuously (as long as undetected doesn't cross maxUndetectedTime)  
	ofParameter<int> minDetectedTime;
    void minDetectedTimeChangeCalback(int & minDetectedTimeChanged);
	
	// enable the storage of blobs that disapeared shorter than maxUndetectedTime
	ofParameter<bool> enableUndetectedBlobs;
    void enableUndetectedBlobsCallback(bool & enableUndetectedBlobsChanged);
	// the maximum time a blob can go undetected before the blob manager will remove the blob
	ofParameter<int> maxUndetectedTime;
    void maxundetectedTimeChangeCallback(int & maxUndetectedTimeChanged);
	
	// normally it will every new blob a sequentially higher id, 
	// but with giveLowestPossibleIDs enabled it will try to give the lowest id's available. 
	ofParameter<bool> giveLowestPossibleIDs;
    void giveLowestIDChangedCallback(bool & giveLowestIDChanged);
	// the max amount of blobs when doing non sequential id's
	ofParameter<int> maxNumBlobs;
    void maxNumberBLobsChangedCallback(int & maxNumBlobsChanged);
	
	// also debug draw the candidate blobs
	ofParameter<bool> debugDrawCandidates;
    void debugDrawCandidatesChangedCallback(bool & debugDrawCandidatesChanged);

    // the final resulting blobs
    vector<ofxStoredBlobVO> blobs;

    // possible new blobs
    vector<ofxStoredBlobVO> candidateBlobs;
    
    
	ofxBlobsManager();
	void update(vector<ofxCvBlob>& newCVBlobs);
	void debugDraw(int baseX, int baseY, int inputWidth, int inputHeight, int displayWidth, int displayHeight);
	bool hasBlob(int blobID);
	void removeBlob(ofxStoredBlobVO& targetBlob, vector<ofxStoredBlobVO>& blobs);
	
private:
	int sequentialID;
	int sequentialCandidateID;
	
	vector<ofxStoredBlobVO*> findCloseBlobs(ofxCvBlob& newBlob,vector<ofxStoredBlobVO>& blobs);
};

#endif
