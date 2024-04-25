#pragma once

class highPassFilter
{
public:
	highPassFilter() : out(0), output(0) {}

	inline double play(const double& pIn, const double& pCutoff)
	{
		output = pIn - (out + pCutoff * (pIn - out));
		out = output;
		return(output);
	}

private:
	double out, output;
};