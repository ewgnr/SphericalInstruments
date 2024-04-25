#pragma once

#include "ofMain.h"

class hanningWindow
{
public:
	hanningWindow() {}
	~hanningWindow() {}

	inline double play(const unsigned long& pWindowLength, const unsigned long& pWindowPos)
	{
		return 0.5 * (1.0 - cos((2.0 * PI * pWindowPos) / (pWindowLength - 1)));
	}
}; 