#ifndef _QuadratureOscillator_
#define _QuadratureOscillator_

#include <cmath>

class QuadratureOscillator
{
public:
	QuadratureOscillator()
	{
		u = 1;
		v = 0;
		k1 = k2 = 0.f;
	}
	void set_frequency(float freq)
	{
		calculate_coefficients(freq);
	}
	inline float process(int wave)
	{
        float tmp = u - (k1*v);
        v = v + k2*tmp;
        u = tmp - k1*v;

        if(wave == 0)
        {
        	return v;
        }
        else
        {
        	return u;
        }
	}
private:
	int fs = 44100;
	float u, v;
	float k1, k2;

	inline void calculate_coefficients(float w)
	{
        k1 = tan(w * 3.1415 / fs);
        k2 = 2 * k1 / (1 + k1 * k1);
	}
};

#endif
