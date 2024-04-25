#pragma once

class loPassFilter
{
public:
	loPassFilter(const double& pCutoff) : out(0), output(0), cutoff(pCutoff) {}
	~loPassFilter() {}
		
	inline double play(const double& pIn)
	{
		output = out + cutoff * (pIn - out);
		out = output;
		return(output);
	}

private:
	double out, output, cutoff;
};