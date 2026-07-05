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

struct BltActionPuzzleDesc {
	// Type 6
	BltPtr<uint16> difficulties;
	BltPtr<byte> unk0x4;
	BltPtr<byte> image;
	BltPtr<byte> palette;
} PACKED_STRUCT;

#include "common/pack-end.h"	// END STRUCT PACKING

void MerlinEngine::loadActionPuzzle() {
	uint16 mainResId = 0x4921; // TODO: select by challenge index

	getBOLTGroup(_boltlib, mainResId & 0xFF00, 1);
	const BltActionPuzzleDesc *mainRes = reinterpret_cast<const BltActionPuzzleDesc*>(memberAddr(_boltlib, mainResId));
	const uint16 *difficulties = getResolved(mainRes->difficulties);

	int difficulty = 0; // TODO: select difficulty by player setting
	uint16 difficultyResId = difficulties[difficulty];
	debug("loading action puzzle difficulty res 0x%.04X", (int)difficultyResId);

	//getBOLTGroup(_boltlib, difficultyResId & 0xFF00, 1);
	//const BltSlidingPuzzleDifficultyDesc *diffRes = reinterpret_cast<const BltSlidingPuzzleDifficultyDesc *>(memberAddr(_boltlib, difficultyResId));
	//_slidingPuzzleScene = loadScene(getResolved(diffRes->scene));
	//drawScene(_slidingPuzzleScene, 0xff);

	displayColors(getResolved(mainRes->palette), 0, 0);
	displayPic(getResolved(mainRes->image), 0, 0, 0);
}
	
} // End of namespace Merlin

} // End of namespace Bolt
