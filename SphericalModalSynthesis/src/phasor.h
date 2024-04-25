#pragma once

#include "ofMain.h"
#include "audioProperties.h"

class phasor
{
public:
	phasor(const double& pFreq, const double& pStartPhase, const double& pEndPhase) : freq(pFreq), startPhase(pStartPhase), endPhase(pEndPhase), output(0), phase(0) {}
	~phasor() {}

	inline double play()
	{
		output = phase;
		if (phase < startPhase)
		{
			phase = startPhase;
		}
		if (phase >= endPhase) phase = startPhase;
		phase += ((endPhase - startPhase) / (sampleRate / (freq)));
		return(output);
	}

	double phase;

private:
	double output, freq, startPhase, endPhase;
};