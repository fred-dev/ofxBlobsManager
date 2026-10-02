/*
 *  BlobsManager.cpp
 *
 *  Created by Peter Uithoven on 5/6/11.
 */

#include "ofxBlobsManager.h"

ofxBlobsManager::ofxBlobsManager()
{
    params.setName("Blob manager parameters");
    params.add(maxMergeDis.set("maxMergeDis", 80, 1, 2000));
    params.add(normalizePercentage.set("normalizePercentage", 1, 0, 1));
    params.add(velocitySmoothing.set("velocitySmoothing", 0.3, 0.01, 1));
    params.add(enableMinDetectedTimeFilter.set("enableMinDetectedTimeFilter", true));
    params.add(minDetectedTime.set("minDetectedTime", 500, 1, 2500));
    params.add(minDetectedRatio.set("minDetectedRatio", 0.6, 0, 1));
    params.add(enableUndetectedBlobs.set("enableUndetectedBlobs", true));
    params.add(maxUndetectedTime.set("maxUndetectedTime", 500, 1, 5000));
    params.add(maxUndetectedTimeAtEdge.set("maxUndetectedTimeAtEdge", 1500, 1, 10000));
    params.add(edgeMargin.set("edgeMargin", 10, 0, 200));
    params.add(undetectedMomentum.set("undetectedMomentum", 0.3, 0.01, 5));
    params.add(enableMergeTracking.set("enableMergeTracking", true));
    params.add(mergeMargin.set("mergeMargin", 20, 0, 500));
    params.add(maxMergedTime.set("maxMergedTime", 10000, 0, 60000));
    params.add(giveLowestPossibleIDs.set("giveLowestPossibleIDs", false));
    params.add(maxNumBlobs.set("maxNumBlobs", 100, 1, 9999));
    params.add(debugDrawCandidates.set("debugDrawCandidates", false));
	sequentialID = 0;
	sequentialCandidateID = 0;
	lastUpdateTime = -1;
}
bool sortBlobsOnDis(ofxStoredBlobVO* blob1, ofxStoredBlobVO* blob2)
{
	return (blob1->dis < blob2->dis);
}
void ofxBlobsManager::update(vector<ofxCvBlob>& newBlobs, int currentTime)
{
	int numNewBlobs = newBlobs.size();

	if(currentTime < 0)
		currentTime = ofGetElapsedTimeMillis();
	lastUpdateTime = currentTime;

	// where every blob is expected to be now
	for( size_t i = 0; i < blobs.size(); i++ )
		blobs[i].predicted = predict(blobs[i], currentTime);
	for( size_t i = 0; i < candidateBlobs.size(); i++ )
		candidateBlobs[i].predicted = predict(candidateBlobs[i], currentTime);

	// STEP 1: stored blobs take the closest new blob to where they are expected,
	// one new blob each
	vector<int> owner(numNewBlobs, -1);
	vector<int> blobMatch;
	match(blobs, newBlobs, owner, blobMatch, currentTime);

	// STEP 2: blobs that found nothing, but are expected inside a new blob that
	// another blob already took, have merged with it (two people touching)
	if(enableMergeTracking)
	{
		for( size_t i = 0; i < blobs.size(); i++ )
		{
			ofxStoredBlobVO& blob = blobs[i];
			// only blobs that were being followed up to now, so a blob that went
			// missing a while ago doesn't attach itself to someone walking past
			if(blobMatch[i] != -1 || blob.undetected)
				continue;
			int best = -1;
			float bestDis = FLT_MAX;
			for( int j = 0; j < numNewBlobs; j++ )
			{
				if(distanceToRect(blob.predicted, newBlobs[j].boundingRect) > mergeMargin)
					continue;
				float dis = glm::distance(blob.predicted, glm::vec2(newBlobs[j].centroid.x, newBlobs[j].centroid.y));
				if(dis < bestDis)
				{
					bestDis = dis;
					best = j;
				}
			}
			if(best != -1)
			{
				blobMatch[i] = best;
				if(owner[best] == -1)
					owner[best] = i;
			}
		}
	}

	// update the stored blobs, a new blob with more than one stored blob is a merge
	vector<int> numOwners(numNewBlobs, 0);
	for( size_t i = 0; i < blobs.size(); i++ )
		if(blobMatch[i] != -1)
			numOwners[blobMatch[i]]++;
	for( size_t i = 0; i < blobs.size(); i++ )
	{
		ofxStoredBlobVO& blob = blobs[i];
		int j = blobMatch[i];
		if(j == -1)
		{
			// undetected, the position is predicted from the velocity until it is found again
			blob.undetected = true;
			blob.merged = false;
			blob.numMerged = 0;
			ofLogVerbose("ofxBlobsManager") << "blob " << blob.id << " undetected";
		}
		else if(numOwners[j] == 1)
		{
			if(blob.merged)
				ofLogVerbose("ofxBlobsManager") << "blob " << blob.id << " separated";
			updateMatched(blob, newBlobs[j], currentTime);
		}
	}
	// blobs sharing a new blob are placed inside it together
	for( int j = 0; j < numNewBlobs; j++ )
	{
		if(numOwners[j] < 2)
			continue;
		vector<ofxStoredBlobVO*> members;
		for( size_t i = 0; i < blobs.size(); i++ )
			if(blobMatch[i] == j)
				members.push_back(&blobs[i]);
		updateMerged(members, newBlobs[j], currentTime);
	}

	// STEP 3: candidates take what is left
	vector<int> candidateMatch;
	vector<int> candidateOwner(numNewBlobs, -1);
	for( int j = 0; j < numNewBlobs; j++ )
		if(owner[j] != -1)
			candidateOwner[j] = -2; // taken by a stored blob
	if(enableMinDetectedTimeFilter)
	{
		// candidates don't get the extra reach, so flicker can't keep one going
		match(candidateBlobs, newBlobs, candidateOwner, candidateMatch, currentTime, false);
		for( size_t i = 0; i < candidateBlobs.size(); i++ )
		{
			candidateBlobs[i].numFrames++;
			if(candidateMatch[i] != -1)
			{
				candidateBlobs[i].numDetected++;
				ofLogVerbose("ofxBlobsManager") << "found matching candidate blob: " << candidateBlobs[i].id;
				updateMatched(candidateBlobs[i], newBlobs[candidateMatch[i]], currentTime);
			}
			else
			{
				candidateBlobs[i].undetected = true;
			}
		}
	}

	// STEP 4: new blobs that nothing took become candidates, or blobs straight away
	for( int j = 0; j < numNewBlobs; j++ )
	{
		if(candidateOwner[j] != -1)
			continue;
		ofxCvBlob& newBlob = newBlobs[j];
		// we make a ofxStoredBlobVO out of the ofxCvBlob so we can store a id for example
		ofxStoredBlobVO newStoredBlob(newBlob);
		newStoredBlob.iniDetectedTime = currentTime;
		newStoredBlob.lastDetectedTime = currentTime;
		if(enableMinDetectedTimeFilter)
		{
			newStoredBlob.id = sequentialCandidateID;
			sequentialCandidateID++;
			candidateBlobs.push_back(newStoredBlob);
			ofLogVerbose("ofxBlobsManager") << "new blob candidate: " << newStoredBlob.id << " x: " << newStoredBlob.centroid.x << ", y: " << newStoredBlob.centroid.y;
		}
		else
		{
			newStoredBlob.id = -1; // given below
			blobs.push_back(newStoredBlob);
		}
	}

	int maxUndetectedTime = (enableUndetectedBlobs)? this->maxUndetectedTime.get() : 0;
	// for blobs, getMaxUndetectedTime() gives more time to blobs lost at the edge

	if(enableMinDetectedTimeFilter)
	{
		for( int i = 0; i < (int)candidateBlobs.size(); i++ )
		{
			int undetectedTime = currentTime-candidateBlobs[i].lastDetectedTime;
			int detectionTime = candidateBlobs[i].lastDetectedTime-candidateBlobs[i].iniDetectedTime;
			if(undetectedTime > maxUndetectedTime)
			{
				ofLogVerbose("ofxBlobsManager") << "candidate " << candidateBlobs[i].id << " to long undetected";
				candidateBlobs.erase(candidateBlobs.begin() + i);
				i--;
			}
			else if(detectionTime > minDetectedTime && candidateBlobs[i].numDetected >= minDetectedRatio * candidateBlobs[i].numFrames)
			{
				ofLogVerbose("ofxBlobsManager") << "candidate " << candidateBlobs[i].id << " long enough detected, move to blobs";
				// copy before erasing, the reference would point at the next candidate
				ofxStoredBlobVO candidateBlob = candidateBlobs[i];
				candidateBlobs.erase(candidateBlobs.begin() + i);
				candidateBlob.id = -1; // given below
				blobs.push_back(candidateBlob);
				i--;
			}
		}
	}

	for( int i = 0; i < (int)blobs.size(); i++ )
	{
		ofxStoredBlobVO& blob = blobs[i];
		if(blob.id == -1)
			continue;
		int undetectedTime = currentTime-blob.lastDetectedTime;
		bool tooLongMerged = blob.merged && maxMergedTime > 0 && currentTime - blob.mergedTime > maxMergedTime;
		if(undetectedTime > getMaxUndetectedTime(blob) || tooLongMerged)
		{
			ofLogVerbose("ofxBlobsManager") << "blob " << blob.id << (tooLongMerged ? " merged too long" : " to long undetected") << ", removed";
			ofxStoredBlobVO removed = blob;
			blobs.erase(blobs.begin() + i);
			ofNotifyEvent(blobRemoved, removed);
			i--;
		}
	}

	// give the new blobs an id
	for( int i = 0; i < (int)blobs.size(); i++ )
	{
		ofxStoredBlobVO& blob = blobs[i];
		if(blob.id != -1)
			continue;
		if(giveLowestPossibleIDs)
		{
			int lowestID = 0;
			while(hasBlob(lowestID))
			{
				lowestID++;
			}
			if(lowestID >= maxNumBlobs)
			{
				blobs.erase(blobs.begin() + i);
				i--;
				continue;
			}
			blob.id = lowestID;
		}
		else
		{
			blob.id = sequentialID;
			sequentialID++;
		}
		ofLogVerbose("ofxBlobsManager") << "new blob: " << blob.id << " x: " << blob.centroid.x << ", y: " << blob.centroid.y;
		ofNotifyEvent(blobAdded, blob);
	}
}

