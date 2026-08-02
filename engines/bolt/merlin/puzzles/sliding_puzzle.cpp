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
	uint16 pieceCount;
	uint16 resId;
} PACKED_STRUCT;

struct BltSlidingPuzzleDifficultyDesc {
	// Type 6
	BltPtr<BltSprite> goalSprites;
	BltPtr<BltScene> scene;
	BltPtr<byte> initialStates[4];
	BltPtr<BltPtr<byte>> moveSets[4];
} PACKED_STRUCT;

#include "common/pack-end.h"	// END STRUCT PACKING

struct SlidingPuzzle {
	int8 lastMove;
	Common::Array<BltSprite> currSprites;
	Common::Array<BltSprite> prevSprites;
	Scene *scene;
	uint16 pieceCount;
	BltScene *bltScene;
	BltSprite *goalSprites;
	byte *initialState;
	BltPtr<byte>* moveSet;

	SlidingPuzzle() : lastMove(-1), scene(nullptr), pieceCount(0), bltScene(nullptr), goalSprites(nullptr), initialState(nullptr), moveSet(nullptr) {}
};

void MerlinEngine::loadSlidingPuzzle() {
	uint16 mainResId = 0x353F; // TODO: select by challenge index

	_slidingPuzzle = new SlidingPuzzle;

	getBOLTGroup(_boltlib, mainResId & 0xFF00, 1);
	const BltSlidingPuzzleDesc *mainRes = reinterpret_cast<const BltSlidingPuzzleDesc*>(memberAddr(_boltlib, mainResId));
	const BltSlidingPuzzleDifficultiesDesc *difficulties = getResolved(mainRes->difficulties);

	int difficulty = 0; // TODO: select difficulty by player setting
	uint16 difficultyResId = difficulties[difficulty].resId;
	debug("loading sliding puzzle difficulty res 0x%.04X", (int)difficultyResId);

	_slidingPuzzle->pieceCount = difficulties[difficulty].pieceCount;

	getBOLTGroup(_boltlib, difficultyResId & 0xFF00, 1);
	const BltSlidingPuzzleDifficultyDesc *diffRes = reinterpret_cast<const BltSlidingPuzzleDifficultyDesc *>(memberAddr(_boltlib, difficultyResId));

	int variant = 0; // TODO: randomized value in 0..4 dealt out to each profile
	_slidingPuzzle->bltScene = getResolved(diffRes->scene);
	_slidingPuzzle->goalSprites = getResolved(diffRes->goalSprites);
	_slidingPuzzle->initialState = getResolved(diffRes->initialStates[variant]);
	_slidingPuzzle->moveSet = getResolved(diffRes->moveSets[variant]);

	_slidingPuzzle->currSprites.resize(_slidingPuzzle->pieceCount);
	_slidingPuzzle->prevSprites.resize(_slidingPuzzle->pieceCount);
	loadSlidingPuzzleSpritePositions();

	// TODO: load from save file if data is available
	resetSlidingPuzzle();

	// TODO: move out of this function
	_slidingPuzzle->scene = loadScene(_slidingPuzzle->bltScene);
	drawSlidingPuzzle(true);
}

void MerlinEngine::loadSlidingPuzzleSpritePositions() {
	for (int i = 0; i < _slidingPuzzle->pieceCount; i++) {
		_slidingPuzzle->currSprites[i].x = _slidingPuzzle->goalSprites[i].x;
		_slidingPuzzle->currSprites[i].y = _slidingPuzzle->goalSprites[i].y;
		_slidingPuzzle->prevSprites[i].x = _slidingPuzzle->goalSprites[i].x;
		_slidingPuzzle->prevSprites[i].y = _slidingPuzzle->goalSprites[i].y;
	}
}

