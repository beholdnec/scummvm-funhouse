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

				while (true) {
					displayColors(getBOLTMember(_boltlib, 0x0113), stFront, 0);
					displayPic(getBOLTMember(_boltlib, 0x0112), 0, 0, stFront);

					// Process events...
					uint32 eventData = 0;
					int16 eventType = _xp->getEvent(etEmpty, &eventData);

					_xp->updateDisplay();
				}
			}
		}
	}
}

void MerlinEngine::initCallbacks() {
	for (int i = 0; i < ARRAYSIZE(_defaultTypeLoadCallbacks); i++) {
		_defaultTypeLoadCallbacks[i] = noOpCb;
	}

	_defaultTypeLoadCallbacks[2] = swapAllWordsCb;
	_defaultTypeLoadCallbacks[8] = swapSpriteHeaderCb;
	_defaultTypeLoadCallbacks[10] = swapPicHeaderCb;
	_defaultTypeLoadCallbacks[11] = swapAndResolvePicDescCb;
	_defaultTypeLoadCallbacks[12] = swapFirstTwoWordsCb;
	_defaultTypeLoadCallbacks[14] = swapFirstFourWordsCb;

	for (int i = 0; i < ARRAYSIZE(_defaultTypeFreeCallbacks); i++) {
		_defaultTypeFreeCallbacks[i] = noOpCb;
	}

	_defaultTypeFreeCallbacks[8] = freeSpriteCleanUpCb;

	for (int i = 0; i < ARRAYSIZE(_defaultMemberLoadCallbacks); i++) {
		_defaultMemberLoadCallbacks[i] = noOpCb;
	}

	for (int i = 0; i < ARRAYSIZE(_defaultMemberFreeCallbacks); i++) {
		_defaultMemberFreeCallbacks[i] = noOpCb;
	}

	for (int i = 0; i < ARRAYSIZE(_defaultGroupLoadCallbacks); i++) {
		_defaultGroupLoadCallbacks[i] = noOpCb;
	}

	for (int i = 0; i < ARRAYSIZE(_defaultGroupFreeCallbacks); i++) {
		_defaultGroupFreeCallbacks[i] = noOpCb;
	}
	
	_boltCallbacks.typeLoadCallbacks = _defaultTypeLoadCallbacks;
	_boltCallbacks.typeFreeCallbacks = _defaultTypeFreeCallbacks;
	_boltCallbacks.memberLoadCallbacks = _defaultMemberLoadCallbacks;
	_boltCallbacks.memberFreeCallbacks = _defaultMemberFreeCallbacks;
	_boltCallbacks.groupLoadCallbacks = _defaultGroupLoadCallbacks;
	_boltCallbacks.groupFreeCallbacks = _defaultGroupFreeCallbacks;
}

} // End of namespace Merlin

} // End of namespace Bolt
