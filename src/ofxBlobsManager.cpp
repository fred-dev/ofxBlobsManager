/*
 *  BlobsManager.cpp
 *
 *  Created by Peter Uithoven on 5/6/11.
 */

#include "ofxBlobsManager.h"

ofxBlobsManager::ofxBlobsManager()
{
    params.setName("Blob manager parameters");
    params.add(maxMergeDis.set("maxMergeDis", 100, 1, 2000));
    params.add(normalizePercentage.set("normalizePercentage", 1, 0, 100));
    params.add(enableMinDetectedTimeFilter.set("enableMinDetectedTimeFilter", true));
    params.add(minDetectedTime.set("minDetectedTime", 500, 1, 2500));
    params.add(enableUndetectedBlobs.set("enableUndetectedBlobs", false));
    params.add(maxUndetectedTime.set("maxUndetectedTime", 500, 1, 2500));
    params.add(giveLowestPossibleIDs.set("giveLowestPossibleIDs", false));
    params.add(maxNumBlobs.set("maxNumBlobs", 100, 1, 9999));
    params.add(debugDrawCandidates.set("debugDrawCandidates", false));
	sequentialID = 0;
	sequentialCandidateID = 0;
}
bool sortBlobsOnDis(ofxStoredBlobVO* blob1, ofxStoredBlobVO* blob2)
{
	return (blob1->dis < blob2->dis); 
}
void ofxBlobsManager::update(vector<ofxCvBlob>& newBlobs)
{
	int numNewBlobs = newBlobs.size();
	
	int currentTime = ofGetElapsedTimeMillis();
	
	for( int i = 0; i < numNewBlobs; i++ )
	{
		ofxCvBlob& newBlob = newBlobs.at(i);
		
		// find blob that are close to new blob in blobs
		vector<ofxStoredBlobVO*> closeBlobs = findCloseBlobs(newBlob,blobs);
		
		bool foundInStoredBlobs = closeBlobs.size() > 0;
		if(foundInStoredBlobs)
		{
			// update stored blob
			ofxStoredBlobVO * closestBlob = closeBlobs.at(0);
			ofLogVerbose() << "      found matching stored blob: " + ofToString(closestBlob->id);
			
			int prevX = closestBlob->centroid.x;
			int prevY = closestBlob->centroid.y;
			
			closestBlob->update(newBlob);
			if(normalizePercentage < 1)
			{
				closestBlob->centroid.x = prevX*(1-normalizePercentage) + newBlob.centroid.x*normalizePercentage;
				closestBlob->centroid.y = prevY*(1-normalizePercentage) + newBlob.centroid.y*normalizePercentage;
			}
			closestBlob->lastDetectedTime = currentTime;
		}
		else
		{
			if(enableMinDetectedTimeFilter)
			{
				// find new blob in candidates
				vector<ofxStoredBlobVO*> closeCandidateBlobs = findCloseBlobs(newBlob,candidateBlobs);
				bool foundInCandidateBlobs = closeCandidateBlobs.size() > 0;
				if(foundInCandidateBlobs)
				{
					// update candidate
					ofxStoredBlobVO* closestCandidateBlob = closeCandidateBlobs.at(0);
					ofLogVerbose() << "      found matching candidate blob: " + ofToString(closestCandidateBlob->id);
					closestCandidateBlob->update(newBlob);
					closestCandidateBlob->lastDetectedTime = currentTime;
				}
				else
				{
					// store the new candidate blob
					// we make a ofxStoredBlobVO out of the ofxCvBlob so we can store a id for example
					ofxStoredBlobVO newCandidateBlob(newBlob);
					newCandidateBlob.id = sequentialCandidateID;
					sequentialCandidateID++;
					newCandidateBlob.iniDetectedTime = currentTime;
					newCandidateBlob.lastDetectedTime = currentTime;
					candidateBlobs.push_back(newCandidateBlob);
					ofLogVerbose() << "    new blob candidate: " + ofToString(newCandidateBlob.id);
					ofLogVerbose() << "        x: " + ofToString(newCandidateBlob.centroid.x) + ", y: " + ofToString(newCandidateBlob.centroid.y);
				}
			}
			else 
			{
				// store the new blob
				// we make a ofxStoredBlobVO out of the ofxCvBlob so we can store a id for example
				ofxStoredBlobVO newStoredBlob(newBlob);
				if(!giveLowestPossibleIDs)
				{
					newStoredBlob.id = sequentialID;
					sequentialID++;	
				}
				newStoredBlob.iniDetectedTime = currentTime;
				newStoredBlob.lastDetectedTime = currentTime;
				blobs.push_back(newStoredBlob);
				ofLogVerbose() << "    new blob: " + ofToString(newStoredBlob.id);
				ofLogVerbose() << "        x: " + ofToString(newStoredBlob.centroid.x) + ", y: " + ofToString(newStoredBlob.centroid.y);
			}

		}
		
	}
	
	if(enableMinDetectedTimeFilter)
	{
		for( int i = 0; i < candidateBlobs.size(); i++ ) 
		{
			ofxStoredBlobVO& candidateBlob = candidateBlobs.at(i);
			int undetectedTime = currentTime-candidateBlob.lastDetectedTime;
			int detectionTime = candidateBlob.lastDetectedTime-candidateBlob.iniDetectedTime;
			ofLogVerbose() << "    candidateBlob: " << ofToString(candidateBlob.id) + " detectionTime: " + ofToString(detectionTime) + " undetectedTime: " + ofToString(undetectedTime);
			int maxUndetectedTime = (enableUndetectedBlobs)? this->maxUndetectedTime.get() : 0;
			if(undetectedTime > maxUndetectedTime)
			{
				ofLogVerbose() << "      to long undetected";
				removeBlob(candidateBlob,candidateBlobs);
				i--;
			}
			else if(detectionTime > minDetectedTime)
			{
				ofLogVerbose() << "      long enough detected, move to blobs";
				removeBlob(candidateBlob,candidateBlobs);
				if(!giveLowestPossibleIDs)
				{
					candidateBlob.id = sequentialID;
					sequentialID++;	
				}
				else
				{
					candidateBlob.id = -1;
				}
				blobs.push_back(candidateBlob);
				i--;

			}
		}
	}
	
	for( int i = 0; i < blobs.size(); i++ )
	{
		ofxStoredBlobVO& blob = blobs.at(i);
		int undetectedTime = currentTime-blob.lastDetectedTime;
		ofLogVerbose() << "    blob: " + ofToString(blob.id) + " undetectedTime: " + ofToString(undetectedTime);
		int maxUndetectedTime = (enableUndetectedBlobs)? this->maxUndetectedTime.get() : 0;
		ofLogVerbose() << "    this->maxUndetectedTime: " + ofToString(this->maxUndetectedTime);
		ofLogVerbose() << "    local maxUndetectedTime: " + ofToString(maxUndetectedTime);
		if(undetectedTime > maxUndetectedTime)
		{
			removeBlob(blob,blobs);
			i--;
		}
	}
	
	// give lowest possible id's 
	if(giveLowestPossibleIDs)
	{
		for( int i = 0; i < blobs.size(); i++ )
		{
			ofxStoredBlobVO& blob = blobs.at(i);
			if(blob.id == -1)
			{
				int lowestID = 0;
				while(hasBlob(lowestID)) 
				{
					lowestID++;
				}
				blob.id = lowestID;
				ofLogVerbose() << "    lowestID: " + ofToString(lowestID);
				if(blob.id > maxNumBlobs)
				{
					removeBlob(blob,blobs);
					i--;
				}
			}
		}
	}
}

