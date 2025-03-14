/*
 * Copyright © 2014-2023 Synthstrom Audible Limited
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
#include <AudioFileManager.h>
#include "SaveSongUI.h"
#include "functions.h"
#include "lookuptables.h"
#include "numericdriver.h"
#include <string.h>
#include <SaveSongOrInstrumentContextMenu.h>
#include "Sample.h"
#include "uart.h"
#include "AudioRecorder.h"
#include "View.h"
#include "storagemanager.h"
#include "ContextMenuOverwriteFile.h"
#include "song.h"
#include "PadLEDs.h"
#include "IndicatorLEDs.h"
#include "Buttons.h"
#include "extern.h"

#if HAVE_OLED
#include "oled.h"
#endif

extern "C" {
#include "ff.h"
}

extern uint8_t currentlyAccessingCard;

SaveSongUI saveSongUI;

SaveSongUI::SaveSongUI() {
	filePrefix = "SONG";
#if HAVE_OLED
	title = "Save song";
#endif
}

bool SaveSongUI::opened() {
	instrumentTypeToLoad = 255;

	// Grab screenshot of song, for saving, before qwerty drawn
	memcpy(PadLEDs::imageStore, PadLEDs::image, sizeof(PadLEDs::image));

	bool success = SaveUI::opened(); // Clears enteredText
	if (!success) {                  // In this case, an error will have already displayed.
doReturnFalse:
		renderingNeededRegardlessOfUI(); // Because unlike many UIs we've already gone and drawn the QWERTY interface on the pads.
		return false;
	}

	int error;

	String searchFilename;
	searchFilename.set(&currentSong->name);
	if (!searchFilename.isEmpty()) {
		error = searchFilename.concatenate(".XML");
		if (error) {
gotError:
			numericDriver.displayError(error);
			goto doReturnFalse;
		}
	}
	currentFolderIsEmpty = false;

	currentDir.set(&currentSong->dirPath);

	error = arrivedInNewFolder(0, searchFilename.get(), "SONGS");
	if (error) goto gotError;

	// TODO: create folder if doesn't exist.

	enteredTextEditPos = enteredText.getLength();

	IndicatorLEDs::setLedState(synthLedX, synthLedY, false);
	IndicatorLEDs::setLedState(kitLedX, kitLedY, false);
	IndicatorLEDs::setLedState(midiLedX, midiLedY, false);

	IndicatorLEDs::setLedState(crossScreenEditLedX, crossScreenEditLedY, false);
	IndicatorLEDs::setLedState(clipViewLedX, clipViewLedY, false);
	IndicatorLEDs::setLedState(scaleModeLedX, scaleModeLedY, false);

	IndicatorLEDs::blinkLed(sessionViewLedX, sessionViewLedY);

	focusRegained();
	return true;
}

void SaveSongUI::focusRegained() {
	collectingSamples = false;
	return SaveUI::focusRegained();
}

bool SaveSongUI::performSave(bool mayOverwrite) {

	if (ALPHA_OR_BETA_VERSION && currentlyAccessingCard) {
		numericDriver.freezeWithError("E316");
	}

	if (currentSong->hasAnyPendingNextOverdubs()) {
		numericDriver.displayPopup(HAVE_OLED ? "Can't save while overdubs pending" : "CANT");
		return false;
	}

#if HAVE_OLED
	OLED::displayWorkingAnimation("Saving");
#else
	numericDriver.displayLoadingAnimation();
#endif

	String filePath;
	int error = getCurrentFilePath(&filePath);
	if (error) {
gotError:
#if HAVE_OLED
		OLED::removeWorkingAnimation();
#else
		numericDriver.removeTopLayer(); // Removes loading animation if it's still there
#endif
		numericDriver.displayError(error);
		return false;
	}

	bool fileAlreadyExisted = storageManager.fileExists(filePath.get());

	if (!mayOverwrite && fileAlreadyExisted) {
		contextMenuOverwriteFile.currentSaveUI = this;

		bool available = contextMenuOverwriteFile.setupAndCheckAvailability();

		if (available) { // Always true.
#if HAVE_OLED
			OLED::removeWorkingAnimation();
#endif
			numericDriver.setNextTransitionDirection(1);
			openUI(&contextMenuOverwriteFile);
			return true;
		}
		else {
			error = ERROR_UNSPECIFIED;
			goto gotError;
		}
	}

	// We might want to copy some samples around - either because we're "collecting" them to a folder, or because they were loaded in from a collected folder and we now need to
	// put them in the main samples folder.

	// Create sample dir
	String newSongAlternatePath;

	String filenameWithoutExtension;
	error = getCurrentFilenameWithoutExtension(&filenameWithoutExtension);
	if (error) goto gotError;

	error =
	    audioFileManager.setupAlternateAudioFileDir(&newSongAlternatePath, currentDir.get(), &filenameWithoutExtension);
	if (error) goto gotError;
	error = newSongAlternatePath.concatenate("/");
	if (error) goto gotError;
	int dirPathLengthNew = newSongAlternatePath.getLength();

	bool anyErrorMovingTempFiles = false;

	// Go through each AudioFile we have a record of in RAM.
	for (int i = 0; i < audioFileManager.audioFiles.getNumElements(); i++) {
		AudioFile* audioFile = (AudioFile*)audioFileManager.audioFiles.getElement(i);

		// If this AudioFile is used in this Song...
		if (audioFile->numReasonsToBeLoaded) {

			if (audioFile->type == AUDIO_FILE_TYPE_SAMPLE) {
				// If this is a recording which still exists at its temporary location, move the file
				if (!((Sample*)audioFile)->tempFilePathForRecording.isEmpty()) {
					FRESULT result =
					    f_rename(((Sample*)audioFile)->tempFilePathForRecording.get(), audioFile->filePath.get());
					if (result == FR_OK) {
						((Sample*)audioFile)->tempFilePathForRecording.clear();
					}
					else {
						// We at least need to warn the user that although the main file save was (hopefully soon to be)
						// successful, something's gone wrong
						anyErrorMovingTempFiles = true;
						/*
						Uart::print("rename failed. ");
						Uart::println(result);
						Uart::println(((Sample*)sample)->tempFilePathForRecording.get());
						Uart::println(sample->filePath.get());
						*/
					}
				}
			}

			// Or if file needs copying either to or from an "alt" location - either because we're doing a "collect media" or importing from such a folder.
			// Crucial obscure combination - we could be doing a "collect media" *AND ALSO* have moved (or even failed to move!) a recorded file
			// from its "temp" location above
			if (collectingSamples || !audioFile->loadedFromAlternatePath.isEmpty()) {

				// If saving as *same* song name/slot, collecting samples, and it already came from alt location, no need to do it again
				if (collectingSamples && !audioFile->loadedFromAlternatePath.isEmpty()) {
					if (currentDir.equalsCaseIrrespective(&currentSong->dirPath)) {
						if (enteredText.equalsCaseIrrespective(&currentSong->name)) continue;
					}
				}

				char const* sourceFilePath;

				// Sort out source file path
				if (!audioFile->loadedFromAlternatePath.isEmpty()) {
					// If we loaded the file from an alternate path originally, well we saved that exact path just so we can recall it here!
					sourceFilePath = audioFile->loadedFromAlternatePath.get();
				}
				else {
					sourceFilePath =
					    (audioFile->type != AUDIO_FILE_TYPE_SAMPLE
					     || ((Sample*)audioFile)->tempFilePathForRecording.isEmpty())
					        ? audioFile->filePath.get()
					        : ((Sample*)audioFile)
					              ->tempFilePathForRecording
					              .get(); // It may still have a temp path if for some reason we failed to move it, above
				}

				// Note: we can't just use the clusters to write back to the card, cos these might contain data that we converted

				// Open file to read
				FRESULT result = f_open(&fileSystemStuff.currentFile, sourceFilePath, FA_READ);
				if (result != FR_OK) {
					Uart::println("open fail");
					Uart::println(sourceFilePath);
					error = ERROR_UNSPECIFIED;
					goto gotError;
				}

				char const* destFilePath;
				char const* normalFilePath; // Just briefly stores a thing below

				bool needToPretendLoadedAlternate = false;

				// Sort out destination file path
				if (collectingSamples) {

					// If this sample is a "recording", we need to append a random string on the end
					// NOTE: I guess this would do this multiple times if we kept re-saving... probably not the end of the world?
					normalFilePath = audioFile->filePath.get();
					if (!memcasecmp(normalFilePath, "SAMPLES/RECORD/REC", 18)
					    || !memcasecmp(normalFilePath, "SAMPLES/RESAMPLE/REC", 20)
					    || !memcasecmp(normalFilePath, "SAMPLES/CLIPS/REC", 17)) {
						char const* slashAddr = strrchr(normalFilePath, '/');
						int slashPos = (uint32_t)slashAddr - (uint32_t)normalFilePath;

						int fileNamePos = slashPos + 1;

						if (audioFile->filePath.getLength() - fileNamePos == 12
						    && !strcasecmp(normalFilePath + fileNamePos + 8, ".WAV")) {

							// Generate random string, with _ at start and .WAV at end
							char buffer[11];
							buffer[0] = '_';
							buffer[6] = '.';
							buffer[7] = 'W';
							buffer[8] = 'A';
							buffer[9] = 'V';
							buffer[10] = 0;

							seedRandom();

							for (int i = 1; i < 6; i++) {
								int rand = random(35);
								if (rand < 10) buffer[i] = '0' + rand;
								else buffer[i] = 'A' + (rand - 10);
							}

							// Append that random string
							audioFile->filePath.concatenateAtPos(buffer, fileNamePos + 8);

							// And because the AudioFile in memory is now associated with a file name which only exists in the "alternative location",
							// we need to mark it as if it was loaded from there, so any future copying of that file will treat it correctly
							// - particularly if the user does another collect-media save over this one, meaning the file should not be copied again.
							needToPretendLoadedAlternate =
							    true; // We don't have that alternate path yet, so just make a note to do it below.
						}
					}

					// Normally, the filePath will be in the SAMPLES folder, which our name-condensing system was designed for...
					if (!memcasecmp(audioFile->filePath.get(), "SAMPLES/", 8)) {
						error = audioFileManager.setupAlternateAudioFilePath(&newSongAlternatePath, dirPathLengthNew,
						                                                     &audioFile->filePath);
						if (error) {
failAfterOpeningSourceFile:
							f_close(&fileSystemStuff.currentFile); // Close source file
							goto gotError;
						}
					}

					// Or, if it wasn't in the SAMPLES folder, e.g. if it was in a dedicated SYNTH folder, then we have to just use the original filename, and hope
					// it doesn't clash with anything.
					else {
						char const* fileName = getFileNameFromEndOfPath(audioFile->filePath.get());
						error = newSongAlternatePath.concatenateAtPos(fileName, dirPathLengthNew);
						if (error) goto failAfterOpeningSourceFile;
					}

					if (needToPretendLoadedAlternate) {
						audioFile->loadedFromAlternatePath.set(&newSongAlternatePath);
					}

					destFilePath = newSongAlternatePath.get();
				}
				else {
					destFilePath = audioFile->filePath.get();
				}

				// Create file to write
				error = storageManager.createFile(&recorderFileSystemStuff.currentFile, destFilePath, false);
				if (error == ERROR_FILE_ALREADY_EXISTS) {
				} // No problem - the audio file was already there from before, so we don't need to copy it again now.
				else if (error) goto failAfterOpeningSourceFile;

				// Or if everything's fine and we're ready to write / copy...
				else {

					// Copy
					while (true) {
						UINT bytesRead;
						result = f_read(&fileSystemStuff.currentFile, storageManager.fileClusterBuffer,
						                audioFileManager.clusterSize, &bytesRead);
						if (result) {
							Uart::println("read fail");
fail3:
							f_close(&recorderFileSystemStuff.currentFile);
							error = ERROR_UNSPECIFIED;
							goto failAfterOpeningSourceFile;
						}
						if (!bytesRead) break; // Stop, on rare case where file ended right at end of last cluster

						UINT bytesWritten;
						result = f_write(&recorderFileSystemStuff.currentFile, storageManager.fileClusterBuffer,
						                 bytesRead, &bytesWritten);
						if (result || bytesWritten != bytesRead) {
							Uart::println("write fail");
							Uart::println(result);
							goto fail3;
						}

						if (bytesRead < audioFileManager.clusterSize)
							break; // Stop - file clearly ended part-way through cluster
					}

					f_close(&recorderFileSystemStuff.currentFile); // Close destination file
				}

				f_close(&fileSystemStuff.currentFile); // Close source file

				// Write has succeeded. We can mark it as existing in its normal main location (e.g. in the SAMPLES folder).
				// Unless we were collection media, in which case it won't be there - it'll be in the new alternate location we put it in.
				if (!collectingSamples) audioFile->loadedFromAlternatePath.clear();
			}
		}
	}

	String filePathDuringWrite;

	// If we're overwriting an existing file, we'll write to a temp file first. Find one that doesn't already exist
	if (fileAlreadyExisted) {

		int tempFileNumber = 0;

		while (true) {
			error = filePathDuringWrite.set("SONGS/TEMP");
			if (error) goto gotError;
			error = filePathDuringWrite.concatenateInt(tempFileNumber, 4);
			if (error) goto gotError;
			error = filePathDuringWrite.concatenate(".XML");
			if (error) goto gotError;

			if (!storageManager.fileExists(filePathDuringWrite.get())) break;

			tempFileNumber++;
		}
	}
	else {
		filePathDuringWrite.set(&filePath);
	}

	Uart::print("creating: ");
	Uart::println(filePathDuringWrite.get());

	// Write the actual song file
	error = storageManager.createXMLFile(filePathDuringWrite.get(), false);
	if (error) goto gotError;

	// (Sept 2019) - it seems a crash sometimes occurs sometime after this point. A 0-byte file gets created. Could be for either overwriting or not.

	currentSong->writeToFile();

	error = storageManager.closeFileAfterWriting(filePathDuringWrite.get(),
	                                             "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<song\n", "\n</song>\n");
	if (error) goto gotError;

	// If "overwriting an existing file"...
	if (fileAlreadyExisted) {

		// Delete the old file
		FRESULT result = f_unlink(filePath.get());
		if (result != FR_OK) {
cardError:
			error = fresultToDelugeErrorCode(result);
			goto gotError;
		}

		// Rename the new file
		result = f_rename(filePathDuringWrite.get(), filePath.get());
		if (result != FR_OK) goto cardError;
	}

#if HAVE_OLED
	OLED::removeWorkingAnimation();
	char const* message = anyErrorMovingTempFiles ? "Song saved, but error moving temp files" : "Song saved";
	OLED::consoleText(message);
#else
	char const* message = anyErrorMovingTempFiles ? "TEMP" : "DONE";
	numericDriver.displayPopup(message);
#endif

	// Update all of these
	currentSong->name.set(&enteredText);
	currentSong->dirPath.set(&currentDir);

	// While we're at it, save MIDI devices if there's anything new to save.
	MIDIDeviceManager::writeDevicesToFile();

	close();
	return true;
}

#if DELUGE_MODEL == DELUGE_MODEL_40_PAD
int SaveSongUI::padAction(int x, int y, int on) {
	if (sdRoutineLock) ACTION_RESULT_REMIND_ME_OUTSIDE_CARD_ROUTINE;
	close();
	return ACTION_RESULT_DEALT_WITH;
}
#endif
