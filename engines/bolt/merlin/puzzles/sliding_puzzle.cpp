/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "bolt/merlin/merlin.h"

namespace Bolt {

namespace Merlin {

#include "common/pack-start.h"	// START STRUCT PACKING

struct BltSlidingPuzzleDifficultiesDesc;

struct BltSlidingPuzzleDesc {
	// Type 6
	BltPtr<byte> unk0x0;
	BltPtr<BltSlidingPuzzleDifficultiesDesc> difficulties;
} PACKED_STRUCT;

struct BltSlidingPuzzleDifficultiesDesc {
	// Type 44
	uint16 unk0x0;
	uint16 resId;
} PACKED_STRUCT;

struct BltSlidingPuzzleDifficultyDesc {
	// Type 6
	BltPtr<byte> unk0x0;
	BltPtr<BltScene> scene;
} PACKED_STRUCT;

#include "common/pack-end.h"	// END STRUCT PACKING

void MerlinEngine::loadSlidingPuzzle() {
	uint16 mainResId = 0x353F; // TODO: select by challenge index

	getBOLTGroup(_boltlib, mainResId & 0xFF00, 1);
	const BltSlidingPuzzleDesc *mainRes = reinterpret_cast<const BltSlidingPuzzleDesc*>(memberAddr(_boltlib, mainResId));
	const BltSlidingPuzzleDifficultiesDesc *difficulties = getResolved(mainRes->difficulties);

	int difficulty = 0; // TODO: select difficulty by player setting
	uint16 difficultyResId = difficulties[difficulty].resId;
	debug("loading sliding puzzle difficulty res 0x%.04X", (int)difficultyResId);

	getBOLTGroup(_boltlib, difficultyResId & 0xFF00, 1);
	const BltSlidingPuzzleDifficultyDesc *diffRes = reinterpret_cast<const BltSlidingPuzzleDifficultyDesc *>(memberAddr(_boltlib, difficultyResId));
	_slidingPuzzleScene = loadScene(getResolved(diffRes->scene));
	drawScene(_slidingPuzzleScene, 0xff);
}

void MerlinEngine::runSlidingPuzzle() {
	while (!shouldQuit()) {
		uint32 eventData = 0;
		int16 eventType = _xp->getEvent(etEmpty, &eventData);

		switch (eventType) {
		case etMouseMove: {
			int16 x = (int16)(eventData >> 16);
			int16 y = (int16)(eventData & -1);
			updateSceneButtons(_slidingPuzzleScene, x, y, nullptr);
			break;
		}
		case etMouseDown: {
			int16 x = 0;
			int16 y = 0;
			_xp->readCursor(nullptr, &x, &y);
			int8 currButton = -1;
			updateSceneButtons(_slidingPuzzleScene, x, y, &currButton);
			debug("clicked button %d", (int)currButton);
			break;
		}
		}

		_xp->updateDisplay();
	}
}

void MerlinEngine::swapSlidingPuzzleDifficultiesDesc() {
	byte *data = _boltCurrentMemberEntry->dataPtr;
	uint32 decompSize = _boltCurrentMemberEntry->decompSize;
	uint32 offset = 0;
	BltSlidingPuzzleDifficultiesDesc *ptr = reinterpret_cast<BltSlidingPuzzleDifficultiesDesc*>(data);

	while (offset < decompSize) {
		WRITE_UINT16(&ptr->unk0x0, READ_BE_UINT16(&ptr->unk0x0));
		WRITE_UINT16(&ptr->resId, READ_BE_UINT16(&ptr->resId));
		offset += sizeof(BltSlidingPuzzleDifficultiesDesc);
		ptr++;
	}
}
	
} // End of namespace Merlin

} // End of namespace Bolt
