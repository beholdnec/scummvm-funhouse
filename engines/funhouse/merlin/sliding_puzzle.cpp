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

struct BltSlidingPuzzle { // type 44
	static const uint32 kType = kBltSlidingPuzzle;
	static const uint kSize = 0xC;
	void load(const ConstSizedDataView<kSize> src, Boltlib &boltlib) {
		numPieces1 = src.readUint16BE(0);
		difficulty1 = BltShortId(src.readUint16BE(2));
		numPieces2 = src.readUint16BE(4);
		difficulty2 = BltShortId(src.readUint16BE(6));
		numPieces3 = src.readUint16BE(8);
		difficulty3 = BltShortId(src.readUint16BE(0xA));
	}

	uint16 numPieces1;
	BltShortId difficulty1;
	uint16 numPieces2;
	BltShortId difficulty2;
	uint16 numPieces3;
	BltShortId difficulty3;
};

void SlidingPuzzle::init(Graphics *graphics, IBoltEventLoop *eventLoop, Boltlib &boltlib, BltId resId) {
    _graphics = graphics;

	BltResourceList resourceList;
	loadBltResourceArray(resourceList, boltlib, resId);
	BltSlidingPuzzle slidingPuzzleInfo;
	loadBltResource(slidingPuzzleInfo, boltlib, resourceList[1].value);

	// TODO: select proper difficulty based on player setting
	BltResourceList difficultyInfo;
	loadBltResourceArray(difficultyInfo, boltlib, slidingPuzzleInfo.difficulty1); // Ex: 3A34, 3B34, 3C34

    _pieces.alloc(slidingPuzzleInfo.numPieces1);
    for (int i = 0; i < slidingPuzzleInfo.numPieces1; ++i) {
        _pieces[i] = i;
    }

	_scene.load(eventLoop, graphics, boltlib, difficultyInfo[1].value);

    BltResourceList moveTablesRes;
    loadBltResourceArray(moveTablesRes, boltlib, difficultyInfo[6].value);
    // FIXME: difficultyInfo[7-9] refers to additional move tables. What are they for?
    for (int i = 0; i < kNumButtons * 2; ++i) {
        loadBltResourceArray(_moveTables[i], boltlib, moveTablesRes[i].value);
    }
}

void SlidingPuzzle::enter() {
	_scene.enter();
    setSprites();
}

BoltCmd SlidingPuzzle::handleMsg(const BoltMsg &msg) {
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
        warning("Piece %d = %d", i, _pieces[i]);
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
    } else if (num != -1) {
        warning("Unhandled button %d", num);
    }

	return BoltCmd::kDone;
}

} // End of namespace Bolt
