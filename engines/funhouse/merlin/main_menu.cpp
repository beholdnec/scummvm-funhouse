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

#include "funhouse/merlin/main_menu.h"

#include "funhouse/merlin/merlin.h"

namespace Funhouse {
	
struct BltMainMenu {
	static const uint32 kType = kBltMainMenu;
	static const uint32 kSize = 0xC;
	void load(Common::Span<const byte> src, Boltlib &bltFile) {
		sceneId = BltId(src.getUint32BEAt(0));
		colorbarsImageId = BltId(src.getUint32BEAt(4));
		colorbarsPaletteId = BltId(src.getUint32BEAt(8));
	}

	BltId sceneId;
	BltId colorbarsImageId;
	BltId colorbarsPaletteId;
};

void MainMenu::init(MerlinGame *game, Boltlib &boltlib, BltId resId) {
	_game = game;

	BltMainMenu mainMenu;
	loadBltResource(mainMenu, boltlib, resId);
	loadScene(_scene, _game->getEngine(), boltlib, mainMenu.sceneId);
}

void MainMenu::enter() {
  _scene.enter();
}

BoltRsp MainMenu::handleMsg(const BoltMsg &msg) {
	if (msg.type == Scene::kClickButton) {
		return handleButtonClick(msg.num);
	}

	return _scene.handleMsg(msg);
}

BoltRsp MainMenu::handleButtonClick(int num) {
	switch (num) {
	case -1: // No button
		return BoltRsp::kDone;
	case 0: // Play
		_game->branchScript(0);
		return BoltRsp::kDone;
	case 1: // Credits
		_game->startMAMovie(MKTAG('C', 'R', 'D', 'T'));
		return BoltRsp::kDone;
	case 4: // Tour
		_game->startMAMovie(MKTAG('T', 'O', 'U', 'R'));
		return BoltRsp::kDone;
	case 5: // Flower
		// XXX: go to freeplay mode. TODO: original game shows colorbars when cheat mode is active.
		_game->branchScript(8, true);
		return BoltRsp::kDone;
	default:
		warning("unknown main menu button %d", num);
		return BoltRsp::kDone;
	}
}

} // End of namespace Funhouse
