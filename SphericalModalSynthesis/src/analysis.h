#pragma once

#include "ofMain.h"

class onsetDetection
{
public:
	onsetDetection() : rms(0), numCounted(0), decayRate(0), minimumThreshold(0), threshold(0) {}
	~onsetDetection() {}

	void init(const double& pDecayRate, const double& pMinimumThreshold)
	{
		decayRate = pDecayRate;
		minimumThreshold = pMinimumThreshold;
		threshold = minimumThreshold;

		rms = 0;
		numCounted = 0;
	}

	void get(const double& pSample)
	{
		rms += pSample * pSample;
		numCounted += 2;
	}

	inline bool detect()
	{
		rms /= (double)numCounted;
		rms = sqrt(rms);

		threshold = ofLerp(threshold, minimumThreshold, decayRate);

		if (rms > threshold)
		{
			threshold = rms;
			std::cout << "trig" << std::endl;
			return true;
		}
	}

private:
	double rms, numCounted, decayRate, minimumThreshold, threshold;
};
