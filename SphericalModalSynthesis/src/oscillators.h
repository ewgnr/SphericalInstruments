#pragma once

#include "ofMain.h"
#include "audioProperties.h"

class sineToneGenerator
{
public:
	sineToneGenerator() : phase(0) {}
	~sineToneGenerator() {}

	inline double play(const double& pFrq)
	{
		double sine = sin(phase * (PI * 2));
		if (phase >= 1.0) phase -= 1.0;
		phase += (1. / (sampleRate / (pFrq)));
		return sine;
	}

private:
	double phase;
};

class pulseWaveGenerator
{
public:
	pulseWaveGenerator() : phase(0) {}
	~pulseWaveGenerator() {}

	inline double play(const double& pFrq)
	{
		if (phase >= 1.0) phase -= 1.0;
		double phaseInc = (1. / (sampleRate / (pFrq)));
		double pulse = phase < phaseInc ? 1.0 : 0.0;
		phase += phaseInc;
		return pulse;
	}

private:
	double phase;
};
