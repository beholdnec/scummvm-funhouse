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

#include "bolt/merlin/main_menu.h"

#include "bolt/merlin/merlin.h"

namespace Bolt {
	
struct BltMainMenu {
	static const uint32 kType = kBltMainMenu;
	static const uint32 kSize = 0xC;
	void load(const ConstSizedDataView<kSize> src, Boltlib &bltFile) {
		sceneId = BltId(src.readUint32BE(0));
		colorbarsImageId = BltId(src.readUint32BE(4));
		colorbarsPaletteId = BltId(src.readUint32BE(8));
	}

	BltId sceneId;
	BltId colorbarsImageId;
	BltId colorbarsPaletteId;
};

void MainMenu::init(MerlinGame *game, Graphics *graphics, IBoltEventLoop *eventLoop, Boltlib &boltlib, BltId resId) {
	_game = game;

	BltMainMenu mainMenu;
	loadBltResource(mainMenu, boltlib, resId);
	_scene.load(graphics, boltlib, mainMenu.sceneId);
}

void MainMenu::enter() {
  _scene.enter();
}

BoltCmd MainMenu::handleMsg(const BoltMsg &msg) {
	if (msg.type == BoltMsg::kHover) {
		_scene.handleHover(msg.point);
	} else if (msg.type == BoltMsg::kClick) {
		int num = _scene.getButtonAtPoint(msg.point);
		return handleButtonClick(num);
	}
	return BoltCmd::kDone;
}

BoltCmd MainMenu::handleButtonClick(int num) {
	switch (num) {
	case -1: // No button
		return BoltCmd::kDone;
	case 0: // Play
		return Card::kEnd;
	case 1: // Credits
		_game->startMAMovie(MKTAG('C', 'R', 'D', 'T'));
		return BoltCmd::kDone;
	case 4: // Tour
		_game->startMAMovie(MKTAG('T', 'O', 'U', 'R'));
		return BoltCmd::kDone;
	default:
		warning("unknown main menu button %d", num);
		return BoltCmd::kDone;
	}
}

} // End of namespace Bolt
