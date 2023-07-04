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
	setdry(initialdry);
	setmode(initialmode);

	setroomsize(1.f);
	setdamp(0.1f);
	setwidth(1.0f);
	reverb.init(44100);

}

void revmodel::mute()
{
	if (getmode() >= freezemode)
		return;
}

void revmodel::update()
{
// Recalculate internal values after parameter change
}

// The following get/set functions are not inlined, because
// speed is never an issue when calling them, and also
// because as you develop the reverb model, you may
// wish to take dynamic action when they are called.

void revmodel::setroomsize(float value)
{
	roomsize = value;
}

float revmodel::getroomsize()
{
	return roomsize;
}

void revmodel::setdamp(float value)
{
	damp = value;
}

float revmodel::getdamp()
{
	return damp;
}

void revmodel::setwet(float value)
{
	wet = value* 1.f;
}

float revmodel::getwet()
{
	return wet /1.f;
}

void revmodel::setdry(float value)
{
	dry = value * 1.f;
}

float revmodel::getdry()
{
	return dry / 1.f;
}

void revmodel::setwidth(float value)
{
	width = value * 1.f;
}

float revmodel::getwidth()
{
	return width / 1.f;
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
