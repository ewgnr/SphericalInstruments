#pragma once

#include "ofMain.h"

class modalFilterRouting
{
public:
	modalFilterRouting() {}
	~modalFilterRouting() {}

	void getParams(const std::vector<size_t>& pRawPickUpPos, const std::vector<glm::vec3>& pRef_vert_pnt)
	{
		rawPickUpPos = pRawPickUpPos;
		ref_vert_pnt = pRef_vert_pnt;
	}

	void drawRawPickUpPos(const ofColor& pColor)
	{
		ofPushStyle();
		ofSetColor(pColor);
		sphere.setRadius(2);
		for (auto const& i : rawPickUpPos)
		{
			sphere.setPosition(ref_vert_pnt[i + 1]);
			sphere.draw();
		}
		ofPopStyle();
	}

private:
	std::vector<size_t> rawPickUpPos, modePickUpPos3;
	std::vector<glm::vec3> ref_vert_pnt;
	ofSpherePrimitive sphere;
};