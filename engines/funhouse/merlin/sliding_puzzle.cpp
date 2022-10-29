/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "funhouse/merlin/sliding_puzzle.h"

namespace Funhouse {

struct BltSlidingPuzzleInfo { // type 43
	static const uint32 kType = kBltSlidingPuzzleInfo;
	static const uint kSize = 0x2;
	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		variationSlot = src.getUint8At(0x1);
	}

	uint8 variationSlot;
};

struct BltSlidingPuzzleDifficulties { // type 44
	static const uint32 kType = kBltSlidingPuzzleDifficulties;
	static const uint kSize = 0xC;
	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		tileCount[0] = src.getUint16BEAt(0x0);
		difficulty[0] = BltShortId(src.getUint16BEAt(0x2));
		tileCount[1] = src.getUint16BEAt(0x4);
		difficulty[1] = BltShortId(src.getUint16BEAt(0x6));
		tileCount[2] = src.getUint16BEAt(0x8);
		difficulty[2] = BltShortId(src.getUint16BEAt(0xA));
	}

	uint16 tileCount[3];
	BltShortId difficulty[3];
};

void SlidingPuzzle::init(MerlinGame *game, Boltlib &boltlib, int challengeIdx) {
	_game = game;

	uint16 resId = 0;
	switch (challengeIdx) {
	case 1: resId = 0x313F; break;
	case 5: resId = 0x353F; break;
	case 8: resId = 0x4140; break;
	case 15: resId = 0x3D3F; break;
	case 22: resId = 0x393F; break;
	case 28: resId = 0x453F; break;
	default: assert(false); break;
	}

	_game->setPopup(MerlinGame::kPuzzlePopup);

	BltResourceList resourceList;
	loadBltResourceArray(resourceList, boltlib, BltShortId(resId));
	BltId infoId = resourceList[0].value;
	BltId puzzleInfoId = resourceList[1].value;

	BltSlidingPuzzleInfo info;
	loadBltResource(info, boltlib, infoId);

	int difficultyLevel = _game->getDifficulty(kLogicDifficulty);
	int variation = (_game->getVariationSlot(info.variationSlot) + 1) % 4;
	debug(3, "Loading sliding puzzle difficulty %d, variation %d", difficultyLevel, variation);

	BltSlidingPuzzleDifficulties slidingPuzzleDiffs;
	loadBltResource(slidingPuzzleDiffs, boltlib, puzzleInfoId);
	int tileCount = slidingPuzzleDiffs.tileCount[difficultyLevel];
	BltId difficultyId = slidingPuzzleDiffs.difficulty[difficultyLevel];

	BltResourceList difficultyInfo;
	loadBltResourceArray(difficultyInfo, boltlib, difficultyId); // Ex: 3A34, 3B34, 3C34
	_solutionTileSprites = loadBltSprites(boltlib, difficultyInfo[0].value);
	BltId sceneId        = difficultyInfo[1].value;
	BltId initialStateId = difficultyInfo[2 + variation].value;
	BltId moveTablesId   = difficultyInfo[6 + variation].value;

	loadBltResourceArray(_initialState, boltlib, initialStateId);

	loadScene(_scene, _game->getEngine(), boltlib, sceneId);

	_tileSprites.reset(new Common::Array<SharedSprite>(tileCount));
	_oldTileSprites.reset(new Common::Array<SharedSprite>(tileCount));
	for (int i = 0; i < tileCount; ++i) {
		(*_tileSprites)[i].reset(new Sprite);
		(*_tileSprites)[i]->pos = (*_solutionTileSprites)[i]->pos;
		(*_oldTileSprites)[i].reset(new Sprite);
		(*_oldTileSprites)[i]->pos = (*_solutionTileSprites)[i]->pos;
	}

	BltResourceList moveTablesRes;
	loadBltResourceArray(moveTablesRes, boltlib, moveTablesId);
	for (int i = 0; i < kMoveCount; ++i) {
		loadBltResourceArray(_moveTables[i], boltlib, moveTablesRes[i].value);
	}

	reset();
}

void SlidingPuzzle::enter() {
	_scene.enter();
	draw();
}

BoltRsp SlidingPuzzle::handleMsg(const BoltMsg &msg) {
	BoltRsp cmd = _game->handlePopup(msg);
	if (cmd != BoltRsp::kPass) {
		return cmd;
	}

	switch (msg.type) {
	case Scene::kClickButton:
		return handleButtonClick(msg.num);
	default:
		return _scene.handleMsg(msg);
	}

	return kDone;
}

void SlidingPuzzle::handleReset() {
	reset();
	// TODO: play reset sound
}

void SlidingPuzzle::handleUndo() {
	// Only one move can be undone. When an undo is requested, the game swaps
	// the current and previous state.
	SWAP(_oldTileSprites, _tileSprites);
	// TODO: play undo sound
	draw();
}

void SlidingPuzzle::reset() {
	for (int i = 0; i < _initialState.size(); ++i) {
		(*_tileSprites)[i]->image = (*_solutionTileSprites)[_initialState[i].value]->image;
		(*_oldTileSprites)[i]->image = (*_solutionTileSprites)[_initialState[i].value]->image;
	}
	_game->setUndoAvailable(false);
	draw();
}

bool SlidingPuzzle::move(int moveIdx) {
	bool win = true;
	for (int i = 0; i < _solutionTileSprites->size(); ++i) {
		int dst = _moveTables[moveIdx][i].value;
		(*_oldTileSprites)[dst]->image = (*_tileSprites)[i]->image;
		if ((*_oldTileSprites)[dst]->image != (*_solutionTileSprites)[dst]->image) {
			win = false;
		}
	}
	SWAP(_oldTileSprites, _tileSprites);
	_game->setUndoAvailable(true);
	draw();
	return win;
}

void SlidingPuzzle::draw() {
	_scene.redraw((SceneDrawFlags)(kDrawBack | kDrawFore | kDrawButtons));
	drawSprites(_game->getEngine()->getGraphics()->getPlaneSurface(kFore), _tileSprites, true, _scene.getOrigin());
}

BoltRsp SlidingPuzzle::handleButtonClick(int num) {
	if (num >= 0 && num < kMoveCount) {
		bool win = move(num);
		if (win) {
			_game->branchWin();
			return BoltRsp::kDone;
		}
	} else if (num != -1) {
		warning("Unhandled button %d", num);
	}

	return BoltRsp::kDone;
}

} // End of namespace Bolt
