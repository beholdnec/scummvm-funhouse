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

#include "funhouse/merlin/file_menu.h"
#include "funhouse/merlin/merlin.h"

namespace Funhouse {

struct BltFileMenu {
	static const uint32 kType = kBltFileMenu;
	static const uint kSize = 0xA6;
	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		sceneId = BltId(src.getUint32BEAt(0));
	}
	
	BltId sceneId;
};

void FileMenu::init(MerlinGame *game, Boltlib &boltlib, BltId resId) {
	_game = game;

	BltFileMenu fileMenu;
	loadBltResource(fileMenu, boltlib, resId);

	loadScene(_scene, _game->getEngine(), boltlib, fileMenu.sceneId);
}

void FileMenu::enter() {
	_scene.enter();
}

BoltCmd FileMenu::handleMsg(const BoltMsg &msg) {
	if (msg.type == Scene::kClickButton) {
		return handleButtonClick(msg.num);
	}

	return _scene.handleMsg(msg);
}

BoltCmd FileMenu::handleButtonClick(int num) {
	static const int kFirstFileButton = 4;
	static const int kNumFiles = 12;
	
	if (num >= kFirstFileButton && num < kFirstFileButton + kNumFiles) {
		int fileNum = num - kFirstFileButton;
		for (int i = 0; i < kNumFiles; ++i) {
			_scene.getButton(kFirstFileButton + i).setGraphics(i == fileNum ? 1 : 0);
		}
		_scene.redraw();
		return BoltCmd::kDone;
	} else {
		switch (num) {
		case -1: // No button
			return Card::kEnd;
		default:
			warning("unknown main menu button %d", num);
			return BoltCmd::kDone;
		}
	}
}

} // End of namespace Funhouse
