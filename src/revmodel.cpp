// Reverb model implementation
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain

/*
 * Copyright © 2015-2023 Synthstrom Audible Limited
 *
 * This file is part of The Synthstrom Audible Deluge Firmware.
 *
 * The Synthstrom Audible Deluge Firmware is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

//i edited this file so I wouldn't have to start again from scratch with the menus and modelstack/audioengine.

#include "revmodel.hpp"

revmodel::revmodel()
{
	setwet(initialwet);
	setroomsize(initialroom);
	setdry(initialdry);
	setdamp(initialdamp);
	setwidth(initialwidth);
	setmode(initialmode);

	for(int i = 0; i < 4; i++)
	{
		id_apf[i].reset();
		id_apf[i].allpass_settings(id_apf_times[i], id_apf_gains[i]);
		output_matrix[i].reset();
	}
	for(int i = 0; i < 3; i++)
	{
		tank0_apf_out[i].reset();
		tank0_apf_out[i].allpass_settings(tank0_apf_times[i], tank_out_gain);
		tank1_apf_out[i].reset();
		tank1_apf_out[i].allpass_settings(tank1_apf_times[i], tank_out_gain);
	}
	for(int i = 0; i < 2; i++)
	{
		tank_apm[i].reset();
		tank_apm[i].allpass_settings(tank_apm_times[i], tank_apm_gain);
		lfo[i].set_frequency(lfo_rates[i]);
	}

	gain = 0.125f * 2147483648u;
	gain2 = 0.5f * 2147483648u;

}

void revmodel::mute()
{
	if (getmode() >= freezemode)
		return;

	for(int i = 0; i < 4; i++)
	{
		id_apf[i].reset();

		output_matrix[i].reset();
	}
	for(int i = 0; i < 2; i++)
	{
		tank_apm[i].reset();
		lfo[i].set_frequency(lfo_rates[i]);
	}
	for(int i = 0; i < 3; i++)
	{
		tank0_apf_out[i].reset();
		tank1_apf_out[i].reset();
	}
}

void revmodel::update()
{
// Recalculate internal values after parameter change
	damp1 = damp * 2147483648u;
	spread = width * 2147483648u;
}

// The following get/set functions are not inlined, because
// speed is never an issue when calling them, and also
// because as you develop the reverb model, you may
// wish to take dynamic action when they are called.

void revmodel::setroomsize(float value)
{
	roomsize = value * 1.f;
	update();
}

float revmodel::getroomsize()
{
	return roomsize / 1.f;
}

void revmodel::setdamp(float value)
{
	damp = (value * 0.5f);
	update();
}

float revmodel::getdamp()
{
	return (damp / 0.5f);
}

void revmodel::setwet(float value)
{
	wet = value*scalewet;
	update();
}

float revmodel::getwet()
{
	return wet/scalewet;
}

void revmodel::setdry(float value)
{
	dry = value*scaledry;
}

float revmodel::getdry()
{
	return dry/scaledry;
}

void revmodel::setwidth(float value)
{
	width = value * 0.5f;
	update();
}

float revmodel::getwidth()
{
	return width / 0.5f;
}

void revmodel::setmode(float value)
{
	mode = value;
	update();
}

float revmodel::getmode()
{
	if (mode >= freezemode)
		return 1;
	else
		return 0;
}

//ends
