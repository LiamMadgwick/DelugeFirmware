/*
 * Basic DelayLine for int32_t with linear interpolation for internal effects use.
 * with allpass interpolation functions and settings..
 * make sure to apply the settings at program creation.
 *
 * could still be further optimized!
 *
 * written by Liam Madgwick
 */


#ifndef _DelayLine_
#define _DelayLine_

#include "r_typedefs.h"
#include "functions.h"

template<int max_size>
class DelayLine
{
public:
	DelayLine(){
		reset();
	}

	inline void reset()
	{
		for(int i = 0; i < max_size; i++)
		{
			line[i] = 0;
		}
		write_ptr = 1;
		delay = 1;
	}

	inline void allpass_settings(float delay_time_ms, float gain)
	{
		ap_g = gain * 2147483647;
		delay_time = delay_time_ms;
	}

	inline void modulate(float mod)
	{
		delay_time *= mod;
	}

	inline void write(int32_t input)
	{
		line[write_ptr] = input;
		write_ptr = (write_ptr - 1 + max_size) % max_size;
	}

	inline int32_t read(float delay_time_in_ms)
	{
		set_delay(delay_time_in_ms * ms);
		int32_t a = line[(write_ptr + delay) % max_size];
		int32_t b = line[(write_ptr + delay + 1) % max_size];
		return a + multiply_32x32_rshift32_rounded(b - a, frac1) << 1;
	}

	inline int32_t allpass(int32_t input)
	{
		int32_t x = read(delay_time);
		write(input + multiply_32x32_rshift32_rounded(x, ap_g) << 1);
		return x + multiply_32x32_rshift32_rounded(input, -ap_g) << 1;
	}

	inline void set_delay(float delay_time_in_samples)
	{
		int int_delay = static_cast<int>(delay_time_in_samples);
		frac = delay_time_in_samples - static_cast<float>(int_delay);
		frac1 = frac * 2147483647;
		delay = static_cast<int>(int_delay) < max_size ? int_delay : max_size - 1;
	}

private:
	int ms = 44;
	int32_t line[max_size];
	int write_ptr = 0;
	int delay = 0;
	float frac;
	int32_t frac1;
	float delay_time;
	int32_t ap_g;
};


#endif
