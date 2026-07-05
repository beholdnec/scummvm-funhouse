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
	
MerlinEngine::MerlinEngine(OSystem *syst, const ADGameDescription *gameDesc)
	: BoltEngine(syst, gameDesc) {
	initCallbacks();
}

void MerlinEngine::boltMain() {
	byte* testAlloc = (byte *)_xp->allocMem(0x100000);
	if (!testAlloc)
		return;

	_xp->freeMem(testAlloc);

	_xp->randomize();

	if (allocResourceIndex()) {
		_boltlib = nullptr;

		if (openBOLTLib(&_boltlib, &_boltCallbacks, assetPath("boltlib.blt"))) {
			if (_xp->setDisplaySpec(&_displayMode, &_displaySpecs[1])) {
				setCursorPict(getBOLTMember(_boltlib, 0x9D00));
				_xp->setCursorColor(255, 255, 255);
				_xp->showCursor();
				_xp->setTransparency(true);

				//loadMainMenu();
				//loadDifficultyMenu();
				//loadSlidingPuzzle();
				//loadActionPuzzle();
				//loadWordPuzzle();
				//loadPotionPuzzle();
				loadMemoryPuzzle();

				while (true) {
					//displayColors(getBOLTMember(_boltlib, 0x0113), stFront, 0);
					//displayPic(getBOLTMember(_boltlib, 0x0112), 0, 0, stBack);
					//drawScene(scene, 0x20);

					//runMainMenu();

					//runDifficultyMenu();
					//runSlidingPuzzle();
					//runWordPuzzle();
					runMemoryPuzzle();

					_xp->updateDisplay();
				}
			}
		}
	}
}

void MerlinEngine::loadMainMenu() {
	getBOLTGroup(_boltlib, 0x0100, 0);
	_mainMenuDesc = memberAddr(_boltlib, 0x0118);
	_mainMenuScene = loadScene((BltScene*)getResolvedPtr(_mainMenuDesc, 0x0));
	drawScene(_mainMenuScene, 0xff);
}

void MerlinEngine::swapMainMenuDesc() {
	byte *data = _boltCurrentMemberEntry->dataPtr;
	uint32 decompSize = _boltCurrentMemberEntry->decompSize;
	uint32 offset = 0;
	byte *ptr = data;

	while (offset < decompSize) {
		resolveIt((uint32 *)ptr);
		resolveIt((uint32 *)(ptr + 0x4));
		resolveIt((uint32 *)(ptr + 0x8));
		offset += 0xc;
		ptr += 0xc;
	}
}

void MerlinEngine::runMainMenu() {
	while (!shouldQuit()) {
		uint32 eventData = 0;
		int16 eventType = _xp->getEvent(etEmpty, &eventData);

		switch (eventType) {
		case etMouseMove:
			int16 x = (int16)(eventData >> 16);
			int16 y = (int16)(eventData & -1);
			updateSceneButtons(_mainMenuScene, x, y, nullptr);
			break;
		}

		_xp->updateDisplay();
	}
}

#include "common/pack-start.h"	// START STRUCT PACKING

struct BltDifficultyMenuDesc {
	// Type 35
	BltPtr<BltScene> scene;
	BltPtr<byte> unk0x4;
	BltPtr<byte> unk0x8;
} PACKED_STRUCT;

#include "common/pack-end.h"	// END STRUCT PACKING

void MerlinEngine::loadDifficultyMenu() {
	getBOLTGroup(_boltlib, 0x0000, 0);
	BltDifficultyMenuDesc* desc = reinterpret_cast<BltDifficultyMenuDesc*>(memberAddr(_boltlib, 0x006E));
	_difficultyMenuScene = loadScene(getResolved(desc->scene));
	drawScene(_difficultyMenuScene, 0xff);
}

void MerlinEngine::runDifficultyMenu() {
	while (!shouldQuit()) {
		uint32 eventData = 0;
		int16 eventType = _xp->getEvent(etEmpty, &eventData);

		switch (eventType) {
		case etMouseMove: {
			int16 x = (int16)(eventData >> 16);
			int16 y = (int16)(eventData & -1);
			updateSceneButtons(_difficultyMenuScene, x, y, nullptr);
			break;
		}
		case etMouseDown: {
			int16 x = 0;
			int16 y = 0;
			_xp->readCursor(nullptr, &x, &y);
			int8 currButton = -1;
			updateSceneButtons(_difficultyMenuScene, x, y, &currButton);
			debug("clicked button %d", (int)currButton);
			if (currButton >= 12) {
				byte category = (currButton - 12) / 3;
				byte level = (currButton - 12) - (category * 3);
				selectDifficulty(category, level);
			}
			if (currButton > -1) {
				drawScene(_difficultyMenuScene, 0x10);
			}
			break;
		}
		}

		_xp->updateDisplay();
	}
}

void MerlinEngine::selectDifficulty(uint8 category, uint8 level) {
	debug("setting difficulty category %d level %d", (int)category, (int)level);
	for (int i = 0; i < 3; i++) {
		setButtonGfx(_difficultyMenuScene, 12 + category * 3 + i, 0);
	}
	setButtonGfx(_difficultyMenuScene, 12 + category * 3 + level, 1);
}

void MerlinEngine::swapDifficultyMenuDesc() {
	byte *data = _boltCurrentMemberEntry->dataPtr;
	uint32 decompSize = _boltCurrentMemberEntry->decompSize;
	uint32 offset = 0;
	BltDifficultyMenuDesc *ptr = reinterpret_cast<BltDifficultyMenuDesc*>(data);

	while (offset < decompSize) {
		resolveIt(&ptr->scene.ptr);
		resolveIt(&ptr->unk0x4.ptr);
		resolveIt(&ptr->unk0x8.ptr);
		offset += sizeof(BltDifficultyMenuDesc);
		ptr++;
	}
}

} // End of namespace Merlin

} // End of namespace Bolt
