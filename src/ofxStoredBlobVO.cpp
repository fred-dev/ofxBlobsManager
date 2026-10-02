/*
 *  ofxStoredBlobVO.cpp
 *
 *  Created by Peter Uithoven on 5/6/11.
 */

#include "ofxStoredBlobVO.h"

ofxStoredBlobVO::ofxStoredBlobVO()
{
	id = -1;
	lastDetectedTime = 0;
	iniDetectedTime = 0;
	dis = 0;
	velocity = glm::vec2(0, 0);
	predicted = glm::vec2(0, 0);
	merged = false;
	mergedTime = 0;
	undetected = false;
	numMerged = 0;
	atEdge = false;
	numFrames = 1;
	numDetected = 1;
}

ofxStoredBlobVO::ofxStoredBlobVO(ofxCvBlob& newBlob) : ofxStoredBlobVO()
{
	// a quick "shallow" copy
	update(newBlob);
	predicted = glm::vec2(centroid.x, centroid.y);
}

void ofxStoredBlobVO::update(ofxCvBlob& newBlob)
{
	// a quick "shallow" update
	area = newBlob.area;
	length = newBlob.length;
	boundingRect = newBlob.boundingRect;
	centroid = newBlob.centroid;
	hole = newBlob.hole;
	pts = newBlob.pts;
	nPts = newBlob.nPts;
}

glm::vec2 ofxStoredBlobVO::getPredictedPosition(int time, float momentum) const
{
	float elapsed = MAX(0, time - lastDetectedTime) / 1000.0f;
	if(momentum > 0)
		elapsed = momentum * (1 - expf(-elapsed / momentum));
	return glm::vec2(centroid.x, centroid.y) + velocity * elapsed;
}

float ofxStoredBlobVO::getAge(int time) const
{
	return (time - iniDetectedTime) / 1000.0f;
}

float ofxStoredBlobVO::getSpeed() const
{
	return glm::length(velocity);
}
