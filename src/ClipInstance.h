/*
 * Copyright © 2018-2023 Synthstrom Audible Limited
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

#ifndef CLIPINSTANCE_H_
#define CLIPINSTANCE_H_

#include "Positionable.h"
#include "r_typedefs.h"

class InstrumentClip;
class Action;
class Output;
class Clip;

class ClipInstance : public Positionable {
public:
	ClipInstance();
	void getColour(uint8_t* colour);
	void change(Action* action, Output* output, int32_t newPos, int32_t newLength, Clip* newClip);

	int32_t length;
	Clip* clip;
};

#endif /* CLIPINSTANCE_H_ */