vector<ofxStoredBlobVO*> ofxBlobsManager::findCloseBlobs(ofxCvBlob& newBlob,vector<ofxStoredBlobVO>& blobs)
{
	// find closest blobs, to see if it is the same blob as a stored blob.
	int numBlobs = blobs.size();
	ofLogVerbose() << "  loop stored (candidate) blobs (" + ofToString(numBlobs) << ")";
	vector<ofxStoredBlobVO*> closeBlobs;
	for( int j = 0; j < numBlobs; j++ ) 
	{
		ofxStoredBlobVO& blob = blobs.at(j);
		blob.dis = ofVec2f (blob.centroid).distance(ofVec2f (newBlob.centroid));
		
		ofLogVerbose() << "      " + ofToString(blob.id) + ": dis: " + ofToString(blob.dis);
		if(blob.dis < maxMergeDis)
			closeBlobs.push_back(&blob);
	}
	if(closeBlobs.size() > 0)
		sort (closeBlobs.begin(), closeBlobs.end(), &sortBlobsOnDis);
	
	return closeBlobs;
}

bool ofxBlobsManager::hasBlob(int blobID)
{
	for( int i = 0; i < blobs.size(); i++ ) 
	{
		ofxStoredBlobVO& blob = blobs.at(i);
		if(blob.id == blobID)
			return true;
	}
	return false;
}
void ofxBlobsManager::removeBlob(ofxStoredBlobVO& targetBlob, vector<ofxStoredBlobVO>& blobs)
{
	vector <ofxStoredBlobVO>::iterator itr;
	for (itr = blobs.begin(); itr != blobs.end(); ++itr) {
		ofxStoredBlobVO& blob = *itr;
		if(&blob == &targetBlob)
		{
			blobs.erase(itr);
			break;
		}
	}
}

void ofxBlobsManager::debugDraw(int baseX, int baseY, int inputWidth, int inputHeight, int displayWidth, int displayHeight)
{
	float scaleX = float(displayWidth)/float(inputWidth);
	float scaleY = float(displayHeight)/float(inputHeight);
	
	ofEnableAlphaBlending();
	int numBlobs = blobs.size();
	for( int i = 0; i < numBlobs; i++ ) 
	{
		ofxStoredBlobVO& blob = blobs.at(i);
		
		int x = baseX+blob.centroid.x*scaleX;
		int y = baseY+blob.centroid.y*scaleY;
		
		ofFill();
		ofSetHexColor(0x00ffff);
		ofDrawCircle(x, y, 10);

		ofSetHexColor(0x000000);
		if(blob.id >= 10) x -= 4;
		ofDrawBitmapString(ofToString(blob.id),x-4,y+5);
	}
	
	if(debugDrawCandidates)
	{
		ofEnableAlphaBlending();
		int numCandicateBlobs = candidateBlobs.size();
		for( int i = 0; i < numCandicateBlobs; i++ ) 
		{
			ofxStoredBlobVO& candidateBlob = candidateBlobs.at(i);
			
			int x = baseX+candidateBlob.centroid.x*scaleX;
			int y = baseY+candidateBlob.centroid.y*scaleY;
			
			ofFill();
			ofSetColor(0,255,255,125);
			ofDrawCircle(x, y, 10);
			
			ofSetHexColor(0x000000);
			if(candidateBlob.id >= 10) x -= 4;
			ofDrawBitmapString(ofToString(candidateBlob.id),x-4,y+5);
		}
		ofDisableAlphaBlending();
	}	
}