void ofxBlobsManager::match(vector<ofxStoredBlobVO>& stored, vector<ofxCvBlob>& newBlobs, vector<int>& owner, vector<int>& storedMatch, int currentTime, bool growWhileMissing)
{
	storedMatch.assign(stored.size(), -1);

	// every possible pair within reach, closest first
	vector< std::pair<float, std::pair<int,int> > > pairs;
	for( size_t i = 0; i < stored.size(); i++ )
	{
		float maxDis = getMaxDistance(stored[i], currentTime, growWhileMissing);
		for( size_t j = 0; j < newBlobs.size(); j++ )
		{
			if(owner[j] != -1)
				continue;
			glm::vec2 newPosition(newBlobs[j].centroid.x, newBlobs[j].centroid.y);
			float dis = glm::distance(stored[i].predicted, newPosition);
			// a blob that went missing may have turned round, so where it was last
			// seen counts too
			if(stored[i].undetected)
				dis = MIN(dis, glm::distance(glm::vec2(stored[i].centroid.x, stored[i].centroid.y), newPosition));
			if(dis < maxDis)
				pairs.push_back(std::make_pair(dis, std::make_pair((int)i, (int)j)));
		}
	}
	sort(pairs.begin(), pairs.end());

	for( size_t p = 0; p < pairs.size(); p++ )
	{
		int i = pairs[p].second.first;
		int j = pairs[p].second.second;
		if(storedMatch[i] == -1 && owner[j] == -1)
		{
			storedMatch[i] = j;
			owner[j] = i;
			stored[i].dis = pairs[p].first;
		}
	}
}

