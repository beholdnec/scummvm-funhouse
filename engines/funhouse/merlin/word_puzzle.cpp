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

#include "funhouse/merlin/word_puzzle.h"

namespace Funhouse {
	
struct BltWordPuzzleInfo {
    static const uint32 kType = kBltWordPuzzleInfo;
    static const uint kSize = 0x4;
    void load(Common::Span<const byte> src, Boltlib &boltlib) {
		numChars = src.getUint8At(0);
		numLines = src.getUint8At(1);
		// TODO: more fields
    }

	uint8 numChars;
	uint8 numLines;
};

void WordPuzzle::init(MerlinGame *game, Boltlib &boltlib, BltId resId) {
    _game = game;

    _popup.init(_game, boltlib, _game->getPopupResId(MerlinGame::kPuzzlePopup));

	BltResourceList resourceList;
	loadBltResourceArray(resourceList, boltlib, resId);
    BltId difficultiesId          = resourceList[0].value;
	BltId normalSpriteListId      = resourceList[2].value;
	BltId highlightedSpriteListId = resourceList[3].value;
	BltId selectedSpriteListId    = resourceList[4].value;

	_normalSprites.load(boltlib, normalSpriteListId);
	_highlightedSprites.load(boltlib, highlightedSpriteListId);
	_selectedSprites.load(boltlib, selectedSpriteListId);

	BltU16Values difficulties;
	loadBltResourceArray(difficulties, boltlib, difficultiesId);
	// TODO: Pick difficulty 0-2 here
    BltId difficultyId = BltShortId(difficulties[0].value); // Ex: 5E18

	int puzzleVariant = 0; // TODO: Choose puzzle variant 0-3

	BltResourceList difficulty;
	loadBltResourceArray(difficulty, boltlib, difficultyId);
	BltId infoId        = difficulty[puzzleVariant].value; // Ex: 5E00
	BltId lineLengthsId = difficulty[4 + puzzleVariant].value; // Ex: 5E01
	BltId solutionId    = difficulty[12 + puzzleVariant].value; // Ex: 5E03
    BltId sceneId       = difficulty[16 + puzzleVariant].value; // Ex: 5E05

	_scene.load(_game->getEngine(), boltlib, sceneId);
}

void WordPuzzle::enter() {
	_scene.enter();
}

BoltCmd WordPuzzle::handleMsg(const BoltMsg &msg) {
    BoltCmd cmd = _popup.handleMsg(msg);
    if (cmd.type != BoltCmd::kPass) {
        return cmd;
    }

	if (msg.type == Scene::kClickButton) {
		return handleButtonClick(msg.num);
	}

	return _scene.handleMsg(msg);
}

BoltCmd WordPuzzle::handleButtonClick(int num) {
	debug(3, "Clicked button %d", num);
	// TODO: implement puzzle
	if (num != -1) {
		return Card::kWin;
	}

	return BoltCmd::kDone;
}

} // End of namespace Funhouse
