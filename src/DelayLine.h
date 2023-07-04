/*
 * Basic DelayLine for int32_t with linear interpolation for internal effects use.
 * with allpass interpolation functions and settings..
 * make sure to apply the settings at program creation.
 *
 * could still be further optimized!
 *
 * written by Liam Madgwick
 */

/*
  ==============================================================================

    DelayLine.h
    Author:  Liam Madgwick

  ==============================================================================
*/

#pragma once
#include <stdint.h>

template<typename T, uint32_t max_size>
class DelayLine
{
    public:
    void init(int sampleRate)
    {
        Reset();
        ms = (float)sampleRate / 1000.f;
    }
    void Reset()
    {
        for(uint32_t i = 0; i < max_size; i++)
        {
            line[i] = T(0);
        }
        write_ptr = 0;
        delay = 1;
    }
    inline void set_delay_samples(float value)
    {
        uint32_t int_delay = static_cast<uint32_t>(value);
        frac = value - static_cast<float>(int_delay);
        delay = int_delay < max_size ? int_delay : max_size - 1;
    }
    inline void set_delay_ms(float value)
    {
        float delay_in_samples = value * ms;
        uint32_t int_delay = static_cast<uint32_t>(delay_in_samples);
        frac = delay_in_samples - static_cast<float>(int_delay);
        delay = int_delay < max_size ? int_delay : max_size - 1;
    }
    void write(const T sample)
    {
        line[write_ptr] = sample;
        write_ptr = (write_ptr - 1 + max_size) % max_size;
    }
    inline const T read() const
    {
        T a = line[(write_ptr + delay) % max_size];
        T b = line[(1 + write_ptr + delay) % max_size];
        return a + (b - a) * frac;
    }
    inline const T set_read(float value)
    {
        set_delay_ms(value);
        return read();
    }
    inline const T allpass(const T input, uint32_t delay_samples, float gain)
    {
        T a = line[(write_ptr + delay_samples) % max_size];
        T W = input - (a * gain);

        line[write_ptr] = W;
        write_ptr = (write_ptr - 1 + max_size) % max_size;

        return a + (W * gain);
    }

    private:
    //regular delayline variables
    T line[max_size];
    uint32_t write_ptr;
    uint32_t delay;
    float frac;
    float ms;
    //all pass variables for delayline
    float ap_g;
};
