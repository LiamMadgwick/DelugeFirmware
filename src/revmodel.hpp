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
#include "functions.h"
#include "DattoroReverb.h"

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


			void process(int32_t input, int32_t *outputL, int32_t *outputR)
			{
				reverb.set_damp(1.f - damp);
				reverb.set_decay(roomsize);
				reverb.set_width(width);

				int32_t *L, *R;
				reverb.process(input, L, R);

				*outputL = *L;
				*outputR = *R;
			}

private:
			void	update();

			DattoroReverb<int32_t> reverb;

	float	roomsize;
	float	damp;
	float width;
	float	wet;
	float wet1;
	float	dry;

	float	mode;
};

#endif//_revmodel_
//ends
