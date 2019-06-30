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

struct BltSlidingPuzzleInfo { // type 44
	static const uint32 kType = kBltSlidingPuzzle;
	static const uint kSize = 0xC;
	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		numPieces1 = src.getUint16BEAt(0);
		difficulty1 = BltShortId(src.getUint16BEAt(2));
		numPieces2 = src.getUint16BEAt(4);
		difficulty2 = BltShortId(src.getUint16BEAt(6));
		numPieces3 = src.getUint16BEAt(8);
		difficulty3 = BltShortId(src.getUint16BEAt(0xA));
	}

	uint16 numPieces1;
	BltShortId difficulty1;
	uint16 numPieces2;
	BltShortId difficulty2;
	uint16 numPieces3;
	BltShortId difficulty3;
};

void SlidingPuzzle::init(MerlinGame *game, Boltlib &boltlib, BltId resId) {
    _game = game;
    _graphics = _game->getGraphics();

    _popup.init(_game->getEventLoop(), _graphics, boltlib,
        _game->getPopupResId(MerlinGame::kPuzzlePopup));

	BltResourceList resourceList;
	loadBltResourceArray(resourceList, boltlib, resId);
    BltId puzzleInfoId = resourceList[1].value;

	BltSlidingPuzzleInfo slidingPuzzleInfo;
	loadBltResource(slidingPuzzleInfo, boltlib, puzzleInfoId);

	// TODO: select proper difficulty based on player setting
	BltResourceList difficultyInfo;
	loadBltResourceArray(difficultyInfo, boltlib, slidingPuzzleInfo.difficulty1); // Ex: 3A34, 3B34, 3C34
    BltId sceneId        = difficultyInfo[1].value;
    BltId initialStateId = difficultyInfo[2].value;
    BltId moveTablesId   = difficultyInfo[6].value;

    // FIXME: difficultyInfo[3-5] are probably more initial state tables.
    BltU8Values initialState;
    loadBltResourceArray(initialState, boltlib, initialStateId);

    _pieces.alloc(slidingPuzzleInfo.numPieces1);
    for (int i = 0; i < slidingPuzzleInfo.numPieces1; ++i) {
        _pieces[i] = initialState[i].value;
    }

	_scene.load(_game->getEventLoop(), _graphics, boltlib, sceneId);

    BltResourceList moveTablesRes;
    loadBltResourceArray(moveTablesRes, boltlib, moveTablesId);
    // FIXME: difficultyInfo[7-9] are more move tables. What are they for?
    for (int i = 0; i < kNumButtons * 2; ++i) {
        loadBltResourceArray(_moveTables[i], boltlib, moveTablesRes[i].value);
    }
}

void SlidingPuzzle::enter() {
	_scene.enter();
    setSprites();
}

BoltCmd SlidingPuzzle::handleMsg(const BoltMsg &msg) {
    BoltCmd cmd = _popup.handleMsg(msg);
    if (cmd.type != BoltCmd::kPass) {
        return cmd;
    }

	if (msg.type == Scene::kClickButton) {
		return handleButtonClick(msg.num);
	}

    if (msg.type == BoltMsg::kRightClick) {
        // XXX: win instantly. TODO: remove.
        return Card::kWin;
    }

	return _scene.handleMsg(msg);
}

void SlidingPuzzle::setSprites() {
    for (int i = 0; i < _pieces.size(); ++i) {
        _scene.getSprites().setSpriteImage(i, _scene.getSprites().getImageFromSet(_pieces[i]));
    }

    _scene.redrawSprites();
    _graphics->markDirty();
}

BoltCmd SlidingPuzzle::handleButtonClick(int num) {
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
            return kWin;
        }
    } else if (num != -1) {
        warning("Unhandled button %d", num);
    }

	return BoltCmd::kDone;
}

} // End of namespace Bolt
