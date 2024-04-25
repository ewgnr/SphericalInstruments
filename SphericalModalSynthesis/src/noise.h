#pragma once

#include "ofMain.h"

class noiseGenerator
{
public:
	noiseGenerator() {}
	~noiseGenerator() {}

	inline double play()
	{
		return ofRandom(-1, 1);
	}
};
