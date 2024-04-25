#pragma once

#include "ofMain.h"
#include "audioProperties.h"

class modalFilter
{
public:
	modalFilter() : x1(0), y1(0), y2(0), alpha(0), beta(0), sum(0), frq(0), q(0), amp(0), out(0) {}
	~modalFilter() {}

	void init(const double& pFrq, const double& pQ, const double& pAmp)
	{
		frq = pFrq;
		q = pQ;
		amp = pAmp;

		if (44100 / frq < PI)
		{
			std::cout << "This filter becomes unstable if sr/xfreq < pi (e.g xfreq > 14037 Hz @ 44 kHz)" << "\n";
			return;
		}

		frq = frq * 2 * PI;
		alpha = sampleRate / frq;
		beta = alpha * alpha;
	}

	inline double play(const double& pIn)
	{
		x1 = pIn;
		sum = (-(1 - 2 * beta) * y1 + x1 - (beta - alpha / (2 * q)) * y2) / (beta + alpha / (2 * q));
		y2 = y1;
		y1 = sum;
		out = sum * sampleRate / (2 * frq) * amp;
		return out;
	}

private:
	double x1, y1, y2, alpha, beta, sum, frq, q, amp, out;
};

