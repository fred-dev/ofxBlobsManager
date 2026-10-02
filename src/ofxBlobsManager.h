/*
 *  BlobsManager.h
 *  A manager for openCV blobs. By remembering blobs and comparing them
 *  it will identify blobs, even though they are moving.
 *  It's basicly a lot of things you need to do when working with blobs.
 *
 *  Created by Peter Uithoven on 5/6/11.
 *
 *  2026: every blob has a velocity and is matched against where it is
 *  expected to be, one detected blob per stored blob. Blobs that go missing
 *  keep moving along their velocity until maxUndetectedTime, so they can be
 *  picked up again. When blobs touch and the contour finder sees one big blob
 *  they are kept apart as "merged" blobs, and when they separate again they
 *  are given back their ids from their velocities.
 */

#ifndef _ofxBlobsManager
#define _ofxBlobsManager

#include "ofMain.h"
#include "ofxStoredBlobVO.h"
#include "ofxOpenCv.h"

class ofxBlobsManager
{
public:

	//the closest stored blob that are withing this distance (in pixels) of where it
	//is expected to be, will be merged into new found blob
	ofParameterGroup params;

    ofParameter<int> maxMergeDis;
    void mergeDisCallback(int & mergeDistanceChanged);
	// how much of each new position is used, 0-1 (1 = no smoothing)
	ofParameter<float> normalizePercentage;
    void normalizePercentageCallback( float & normalisePercentageChanged);
	// how quickly the velocity follows the movement, 0-1 (1 = no smoothing)
	ofParameter<float> velocitySmoothing;

	// enable a filter for blobs that show up shorter than minDetectedTime
	ofParameter<bool> enableMinDetectedTimeFilter;
    void useMinDetectedTimeFilterCallack(bool & useMinDetectedTimeChanged);
	// the minimum time a blob has to be detected before the blob manager will add the blob
	// (last time detected - first time detection)
	// not necessarily continuously (as long as undetected doesn't cross maxUndetectedTime)
	ofParameter<int> minDetectedTime;
    void minDetectedTimeChangeCalback(int & minDetectedTimeChanged);
	// and has to be detected in at least this part of the frames in that time (0-1)
	ofParameter<float> minDetectedRatio;

	// enable the storage of blobs that disapeared shorter than maxUndetectedTime
	ofParameter<bool> enableUndetectedBlobs;
    void enableUndetectedBlobsCallback(bool & enableUndetectedBlobsChanged);
	// the maximum time a blob can go undetected before the blob manager will remove the blob
	ofParameter<int> maxUndetectedTime;
    void maxundetectedTimeChangeCallback(int & maxUndetectedTimeChanged);
	// blobs that were last seen at the edge of the image (needs setBounds()) may have
	// stepped out of view, they are kept for this long instead
	ofParameter<int> maxUndetectedTimeAtEdge;
	// how far (pixels) from the edge of the image counts as at the edge
	ofParameter<int> edgeMargin;
	// a missing blob keeps moving along its velocity, slowing down and stopping over
	// about this many seconds (people turn round, stop, step back)
	ofParameter<float> undetectedMomentum;

	// keep blobs apart when they touch and are detected as one blob
	ofParameter<bool> enableMergeTracking;
	// how far (pixels) outside a detected blob a blob can be expected and still count as part of it
	ofParameter<int> mergeMargin;
	// the longest a blob can stay merged before it is dropped (ms, 0 = no limit)
	ofParameter<int> maxMergedTime;

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

    // a blob got an id / a blob was removed
    ofEvent<ofxStoredBlobVO> blobAdded;
    ofEvent<ofxStoredBlobVO> blobRemoved;

	ofxBlobsManager();
	// call once per new camera / video frame, currentTime in ms (-1 = ofGetElapsedTimeMillis())
	void update(vector<ofxCvBlob>& newCVBlobs, int currentTime = -1);
	// the size of the images the blobs come from, so blobs aren't expected outside them
	void setBounds(const ofRectangle& bounds);
	void setBounds(float width, float height);
	const ofRectangle& getBounds() const { return bounds; }
	void debugDraw(int baseX, int baseY, int inputWidth, int inputHeight, int displayWidth, int displayHeight);
	bool hasBlob(int blobID);
	ofxStoredBlobVO* getBlob(int blobID); // NULL when there is no blob with that id
	void removeBlob(ofxStoredBlobVO& targetBlob, vector<ofxStoredBlobVO>& blobs);
	void clear();

	// a colour per id, for drawing
	static ofColor getColor(int blobID);

private:
	int sequentialID;
	int sequentialCandidateID;
	int lastUpdateTime;
	ofRectangle bounds;
	
	int getMaxUndetectedTime(const ofxStoredBlobVO& blob);
	bool isAtEdge(const ofRectangle& rect);
	glm::vec2 predict(const ofxStoredBlobVO& blob, int currentTime);

	vector<ofxStoredBlobVO*> findCloseBlobs(ofxCvBlob& newBlob,vector<ofxStoredBlobVO>& blobs);

	// one to one matching of stored blobs to detected blobs, closest first
	void match(vector<ofxStoredBlobVO>& stored, vector<ofxCvBlob>& newBlobs, vector<int>& owner, vector<int>& storedMatch, int currentTime, bool growWhileMissing = true);
	void updateMatched(ofxStoredBlobVO& blob, ofxCvBlob& newBlob, int currentTime);
	void updateMerged(vector<ofxStoredBlobVO*>& members, ofxCvBlob& newBlob, int currentTime);
	float getMaxDistance(ofxStoredBlobVO& blob, int currentTime, bool growWhileMissing = true);
	static float distanceToRect(const glm::vec2& p, const ofRectangle& rect);
};

#endif