void ofxBlobsManager::updateMatched(ofxStoredBlobVO& blob, ofxCvBlob& newBlob, int currentTime)
{
	glm::vec2 measured(newBlob.centroid.x, newBlob.centroid.y);
	glm::vec2 last(blob.centroid.x, blob.centroid.y);
	float dt = (currentTime - blob.lastDetectedTime) / 1000.0f;

	if(dt > 0)
	{
		glm::vec2 measuredVelocity = (measured - last) / dt;
		if(blob.lastDetectedTime == blob.iniDetectedTime)
			blob.velocity = measuredVelocity; // second sighting, nothing to smooth with yet
		else
			blob.velocity = glm::mix(blob.velocity, measuredVelocity, ofClamp(velocitySmoothing, 0.01, 1));
	}

	// blend between where it was expected and where it was seen
	float amount = ofClamp(normalizePercentage, 0, 1);
	glm::vec2 position = glm::mix(blob.predicted, measured, amount);

	blob.update(newBlob);
	blob.atEdge = isAtEdge(newBlob.boundingRect);
	blob.centroid.x = position.x;
	blob.centroid.y = position.y;
	blob.predicted = position;
	blob.lastDetectedTime = currentTime;
	blob.undetected = false;
	blob.merged = false;
	blob.numMerged = 0;
}

