// Reverb model declaration

/*

//this code has been extrapolated from Jon Dattorro's 1997 paper
//linked here https://ccrma.stanford.edu/~dattorro/EffectDesignPart1.pdf

//written / compiled / ported by Liam Madgwick
 *
 * implementing modulatable allpass filters through DelayLine.h
 * lfo's are made using a quadrature osc algorithm..
 * this will be further optimised! and also made to sound even better!
 *
 * still slightly metallicy at times.. playing with the modulation rates a bit more might help!
 *
 */

#ifndef _revmodel_
#define _revmodel_

#include "tuning.h"
#include "DelayLine.h"
#include "QuadratureOscillator.h"
#include "functions.h"

class revmodel
{
public:
					revmodel();
			void	mute();
			//void	process(int32_t input, int32_t *outputL, int32_t *outputR);
			void	setroomsize(float value);
			float	getroomsize();
			void	setdamp(float value);
			float	getdamp();
			void	setwet(float value);
			float	getwet();
			void	setdry(float value);
			float	getdry();
			void	setwidth(float value);
			float	getwidth();
			void	setmode(float value);
			float	getmode();


			inline void process(int32_t input, int32_t *outputL, int32_t *outputR)
			{
				int32_t outL,outR;

				outL = outR = 0;

				//series input diffusion network, four all pass filters
				outL = id_apf[0].allpass(input);
				outL = id_apf[1].allpass(outL);
				outL = id_apf[2].allpass(outL);
				outL = id_apf[3].allpass(outL);

				//"tank" section input, time modulated allpass filters
				//we also begin to take two paths with the audio here..

				float mod = 1 + (lfo[0].process(0) * 0.25f);
				tank_apm[0].modulate(mod);
				outL = tank_apm[0].allpass(outL + feedbackA);

				mod = 1 + (lfo[1].process(0) * 0.25f);
				tank_apm[1].modulate(mod);
				outR = tank_apm[1].allpass(outL + feedbackB);

				//insert for output matrix here...
				output_matrix[2].write(outL);
				outL = output_matrix[2].read(149.62f);

				output_matrix[0].write(outR);
				outR = output_matrix[0].read(141.69f);

				//damping filters

				current_outL += multiply_32x32_rshift32_rounded(outL - current_outL, damp1);
				//current_outL += damp * (outL - current_outL);
				current_outR += multiply_32x32_rshift32_rounded(outR - current_outR, damp1);
				//current_outR += damp * (outR - current_outR);

				//"tank" output tap 1
				outL = tank0_apf_out[0].allpass(current_outL);
				outR = tank1_apf_out[0].allpass(current_outR);

				//feeds through a delayline of which the output is used for a feedback signal
				output_matrix[3].write(outL * roomsize);
				output_matrix[1].write(outR * roomsize);
				feedbackA = output_matrix[3].read(125.f) * roomsize;
				feedbackB = output_matrix[1].read(106.28f) * roomsize;

				//output matrix begins...
				outL = output_matrix[0].read(8.9f) + output_matrix[0].read(99.8f);
				outL -= tank1_apf_out[1].allpass(current_outR);

				outR = output_matrix[2].read(11.8f) + output_matrix[2].read(121.70f);
				outR -= tank0_apf_out[2].allpass(current_outL);

				outL += output_matrix[1].read(67.f) - output_matrix[2].read(60.8f);
				outL -= tank0_apf_out[1].allpass(current_outL);

				outR += output_matrix[4].read(89.7f) - output_matrix[0].read(70.8f);
				outR -= tank1_apf_out[2].allpass(current_outR);

				outL -= output_matrix[4].read(35.8f);
				outR -= output_matrix[1].read(4.1f);

				outL = multiply_32x32_rshift32_rounded(outL, gain);
				outR = multiply_32x32_rshift32_rounded(outR, gain);

				int32_t mid = multiply_32x32_rshift32_rounded(outL + outR, gain2);
				int32_t side = multiply_32x32_rshift32_rounded(outR - outL, spread);

				*outputL = mid + side;
				*outputR = mid - side;
			}

private:
			void	update();
private:

	int32_t current_outL, current_outR = 0;
	int32_t feedbackA, feedbackB = 0;

	DelayLine<565> id_apf[4];
	DelayLine<1367> tank_apm[2];
	QuadratureOscillator lfo[2];
	DelayLine<3980> tank0_apf_out[3];
	DelayLine<3980> tank1_apf_out[3];
	DelayLine<6616> output_matrix[4];

	float	roomsize;
	float	damp;
	float width;
	int32_t damp1;
	int32_t spread;
	int32_t	gain;
	int32_t gain2;

	float	wet;
	float wet1;
	int32_t wet2;
	float	dry;
	float	mode;
};

#endif//_revmodel_

//ends
