/*
 *  ofxStoredBlobVO.h
 *
 *  Created by Peter Uithoven on 5/6/11.
 */

#ifndef _ofxStoredBlobVO
#define _ofxStoredBlobVO

#include "ofMain.h"
#include "ofxOpenCv.h"

class ofxStoredBlobVO : public ofxCvBlob
{
public:
	int id;
	int lastDetectedTime; // the time when it was last detected (or last seen inside a merged blob)
	int iniDetectedTime; // the time when it was first detected
	float dis; //used to sort on distance and to find the closest blob to merge with

	glm::vec2 velocity;     // pixels per second, smoothed
	glm::vec2 predicted;    // where the blob is expected to be this frame
	bool merged;            // shares one detected blob with other blobs (people walking past each other)
	int mergedTime;         // when it merged
	bool undetected;        // not seen this frame, its position is being predicted from its velocity
	int numMerged;          // how many blobs share the detected blob, 0 when not merged
	bool atEdge;            // was last seen touching the edge of the image
	int numFrames;          // updates since it was first detected
	int numDetected;        // of which it was detected in

	ofxStoredBlobVO();
	ofxStoredBlobVO(ofxCvBlob& newBlob);
	void update(ofxCvBlob& newBlob);

	// position expected at time (ms), from the last known position and the velocity.
	// With momentum > 0 (seconds) the movement slows down and stops, so a blob that
	// has been missing for a while is expected near where it was lost.
	glm::vec2 getPredictedPosition(int time, float momentum = 0) const;
	// time since first detection in seconds
	float getAge(int time) const;
	float getSpeed() const;
};

#endif