void ofxBlobsManager::updateMerged(vector<ofxStoredBlobVO*>& members, ofxCvBlob& newBlob, int currentTime)
{
	// The detected blob's centre belongs to the group, so it says little about where
	// each blob in it is. Its outline does: each blob keeps the size it had before
	// merging, and on each axis the blob expected to be furthest out on one side
	// makes that edge of the outline. Blobs in between carry on along their velocity.
	// This keeps blobs that stop together in their places, carries blobs that walk
	// together along, and lets blobs that pass through each other swap sides.
	const ofRectangle& rect = newBlob.boundingRect;
	int numMerged = members.size();
	vector<glm::vec2> positions(numMerged);
	for( int m = 0; m < numMerged; m++ )
		positions[m] = members[m]->predicted;
	
	for( int axis = 0; axis < 2; axis++ )
	{
		float rectMin = axis == 0 ? rect.getMinX() : rect.getMinY();
		float rectMax = axis == 0 ? rect.getMaxX() : rect.getMaxY();
		int first = -1, last = -1;
		float firstEdge = FLT_MAX, lastEdge = -FLT_MAX;
		for( int m = 0; m < numMerged; m++ )
		{
			float half = (axis == 0 ? members[m]->boundingRect.width : members[m]->boundingRect.height) / 2;
			float p = members[m]->predicted[axis];
			if(p - half < firstEdge){ firstEdge = p - half; first = m; }
			if(p + half > lastEdge){ lastEdge = p + half; last = m; }
		}
		for( int m = 0; m < numMerged; m++ )
		{
			float half = (axis == 0 ? members[m]->boundingRect.width : members[m]->boundingRect.height) / 2;
			float low = rectMin + half, high = rectMax - half;
			if(low > high)
				low = high = (rectMin + rectMax) / 2; // bigger than the group, centre it
			if(first != last && m == first)
				positions[m][axis] = low;
			else if(first != last && m == last)
				positions[m][axis] = high;
			else
				positions[m][axis] = ofClamp(positions[m][axis], low, high);
		}
	}
	
	for( int m = 0; m < numMerged; m++ )
	{
		ofxStoredBlobVO& blob = *members[m];
		if(!blob.merged)
		{
			ofLogVerbose("ofxBlobsManager") << "blob " << blob.id << " merged with " << numMerged - 1 << " other blob(s)";
			blob.mergedTime = currentTime;
		}
		glm::vec2 position = positions[m];
		float dt = (currentTime - blob.lastDetectedTime) / 1000.0f;
		if(dt > 0)
		{
			glm::vec2 measuredVelocity = (position - glm::vec2(blob.centroid.x, blob.centroid.y)) / dt;
			blob.velocity = glm::mix(blob.velocity, measuredVelocity, ofClamp(velocitySmoothing, 0.01, 1));
		}
		blob.centroid.x = position.x;
		blob.centroid.y = position.y;
		blob.boundingRect.setFromCenter(position.x, position.y, blob.boundingRect.width, blob.boundingRect.height);
		blob.predicted = position;
		blob.lastDetectedTime = currentTime;
		blob.undetected = false;
		blob.merged = true;
		blob.numMerged = numMerged;
	}
}

void ofxBlobsManager::setBounds(const ofRectangle& bounds)
{
	this->bounds = bounds;
}

void ofxBlobsManager::setBounds(float width, float height)
{
	bounds.set(0, 0, width, height);
}

glm::vec2 ofxBlobsManager::predict(const ofxStoredBlobVO& blob, int currentTime)
{
	glm::vec2 position = blob.getPredictedPosition(currentTime, undetectedMomentum);
	// it can't be seen outside the image, so that is as far as it is expected
	if(bounds.width > 0 && bounds.height > 0)
	{
		position.x = ofClamp(position.x, bounds.getMinX(), bounds.getMaxX());
		position.y = ofClamp(position.y, bounds.getMinY(), bounds.getMaxY());
	}
	return position;
}

bool ofxBlobsManager::isAtEdge(const ofRectangle& rect)
{
	if(bounds.width <= 0 || bounds.height <= 0)
		return false;
	return rect.getMinX() <= bounds.getMinX() + edgeMargin || rect.getMinY() <= bounds.getMinY() + edgeMargin ||
	       rect.getMaxX() >= bounds.getMaxX() - edgeMargin || rect.getMaxY() >= bounds.getMaxY() - edgeMargin;
}

int ofxBlobsManager::getMaxUndetectedTime(const ofxStoredBlobVO& blob)
{
	if(!enableUndetectedBlobs)
		return 0;
	return blob.atEdge ? MAX(maxUndetectedTime.get(), maxUndetectedTimeAtEdge.get()) : maxUndetectedTime.get();
}

