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

void WordPuzzle::init(MerlinGame *game, Boltlib &boltlib, BltId resId) {
    _game = game;

    _popup.init(_game, boltlib, _game->getPopupResId(MerlinGame::kPuzzlePopup));

	BltResourceList resourceList;
	loadBltResourceArray(resourceList, boltlib, resId);
    BltId difficultiesId = resourceList[0].value;

	BltU16Values difficulties;
	loadBltResourceArray(difficulties, boltlib, difficultiesId);
    BltId difficultyId = BltShortId(difficulties[0].value);

	// There are three difficulties, choose one here
	BltResourceList difficulty;
	loadBltResourceArray(difficulty, boltlib, difficultyId); // Difficulty 0
    BltId sceneId = difficulty[19].value;

	_scene.load(_game->getEventLoop(), _game->getGraphics(), boltlib, sceneId);
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
