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
		pieceCount[0] = src.getUint16BEAt(0x0);
		difficulty[0] = BltShortId(src.getUint16BEAt(0x2));
		pieceCount[1] = src.getUint16BEAt(0x4);
		difficulty[1] = BltShortId(src.getUint16BEAt(0x6));
		pieceCount[2] = src.getUint16BEAt(0x8);
		difficulty[2] = BltShortId(src.getUint16BEAt(0xA));
	}

	uint16 pieceCount[3];
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

	BltId difficultyId = slidingPuzzleDiffs.difficulty[difficultyLevel];

	BltResourceList difficultyInfo;
	loadBltResourceArray(difficultyInfo, boltlib, difficultyId); // Ex: 3A34, 3B34, 3C34
	BltId sceneId        = difficultyInfo[1].value;
	BltId initialStateId = difficultyInfo[2 + variation].value;
	BltId moveTablesId   = difficultyInfo[6 + variation].value;

	BltU8Values initialState;
	loadBltResourceArray(initialState, boltlib, initialStateId);

	_pieces.alloc(slidingPuzzleDiffs.pieceCount[difficultyLevel]);
	for (int i = 0; i < _pieces.size(); ++i) {
		_pieces[i] = initialState[i].value;
	}

	loadScene(_scene, _game->getEngine(), boltlib, sceneId);

	BltResourceList moveTablesRes;
	loadBltResourceArray(moveTablesRes, boltlib, moveTablesId);
	for (int i = 0; i < kNumButtons * 2; ++i) {
		loadBltResourceArray(_moveTables[i], boltlib, moveTablesRes[i].value);
	}
}

void SlidingPuzzle::enter() {
	_scene.enter();
	setSprites();
	idleMode();
}

BoltRsp SlidingPuzzle::handleMsg(const BoltMsg &msg) {
	_modeCtx.react(msg);
	return kDone;
}

void SlidingPuzzle::setSprites() {
	for (int i = 0; i < _pieces.size(); ++i) {
		_scene.setSpriteImageNum(i, _pieces[i]);
	}

	_scene.redraw();
	_game->getGraphics()->markDirty();
}

void SlidingPuzzle::idleMode() {
	_idleMode = {};
	_idleMode.onMsg([this](const BoltMsg& msg) {
		BoltRsp cmd = _game->handlePopup(&_modeCtx, msg);
		if (cmd != BoltRsp::kPass) {
			return cmd;
		}

		switch (msg.type) {
		case Scene::kClickButton:
			return handleButtonClick(msg.num);
		default:
			return _scene.handleMsg(msg);
		}
	});

	_modeCtx.setNextMode(&_idleMode);
}

BoltRsp SlidingPuzzle::handleButtonClick(int num) {
	if (num >= 0 && num < kNumButtons * 2) {
		ScopedArray<int> oldPieces(_pieces.clone());
		for (uint i = 0; i < _pieces.size(); ++i) {
			_pieces[i] = oldPieces[_moveTables[num][i].value];
		}

		setSprites();

		bool win = true;
		for (uint i = 0; i < _pieces.size(); ++i) {
			if (_pieces[i] != i) {
				win = false;
				break;
			}
		}

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