float ofxBlobsManager::getMaxDistance(ofxStoredBlobVO& blob, int currentTime, bool growWhileMissing)
{
	if(!growWhileMissing)
		return maxMergeDis;
	// the longer a blob has been missing, the less sure we are where it is
	int undetectedTime = currentTime - blob.lastDetectedTime;
	float uncertainty = ofClamp(undetectedTime / float(MAX(1, getMaxUndetectedTime(blob))), 0, 1);
	return maxMergeDis * (1 + uncertainty);
}

float ofxBlobsManager::distanceToRect(const glm::vec2& p, const ofRectangle& rect)
{
	float dx = MAX(MAX(rect.getMinX() - p.x, 0.0f), p.x - rect.getMaxX());
	float dy = MAX(MAX(rect.getMinY() - p.y, 0.0f), p.y - rect.getMaxY());
	return sqrtf(dx*dx + dy*dy);
}

vector<ofxStoredBlobVO*> ofxBlobsManager::findCloseBlobs(ofxCvBlob& newBlob,vector<ofxStoredBlobVO>& blobs)
{
	// find closest blobs, to see if it is the same blob as a stored blob.
	int numBlobs = blobs.size();
	vector<ofxStoredBlobVO*> closeBlobs;
	for( int j = 0; j < numBlobs; j++ )
	{
		ofxStoredBlobVO& blob = blobs.at(j);
		blob.dis = glm::distance(blob.predicted, glm::vec2(newBlob.centroid.x, newBlob.centroid.y));
		if(blob.dis < maxMergeDis)
			closeBlobs.push_back(&blob);
	}
	if(closeBlobs.size() > 0)
		sort (closeBlobs.begin(), closeBlobs.end(), &sortBlobsOnDis);

	return closeBlobs;
}

bool ofxBlobsManager::hasBlob(int blobID)
{
	return getBlob(blobID) != NULL;
}

ofxStoredBlobVO* ofxBlobsManager::getBlob(int blobID)
{
	for( size_t i = 0; i < blobs.size(); i++ )
	{
		if(blobs[i].id == blobID)
			return &blobs[i];
	}
	return NULL;
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

void ofxBlobsManager::clear()
{
	for( size_t i = 0; i < blobs.size(); i++ )
		ofNotifyEvent(blobRemoved, blobs[i]);
	blobs.clear();
	candidateBlobs.clear();
}

ofColor ofxBlobsManager::getColor(int blobID)
{
	return ofColor::fromHsb((blobID * 47) % 256, 200, 255);
}

void ofxBlobsManager::debugDraw(int baseX, int baseY, int inputWidth, int inputHeight, int displayWidth, int displayHeight)
{
	float scaleX = float(displayWidth)/float(inputWidth);
	float scaleY = float(displayHeight)/float(inputHeight);

	ofPushStyle();
	ofEnableAlphaBlending();
	int numBlobs = blobs.size();
	for( int i = 0; i < numBlobs; i++ )
	{
		ofxStoredBlobVO& blob = blobs.at(i);

		glm::vec2 pos = blob.undetected ? blob.predicted : glm::vec2(blob.centroid.x, blob.centroid.y);
		float x = baseX+pos.x*scaleX;
		float y = baseY+pos.y*scaleY;
		ofColor color = getColor(blob.id);

		// velocity, where it will be in half a second
		ofSetColor(color);
		ofSetLineWidth(2);
		ofDrawLine(x, y, x + blob.velocity.x*0.5*scaleX, y + blob.velocity.y*0.5*scaleY);

		// filled when seen, a ring when merged, faint when missing
		if(blob.undetected)
		{
			ofNoFill();
			ofSetColor(color, 120);
		}
		else if(blob.merged)
		{
			ofNoFill();
			ofSetColor(color);
		}
		else
		{
			ofFill();
			ofSetColor(color);
		}
		ofDrawCircle(x, y, 10);

		ofSetHexColor(blob.merged || blob.undetected ? 0xffffff : 0x000000);
		string label = ofToString(blob.id);
		ofDrawBitmapString(label, x-4*label.size(), y+5);
	}

	if(debugDrawCandidates)
	{
		int numCandicateBlobs = candidateBlobs.size();
		for( int i = 0; i < numCandicateBlobs; i++ )
		{
			ofxStoredBlobVO& candidateBlob = candidateBlobs.at(i);

			float x = baseX+candidateBlob.centroid.x*scaleX;
			float y = baseY+candidateBlob.centroid.y*scaleY;

			ofFill();
			ofSetColor(128,128,128,125);
			ofDrawCircle(x, y, 6);
		}
	}
	ofPopStyle();
}
