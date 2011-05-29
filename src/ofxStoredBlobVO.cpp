/*
 *  BlobVO.cpp
 *
 *  Created by Peter Uithoven on 5/6/11.
 */

#import "ofxStoredBlobVO.h"


ofxStoredBlobVO::ofxStoredBlobVO(ofxCvBlob newBlob)
{
	// we will do a quick "shallow" copy 
	area = newBlob.area;
	length = newBlob.length;
	boundingRect = newBlob.boundingRect;
	centroid = newBlob.centroid;
	hole = newBlob.hole;
	pts = newBlob.pts;
	nPts = newBlob.nPts;
	
	id = -1;
	lastDetectedTime = 0;
}