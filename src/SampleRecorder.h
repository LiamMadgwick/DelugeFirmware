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

#ifndef SAMPLERECORDER_H_
#define SAMPLERECORDER_H_

#include "r_typedefs.h"
#include "DString.h"
#include "AudioSample.h"

extern "C" {
#include "fatfs/ff.h"
}

#define ACTION_REMOVE_RIGHT_CHANNEL 1
#define ACTION_SUBTRACT_RIGHT_CHANNEL 2

#define RECORDER_STATUS_CAPTURING_DATA 0
#define RECORDER_STATUS_CAPTURING_DATA_WAITING_TO_STOP 1
#define RECORDER_STATUS_FINISHED_CAPTURING_BUT_STILL_WRITING 2
#define RECORDER_STATUS_COMPLETE 3
#define RECORDER_STATUS_ABORTED                                                                                        \
	4 // Means RAM error only. SD errors are noted separately and won't affect operation, as long as RAM lasts
#define RECORDER_STATUS_AWAITING_DELETION 5

class Sample;
class Cluster;
class AudioClip;

class SampleRecorder {
public:
	SampleRecorder();
	~SampleRecorder();
	int setup(int newNumChannels, int newMode, bool newKeepingReasons, bool shouldRecordExtraMargins, int newFolderID,
	          int buttonPressLatency);
	void feedAudio(int32_t* inputAddress, int numSamples, bool applyGain = false);
	int cardRoutine();
	void endSyncedRecording(int buttonLatencyForTempolessRecording);
	bool inputLooksDifferential();
	bool inputHasNoRightChannel();
	void abort();

	SampleRecorder* next;

	Sample* sample;

	int32_t numSamplesToRunBeforeBeginningCapturing;
	uint32_t numSamplesBeenRunning;
	uint32_t numSamplesCaptured;

	uint32_t numSamplesExtraToCaptureAtEndSyncingWise;

	int32_t firstUnwrittenClusterIndex;
	int32_t currentRecordClusterIndex;
	Cluster*
	    currentRecordCluster; // Note! If this is NULL, that means that currentRecordClusterIndex refers to a cluster that never got created (cos some error or max file size reached)

	uint32_t audioFileNumber;
	uint8_t folderID;

	char* writePos;
	char* clusterEndPos;

	String
	    filePathCreated; // When this gets set, we add the Sample to the master list. This is stored here in addition to in the Sample,
	                     // so we can delete an aborted file even after the Sample has been detached / destructed.
	                     // This will be the temp file path if there is one.
	uint8_t status;
	uint8_t mode;

	bool
	    haveAddedSampleToArray; // Need to keep track of this, so we know whether to remove it. Well I guess we could just look and see if it's there... but this is nice.

	bool allowFileAlterationAfter;
	bool autoDeleteWhenDone;
	bool keepingReasonsForFirstClusters;
	uint8_t recordingNumChannels;
	bool hadCardError;
	bool reachedMaxFileSize;
	bool recordingExtraMargins;
	bool pointerHeldElsewhere;
	bool capturedTooMuch;

	// Most of these are not captured in the case of BALANCED input for AudioClips
	bool recordingClippedRecently;
	int32_t recordPeakL;
	int32_t recordPeakR;
	int32_t recordPeakLMinusR;
	uint64_t recordSumL;
	uint64_t recordSumR;
	uint64_t recordSumLPlusR;  // L and R are halved before these two are calculated
	uint64_t recordSumLMinusR; // --------

	int32_t recordMax;
	int32_t recordMin;

	uint32_t audioDataLengthBytesAsWrittenToFile;
	uint32_t loopEndSampleAsWrittenToFile;

	int32_t* sourcePos;

	FIL file;

private:
	void setExtraBytesOnPreviousCluster(Cluster* currentCluster, int currentClusterIndex);
	int writeCluster(int32_t clusterIndex, int numBytes);
	int alterFile(int action, int lshiftAmount, uint32_t idealFileSizeBeforeAction, uint64_t dataLengthAfterAction);
	int finalizeRecordedFile();
	int createNextCluster();
	int writeAnyCompletedClusters();
	void finishCapturing();
	void updateDataLengthInFirstCluster(Cluster* cluster);
	void totalSampleLengthNowKnown(uint32_t totalLength, uint32_t loopEndPointSamples = 0);
	void detachSample();
	int truncateFileDownToSize(uint32_t newFileSize);
	int writeOneCompletedCluster();
};

#endif /* SAMPLERECORDER_H_ */
