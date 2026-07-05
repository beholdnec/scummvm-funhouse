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

struct BltMemoryPuzzleDesc {
	// Type 6
	BltPtr<byte> unk0x0;
	BltPtr<BltScene> scene;
} PACKED_STRUCT;

#include "common/pack-end.h"	// END STRUCT PACKING

void MerlinEngine::loadMemoryPuzzle() {
	uint16 mainResId = 0x865E; // TODO: select by challenge index

	getBOLTGroup(_boltlib, mainResId & 0xFF00, 1);
	const BltMemoryPuzzleDesc *mainRes = reinterpret_cast<const BltMemoryPuzzleDesc*>(memberAddr(_boltlib, mainResId));

	//int difficulty = 0; // TODO: select difficulty by player setting
	//uint16 difficultyResId = difficulties[difficulty];
	//debug("loading word puzzle difficulty res 0x%.04X", (int)difficultyResId);

	//getBOLTGroup(_boltlib, difficultyResId & 0xFF00, 1);
	//const BltWordPuzzleDifficultyDesc *diffRes = reinterpret_cast<const BltWordPuzzleDifficultyDesc *>(memberAddr(_boltlib, difficultyResId));
	_memoryPuzzleScene = loadScene(getResolved(mainRes->scene));
	drawScene(_memoryPuzzleScene, 0xff);
}

void MerlinEngine::runMemoryPuzzle() {
	while (!shouldQuit()) {
		uint32 eventData = 0;
		int16 eventType = _xp->getEvent(etEmpty, &eventData);

		switch (eventType) {
		case etMouseMove: {
			int16 x = (int16)(eventData >> 16);
			int16 y = (int16)(eventData & -1);
			updateSceneButtons(_memoryPuzzleScene, x, y, nullptr);
			break;
		}
		case etMouseDown: {
			int16 x = 0;
			int16 y = 0;
			_xp->readCursor(nullptr, &x, &y);
			int8 currButton = -1;
			updateSceneButtons(_memoryPuzzleScene, x, y, &currButton);
			debug("clicked button %d", (int)currButton);
			break;
		}
		}

		_xp->updateDisplay();
	}
}

} // End of namespace Merlin

} // End of namespace Bolt