void MerlinEngine::runSlidingPuzzle() {
	while (!shouldQuit()) {
		uint32 eventData = 0;
		int16 eventType = _xp->getEvent(etEmpty, &eventData);

		switch (eventType) {
		case etMouseMove: {
			int16 x = (int16)(eventData >> 16);
			int16 y = (int16)(eventData & -1);
			updateSceneButtons(_slidingPuzzle->scene, x, y, nullptr);
			break;
		}
		case etMouseDown: {
			int16 x = 0;
			int16 y = 0;
			_xp->readCursor(nullptr, &x, &y);
			int8 button = -1;
			updateSceneButtons(_slidingPuzzle->scene, x, y, &button);
			debug("clicked button %d", (int)button);
			if (button >= 0) {
				// TODO: play sound
				if (performSlidingPuzzleMove(button)) {
					debug("WIN!!!");
				}
				drawSlidingPuzzlePiecesAndPlaySound();
			} else {
				// TODO: reveal goal
			}
			break;
		}
		}

		_xp->updateDisplay();
	}
}

void MerlinEngine::resetSlidingPuzzle() {
	for (int i = 0; i < _slidingPuzzle->pieceCount; i++) {
		byte initial = _slidingPuzzle->initialState[i];
		_slidingPuzzle->currSprites[i].image = _slidingPuzzle->goalSprites[initial].image;
		_slidingPuzzle->prevSprites[i].image = _slidingPuzzle->goalSprites[initial].image;
	}
}

void MerlinEngine::drawSlidingPuzzle(bool current) {
	if (!current) {
		// TODO: draw goal sprites
	} else {
		drawScene(_slidingPuzzle->scene, 0x20);
		_xp->fillDisplay(0, 0);
		_xp->updateDisplay();
		drawScene(_slidingPuzzle->scene, 0x11);
		_xp->updateDisplay();
		displayColors(getResolved(getResolved(_slidingPuzzle->bltScene->forePlane)->palette), 0, 0);

		const BltSprite *sprite = _slidingPuzzle->currSprites.data();
		for (int i = 0; i < _slidingPuzzle->bltScene->spriteCount; i++) {
			displayPic(getResolved(sprite->image), sprite->x - _slidingPuzzle->bltScene->originX, sprite->y - _slidingPuzzle->bltScene->originY, 0);
			sprite++;
		}
	}
}

void MerlinEngine::drawSlidingPuzzlePiecesAndPlaySound() {
	_slidingPuzzle->scene->overrideSprites = _slidingPuzzle->currSprites.data();
	drawScene(_slidingPuzzle->scene, 8);
	_xp->updateDisplay();
}

bool MerlinEngine::performSlidingPuzzleMove(int8 move) {
	bool win = true;

	Common::Array<BltSprite> sprites = Common::move(_slidingPuzzle->prevSprites);
	byte *moveData = getResolved(_slidingPuzzle->moveSet[move]);
	for (int i = 0; i < _slidingPuzzle->pieceCount; i++) {
		byte target = moveData[i];
		sprites[target].image = _slidingPuzzle->currSprites[i].image;
		if (sprites[target].image != _slidingPuzzle->goalSprites[target].image) {
			win = false;
		}
	}

	_slidingPuzzle->prevSprites = _slidingPuzzle->currSprites;
	_slidingPuzzle->currSprites = sprites;
	_slidingPuzzle->lastMove = move;

	return win;
}

void MerlinEngine::swapSlidingPuzzleDifficultiesDesc() {
	byte *data = _boltCurrentMemberEntry->dataPtr;
	uint32 decompSize = _boltCurrentMemberEntry->decompSize;
	uint32 offset = 0;
	BltSlidingPuzzleDifficultiesDesc *ptr = reinterpret_cast<BltSlidingPuzzleDifficultiesDesc*>(data);

	while (offset < decompSize) {
		WRITE_UINT16(&ptr->pieceCount, READ_BE_UINT16(&ptr->pieceCount));
		WRITE_UINT16(&ptr->resId, READ_BE_UINT16(&ptr->resId));
		offset += sizeof(BltSlidingPuzzleDifficultiesDesc);
		ptr++;
	}
}
	
} // End of namespace Merlin

} // End of namespace Bolt
