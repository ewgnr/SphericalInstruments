#pragma once

#include "audioProperties.h"

class metro
{
public:
	metro() : phase(0), trigger(false) {}
	~metro() {}

	inline double play(const double& pFrq)
	{
		if (phase >= 1.0)
		{
			phase -= 1.0;
			trigger = true;
		}
		else
		{
			trigger = false;
		}
		double phaseInc = (1. / (sampleRate / (pFrq)));
		double pulse = phase < phaseInc ? 1.0 : 0.0;
		phase += phaseInc;
		return trigger;
	}

private:
	double phase;
	bool trigger;
};
