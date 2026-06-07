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

		debug("type load callback for 32 = %p", _boltCallbacks.typeLoadCallbacks[32]);
		if (openBOLTLib(&_boltlib, &_boltCallbacks, assetPath("boltlib.blt"))) {
			if (_xp->setDisplaySpec(&_displayMode, &_displaySpecs[1])) {
				setCursorPict(getBOLTMember(_boltlib, 0x9D00));
				_xp->setCursorColor(255, 255, 255);
				_xp->showCursor();
				
				if (!getBOLTGroup(_boltlib, 0x9000, 1))
					return;
				Scene *scene = loadScene(getBOLTMember(_boltlib, 0x900D));

				while (true) {
					//displayColors(getBOLTMember(_boltlib, 0x0113), stFront, 0);
					//displayPic(getBOLTMember(_boltlib, 0x0112), 0, 0, stFront);
					drawScene(scene, 0x20);

					// Process events...
					uint32 eventData = 0;
					int16 eventType = _xp->getEvent(etEmpty, &eventData);

					_xp->updateDisplay();
				}
			}
		}
	}
}

} // End of namespace Merlin

} // End of namespace Bolt
