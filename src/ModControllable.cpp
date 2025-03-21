/*
 * Copyright © 2016-2023 Synthstrom Audible Limited
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

#include "ModControllable.h"
#include "ModelStack.h"
#include "AutoParam.h"
#include "functions.h"
#include "TimelineCounter.h"

ModControllable::ModControllable() {
}

// modelStack->autoParam will be NULL in this rare case!!
int ModControllable::getKnobPosForNonExistentParam(int whichModEncoder, ModelStackWithAutoParam* modelStack) {
	return -64;
}

void setTheAutoParamToNull(ModelStackWithThreeMainThings* modelStack) {
	ModelStackWithAutoParam* modelStackWithAutoParam = (ModelStackWithAutoParam*)modelStack;
	modelStackWithAutoParam->autoParam = NULL;
}

ModelStackWithAutoParam* ModControllable::getParamFromModEncoder(int whichModEncoder,
                                                                 ModelStackWithThreeMainThings* modelStack,
                                                                 bool allowCreation) {
	setTheAutoParamToNull(modelStack);
	return (ModelStackWithAutoParam*)modelStack;
}

ModelStackWithAutoParam* ModControllable::getParamFromMIDIKnob(MIDIKnob* knob,
                                                               ModelStackWithThreeMainThings* modelStack) {

	setTheAutoParamToNull(modelStack);
	return (ModelStackWithAutoParam*)modelStack;
}

uint8_t* ModControllable::getModKnobMode() {
	return NULL;
}
