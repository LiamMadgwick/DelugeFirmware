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

#include <ConsequenceArrangerParamsTimeInserted.h>
#include <ParamManager.h>
#include "definitions.h"
#include "song.h"
#include "ModelStack.h"
#include "ParamSet.h"
#include "ParamCollection.h"

ConsequenceArrangerParamsTimeInserted::ConsequenceArrangerParamsTimeInserted(int32_t newPos, int32_t newLength) {
	pos = newPos;
	length = newLength;
}

int ConsequenceArrangerParamsTimeInserted::revert(int time, ModelStack* modelStack) {

	ParamCollectionSummary* unpatchedParamsSummary = modelStack->song->paramManager.getUnpatchedParamSetSummary();

	ModelStackWithParamCollection* modelStackWithParamCollection =
	    modelStack->song->setupModelStackWithSongAsTimelineCounter(modelStack)
	        ->addParamCollectionSummary(unpatchedParamsSummary);

	if (time == BEFORE) {
		((ParamSet*)modelStackWithParamCollection->paramCollection)
		    ->deleteTime(modelStackWithParamCollection, pos, length);
	}
	else {
		((ParamSet*)modelStackWithParamCollection->paramCollection)
		    ->insertTime(modelStackWithParamCollection, pos, length);
	}

	return NO_ERROR;
}
