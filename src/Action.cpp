/*
 * Copyright © 2017-2023 Synthstrom Audible Limited
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

#include <ActionClipState.h>
#include <AudioEngine.h>
#include <AudioFileManager.h>
#include <ConsequenceClipInstanceExistence.h>
#include <InstrumentClip.h>
#include "Action.h"
#include "r_typedefs.h"
#include "Consequence.h"
#include "ConsequenceParamChange.h"
#include "functions.h"
#include "song.h"
#include "Note.h"
#include "ActionLogger.h"
#include "ConsequenceNoteExistence.h"
#include "uart.h"
#include <new>
#include "GeneralMemoryAllocator.h"
#include "ConsequenceClipLength.h"
#include "ConsequenceClipExistence.h"
#include "ConsequenceAudioClipSetSample.h"
#include "ConsequenceNoteArrayChange.h"
#include "ModelStack.h"

Action::Action(int newActionType) {
	firstConsequence = NULL;
	nextAction = NULL;
	type = newActionType;
	openForAdditions = true;

	clipStates = NULL;
	numClipStates = 0;
	creationTime = AudioEngine::audioSampleTimer;

	offset = 0;
}

// Call this before the destructor!
void Action::prepareForDestruction(int whichQueueActionIn, Song* song) {

	deleteAllConsequences(whichQueueActionIn, song, true);

	if (clipStates) generalMemoryAllocator.dealloc(clipStates);
}

void Action::deleteAllConsequences(int whichQueueActionIn, Song* song, bool destructing) {
	Consequence* currentConsequence = firstConsequence;
	while (currentConsequence) {
		AudioEngine::routineWithClusterLoading(); // -----------------------------------
		Consequence* toDelete = currentConsequence;
		currentConsequence = currentConsequence->next;
		toDelete->prepareForDestruction(whichQueueActionIn, song);
		toDelete->~Consequence();
		generalMemoryAllocator.dealloc(toDelete);
	}
	if (!destructing) firstConsequence = NULL;
}

void Action::addConsequence(Consequence* consequence) {
	consequence->next = firstConsequence;
	firstConsequence = consequence;
}

// Returns error code
int Action::revert(int time, ModelStack* modelStack) {

	Consequence* thisConsequence = firstConsequence;

	// If we're a record-arrangement-from-session Action, there's a trick - we know that whether we're being undone or redone, this will involve
	// clearing the arrangement to the right of a certain pos. So we'll do that, and we'll record the Consequences involved in doing so, so that
	// this Action can then be reverted in the opposite direction next time
	if (type == ACTION_ARRANGEMENT_RECORD) {
		firstConsequence = NULL;
		currentSong->clearArrangementBeyondPos(posToClearArrangementFrom, this);
		time = BEFORE;
	}

	Consequence* newFirstConsequence = NULL;

	int error = NO_ERROR;

	while (thisConsequence) {
		if (!error) {

			// Can't quite remember why, but we don't wanna revert param changes for arrangement-record actions
			if (type == ACTION_ARRANGEMENT_RECORD && thisConsequence->type == CONSEQUENCE_PARAM_CHANGE) {}

			else {
				error = thisConsequence->revert(time, modelStack);
				// If an error occurs, keep swapping the order cos it's too late to stop, but don't keep calling the things
			}
		}

		Consequence* nextConsequence = thisConsequence->next;

		// Special case for arrangement-record. See big comment above
		if (type == ACTION_ARRANGEMENT_RECORD) {
			// Delete the old one
			thisConsequence->prepareForDestruction(
			    AFTER,
			    modelStack
			        ->song); // Have to put AFTER. See the effect this will have in ConsequenceCDelete::prepareForDestruction()
			thisConsequence->~Consequence();
			generalMemoryAllocator.dealloc(thisConsequence);
		}

		// Or, normal case
		else {
			// Reverse the order, for next time we revert this, which will be in the other direction
			thisConsequence->next = newFirstConsequence;
			newFirstConsequence = thisConsequence;
		}

		thisConsequence = nextConsequence;
	}

	if (type != ACTION_ARRANGEMENT_RECORD) {
		firstConsequence = newFirstConsequence;
	}

	return error;
}

bool Action::containsConsequenceParamChange(ParamCollection* paramCollection, int paramId) {
	// See if this param has already had its state snapshotted. If so, get out
	for (Consequence* thisCons = firstConsequence; thisCons; thisCons = thisCons->next) {
		if (thisCons->type == CONSEQUENCE_PARAM_CHANGE) {
			ConsequenceParamChange* thisConsParamChange = (ConsequenceParamChange*)thisCons;
			if (thisConsParamChange->modelStack.paramCollection == paramCollection
			    && thisConsParamChange->modelStack.paramId == paramId)
				return true;
		}
	}
	return false;
}

void Action::recordParamChangeIfNotAlreadySnapshotted(ModelStackWithAutoParam const* modelStack, bool stealData) {

	// If we already have a snapshot of this, we can get out.
	if (containsConsequenceParamChange(modelStack->paramCollection, modelStack->paramId)) {

		// Except, if we were planning to steal the data, well we'd better pretend we've just done that by deleting it instead.
		if (stealData) {
			modelStack->autoParam->nodes.empty();
		}
		return;
	}

	// If we're still here, we need to snapshot.
	recordParamChangeDefinitely(modelStack, stealData);
}

void Action::recordParamChangeDefinitely(ModelStackWithAutoParam const* modelStack, bool stealData) {

	void* consMemory = generalMemoryAllocator.alloc(sizeof(ConsequenceParamChange));

	if (consMemory) {
		ConsequenceParamChange* newCons = new (consMemory) ConsequenceParamChange(modelStack, stealData);
		addConsequence(newCons);
	}
}

bool Action::containsConsequenceNoteArrayChange(InstrumentClip* clip, int noteRowId, bool moveToFrontIfFound) {

	for (Consequence** prevPointer = &firstConsequence; *prevPointer; prevPointer = &(*prevPointer)->next) {
		Consequence* thisCons = *prevPointer;
		if (thisCons->type == CONSEQUENCE_NOTE_ARRAY_CHANGE) {
			ConsequenceNoteArrayChange* thisNoteArrayChange = (ConsequenceNoteArrayChange*)thisCons;
			if (thisNoteArrayChange->clip == clip && thisNoteArrayChange->noteRowId == noteRowId) {
				if (moveToFrontIfFound) {
					*prevPointer = thisCons->next;

					thisCons->next = firstConsequence;
					firstConsequence = thisCons;
				}
				return true;
			}
		}
	}
	return false;
}

int Action::recordNoteArrayChangeIfNotAlreadySnapshotted(InstrumentClip* clip, int noteRowId, NoteVector* noteVector,
                                                         bool stealData, bool moveToFrontIfAlreadySnapshotted) {
	if (containsConsequenceNoteArrayChange(clip, noteRowId, moveToFrontIfAlreadySnapshotted)) return NO_ERROR;

	// If we're still here, we need to snapshot.
	return recordNoteArrayChangeDefinitely(clip, noteRowId, noteVector, stealData);
}

int Action::recordNoteArrayChangeDefinitely(InstrumentClip* clip, int noteRowId, NoteVector* noteVector,
                                            bool stealData) {
	void* consMemory = generalMemoryAllocator.alloc(sizeof(ConsequenceNoteArrayChange));

	if (!consMemory) return ERROR_INSUFFICIENT_RAM;

	ConsequenceNoteArrayChange* newCons =
	    new (consMemory) ConsequenceNoteArrayChange(clip, noteRowId, noteVector, stealData);
	addConsequence(newCons);

	return NO_ERROR; // Though we wouldn't know if there was a RAM error as ConsequenceNoteArrayChange tried to clone the data...
}

void Action::recordNoteExistenceChange(InstrumentClip* clip, int noteRowId, Note* note, int type) {

	if (containsConsequenceNoteArrayChange(clip, noteRowId)) return;

	void* consMemory = generalMemoryAllocator.alloc(sizeof(ConsequenceNoteExistence));

	if (consMemory) {
		ConsequenceNoteExistence* newConsequence =
		    new (consMemory) ConsequenceNoteExistence(clip, noteRowId, note, type);
		addConsequence(newConsequence);
	}
}

void Action::recordClipInstanceExistenceChange(Output* output, ClipInstance* clipInstance, int type) {

	void* consMemory = generalMemoryAllocator.alloc(sizeof(ConsequenceClipInstanceExistence));

	if (consMemory) {
		ConsequenceClipInstanceExistence* newConsequence =
		    new (consMemory) ConsequenceClipInstanceExistence(output, clipInstance, type);
		addConsequence(newConsequence);
	}
}

void Action::recordClipLengthChange(Clip* clip, int32_t oldLength) {

	// Check we don't already have a Consequence for this Clip's length
	for (Consequence* cons = firstConsequence; cons; cons = cons->next) {
		if (cons->type == CONSEQUENCE_CLIP_LENGTH) {
			ConsequenceClipLength* consequenceClipLength = (ConsequenceClipLength*)cons;
			if (consequenceClipLength->clip == clip) return;
		}
	}

	void* consMemory = generalMemoryAllocator.alloc(sizeof(ConsequenceClipLength));

	if (consMemory) {
		ConsequenceClipLength* consequenceClipLength = new (consMemory) ConsequenceClipLength(clip, oldLength);
		addConsequence(consequenceClipLength);
	}
}

bool Action::recordClipExistenceChange(Song* song, ClipArray* clipArray, Clip* clip, int type) {
	void* consMemory = generalMemoryAllocator.alloc(sizeof(ConsequenceClipExistence));
	if (!consMemory) return false;

	ConsequenceClipExistence* consequence = new (consMemory) ConsequenceClipExistence(clip, clipArray, type);
	if (type == DELETE) {
		char modelStackMemory[MODEL_STACK_MAX_SIZE];
		ModelStack* modelStack = setupModelStackWithSong(modelStackMemory, song);

		consequence->revert(AFTER, modelStack); // Does the actual "deletion" for us
	}
	addConsequence(consequence);

	// For undoing looping stuff, this helps:
	xScrollClip[BEFORE] = 0;
	xScrollClip[AFTER] = 0;

	return true;
}

// Call this *before* you change the Sample or its filePath
void Action::recordAudioClipSampleChange(AudioClip* clip) {
	void* consMemory = generalMemoryAllocator.alloc(sizeof(ConsequenceAudioClipSetSample));
	if (consMemory) {
		ConsequenceAudioClipSetSample* cons = new (consMemory) ConsequenceAudioClipSetSample(clip);
		addConsequence(cons);
	}
}

void Action::updateYScrollClipViewAfter(InstrumentClip* clip) {
	if (!numClipStates) return;

	if (numClipStates
	    != currentSong->sessionClips.getNumElements() + currentSong->arrangementOnlyClips.getNumElements()) {
		numClipStates = 0;
		generalMemoryAllocator.dealloc(clipStates);
		clipStates = NULL;
		Uart::println("discarded clip states");
		return;
	}

	int i = 0;

	// For each Clip in session and arranger
	ClipArray* clipArray = &currentSong->sessionClips;
traverseClips:
	for (int c = 0; c < clipArray->getNumElements(); c++) {
		Clip* thisClip = clipArray->getClipAtIndex(c);

		if (thisClip->type == CLIP_TYPE_INSTRUMENT) {

			if (!clip || thisClip == clip) {
				clipStates[i].yScrollSessionView[AFTER] = ((InstrumentClip*)thisClip)->yScroll;
				break;
			}
		}

		i++;
	}
	if (clipArray != &currentSong->arrangementOnlyClips) {
		clipArray = &currentSong->arrangementOnlyClips;
		goto traverseClips;
	}
}
