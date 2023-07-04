#ifndef DattoroReverb_h_
#define DattoroReverb_h_

#include "DelayLine.h"

const uint32_t pre_delay_max_size = 8820;
const uint32_t input_diffusion_max_size = 400;
const uint32_t allpass_mod_max_size = 1000;
const uint32_t output_matrix_max_size = 10000;
const uint32_t allpass_max_size = 3000;

template<typename T>
class DattoroReverb
{
    public:
    void init(int sampleRate)
    {
        FS = sampleRate;
        set_pre_delay(15.f);
        set_bandwidth(1.f);
        set_input_diffusion(0.5f);
        set_decay(.5f);
        set_damp(0.05f);
        set_width(0.5f);

        pre_delay.init(sampleRate);
        for(int i = 0; i < 4; i++)
        {
            output_matrix[i].init(sampleRate);
            input_diffuse[i].init(sampleRate);
        }
        for(int i = 0; i < 2; i++)
        {
            allpass_mod[i].init(sampleRate);
            //lfo[i].Init(sampleRate);
            //lfo[i].setFreq(lfo_rates[i]);
        }
    }

    void set_pre_delay(float value)
    {
        if(pre_delay_ms != value)
        {
            pre_delay_ms = value;
        }
    }

    void set_bandwidth(float value)
    {
        if(bandwidth != value)
        {
            bandwidth = value;
        }
    }

    void set_input_diffusion(float value)
    {
        if(id_gain[0] != value)
        {
            id_gain[0] = value;
            id_gain[1] = value;
            id_gain[2] = value - 0.15f;
            id_gain[3] = value - 0.15f;
        }
    }

    void set_decay(float value)
    {
        value *= 0.4f;
        value += 0.55f;

        if(decay1 != value)
        {
            decay1 = value;
            decay2 = value - 0.15f;
        }
    }

    void set_damp(float value)
    {
    	value = value > 0.9f ? 0.9f : value;
    	value = value < 0.05f ? 0.05f : value;
        if(damp != value)
        {
            damp = value;
        }
    }

    void set_width(float value)
    {
        if(width != value)
        {
            width = value;
        }
    }

    inline void process(const T input, T *outputL, T *outputR)
    {
        //y = do_predelay(input, pre_delay_ms);
        //y = do_bandwidth(y);

        T y = do_input_diffusion(input);
        
        T a,b;

        a = do_allpass_mod_a(y);
        b = do_allpass_mod_b(y);

        a = do_damp_a(a);
        b = do_damp_b(b);

        a = do_allpass_a(a);
        b = do_allpass_b(b);

        a = left_output();
        b = right_output();

        float spread = width * .5f;
        T mid = (a + b) * .5f;
        T side = (a - b) * spread;

        *outputL =  mid + side;
        *outputR =  mid - side;
    }

    private:
    //variables and params
    int FS;
    float pre_delay_ms;
    float bandwidth;
    float id_gain[4];
    float decay1 , decay2;
    float damp;
    float width;

    //feedback and held samples for filters..
    T fb_a, fb_b;
    T bx;
    T current_a, current_b;

    //output matrix delaylines.
    DelayLine< T, output_matrix_max_size> output_matrix[4];

    //pre delay
    DelayLine< T, pre_delay_max_size> pre_delay;

    inline T do_predelay(T input, float delay_ms)
    {
        T x = input;
        float deltime = delay_ms;
        pre_delay.write(x);
        return pre_delay.set_read(deltime);
    }

    //bandwidth filter
    inline T do_bandwidth(T input)
    {
        T x = input * bandwidth;
        x += bx * (1.f - bandwidth);
        bx = x;
        return x;
    }

    //input diffusion
    DelayLine< T, input_diffusion_max_size> input_diffuse[4];
    uint16_t id_times[4] = {142, 379, 107, 277};

    inline T do_input_diffusion(T input)
    {
        T x = input;
        for(int i = 0; i < 4; i++)
        {
            x =  input_diffuse[i].allpass(x, id_times[i], id_gain[i]);
        }
        return x;
    }

    //time modulated all pass
    DelayLine< T, allpass_mod_max_size> allpass_mod[2];
    uint16_t allpass_mod_times[2] = {672, 908};
    float lfo_rates[2] = {0.07f, 0.1f};
    //QuadratureOsc lfo[2];

    inline T do_allpass_mod_a(T input)
    {
        //float m = lfo[0].Process(0);
        //m *= 16.f;
        T x = input + fb_b;
        x = allpass_mod[0].allpass(x, allpass_mod_times[0] , decay1); // + lfo here..

        output_matrix[0].write(x);

        return output_matrix[0].set_read(100.9f);
    }

    inline T do_allpass_mod_b(T input)
    {
        //float m = lfo[0].Process(0);
        //m *= 16.f;
        T x = input + fb_a;
        x = allpass_mod[1].allpass(x, allpass_mod_times[1] , decay1); // + lfo here..

        output_matrix[2].write(x);

        return output_matrix[2].set_read(95.6f);
    }

    //damping filter
    inline T do_damp_a(T input)
    {
        T x = input * (1 - damp);
        x += (current_a * damp);
        current_a = x;
        return x * decay1;
    }

    inline T do_damp_b(T input)
    {
        T x = input * (1-damp);
        x += (current_b * damp);
        current_b = x;
        return x * decay1;
    }

    //allpass
    DelayLine<T, allpass_max_size> allpass[2];
    uint16_t allpass_times[2] = {1800, 2656};

    inline T do_allpass_a(T input)
    {
        T x = allpass[0].allpass(input, allpass_times[0], decay2);
        
        output_matrix[1].write(x);
        fb_a = output_matrix[1].set_read(84.3f) * decay1;
        return x;
    }

    inline T do_allpass_b(T input)
    {
        T x = allpass[1].allpass(input, allpass_times[1], decay2);

        output_matrix[3].write(x);
        fb_b = output_matrix[3].set_read(71.7f) * decay1;
        return x;
    }

    //output matrix

    inline T left_output()
    {
        T x = output_matrix[1].set_read(6.f);
        x += output_matrix[1].set_read(67.4f);
        x -= output_matrix[3].set_read(43.3f);
        x += output_matrix[3].set_read(45.2f);
        x -= output_matrix[0].set_read(45.1f);
        x -= output_matrix[1].set_read(4.24f);
        x -= output_matrix[1].set_read(24.17f);
        return 0.6f * x;
    }

    inline T right_output()
    {
        T x = output_matrix[0].set_read(8.f);
        x += output_matrix[0].set_read(82.2f);
        x -= output_matrix[1].set_read(27.8f);
        x += output_matrix[1].set_read(60.6f);
        x -= output_matrix[2].set_read(47.8f);
        x -= output_matrix[3].set_read(7.5f);
        x -= output_matrix[3].set_read(2.7f);
        return 0.6f * x;
    }

};

#endif
