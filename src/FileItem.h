/*
 * Copyright © 2022-2023 Synthstrom Audible Limited
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

#ifndef FILEITEM_H_
#define FILEITEM_H_

#include "r_typedefs.h"
#include "DString.h"
#include "storagemanager.h"

class FileItem {
public:
	FileItem();
	int setupWithInstrument(Instrument* newInstrument, bool hibernating);
	int getFilenameWithExtension(String* filenameWithExtension);
	int getFilenameWithoutExtension(String* filenameWithoutExtension);
	int getDisplayNameWithoutExtension(String* displayNameWithoutExtension);

	char const*
	    displayName; // Usually points to filePointer.get(), but for "numeric" files, will cut off the prefix, e.g. "SONG". And I think this always includes the file extension...
	String filename; // May or may not include file extension. (Or actually I think it always does now...)
	FilePointer filePointer;
	Instrument* instrument;
	bool isFolder;
	bool instrumentAlreadyInSong; // Only valid if instrument is set to something.
	bool filenameIncludesExtension;
};

#endif /* FILEITEM_H_ */
