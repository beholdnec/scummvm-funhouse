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
	setButtons();
}

BoltCmd FileMenu::handleMsg(const BoltMsg &msg) {
	if (msg.type == Scene::kClickButton) {
		return handleButtonClick(msg.num);
	}

	return _scene.handleMsg(msg);
}

static const int kFirstFileButton = 4;
static const int kNumFiles = 12;

BoltCmd FileMenu::handleButtonClick(int num) {
	
	if (num >= kFirstFileButton && num < kFirstFileButton + kNumFiles) {
		_game->setFile(num - kFirstFileButton);
		setButtons();
		return BoltCmd::kDone;
	} else {
		switch (num) {
		case -1: // No button
			return BoltCmd::kDone;
		case 1: // Play
			if (_game->getFile() != -1) {
				return Card::kEnd;
			} else {
				return BoltCmd::kDone;
			}
		default:
			warning("unknown main menu button %d", num);
			return BoltCmd::kDone;
		}
	}
}

void FileMenu::setButtons() {
	for (int i = 0; i < kNumFiles; ++i) {
		_scene.getButton(kFirstFileButton + i).setGraphics(i == _game->getFile() ? 1 : 0);
	}

	static const int kPlayButton = 1;
	if (_game->getFile() != -1) {
		_scene.getButton(kPlayButton).setGraphics(1);
	}

	_scene.redraw();
}

} // End of namespace Funhouse
