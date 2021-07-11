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
        cheatCodeId = BltId(src.getUint32BEAt(0xa0));
        cheatSoundId = BltShortId(src.getUint16BEAt(0xa4));
	}
	
	BltId sceneId;
    BltId cheatCodeId;
    BltId cheatSoundId;
};

void FileMenu::init(MerlinGame *game, Boltlib &boltlib, BltId resId) {
	_game = game;

	BltFileMenu fileMenu;
	loadBltResource(fileMenu, boltlib, resId);

	loadScene(_scene, _game->getEngine(), boltlib, fileMenu.sceneId);

    // Resource 0275 contains the cheat code, which is entered by selecting
    // file icons in a special sequence.
    // If a mistake is made while entering the code, the player must start over
    // by leaving and re-entering the file menu.
    // The cheat code is (starting from 1 at the top left):
    // 11, 12, 4, 3, 8, 6, 12, 3, 6, 9, 5, 2, 10, 1, 7
    loadBltResourceArray(_cheatCode, boltlib, fileMenu.cheatCodeId);
    _cheatIndex = 0;
    _cheatSound.load(boltlib, fileMenu.cheatSoundId);
}

void FileMenu::enter() {
	_scene.enter();
	setButtons();
}

BoltRsp FileMenu::handleMsg(const BoltMsg &msg) {
	if (msg.type == Scene::kClickButton) {
		return handleButtonClick(msg.num);
	}

	return _scene.handleMsg(msg);
}

static const int kFirstFileButton = 4;

BoltRsp FileMenu::handleButtonClick(int num) {
    if (!_game->getCheatMode() && _cheatIndex != -1) {
        if (num == _cheatCode[_cheatIndex].value) {
            ++_cheatIndex;
            if (_cheatCode[_cheatIndex].value == 0) {
                warning("Cheat mode activated");
                _game->setCheatMode(true);
                _cheatIndex = -1;
                _cheatSound.play(_game->getEngine()->_mixer);
            }
        }
        else {
            _cheatIndex = -1;
        }
    }

	if (num >= kFirstFileButton && num < kFirstFileButton + kProfileCount) {
		_game->selectProfile(num - kFirstFileButton);
		setButtons();
		return BoltRsp::kDone;
	} else {
		switch (num) {
		case -1: // No button
			return BoltRsp::kDone;
		case 1: // Play
			if (_game->getProfile() != -1) {
				if (_game->doesProfileExist(_game->getProfile())) {
					_game->branchLoadProfile();
				}
				else {
					_game->branchScript(2); // Difficulty menu
				}
			}
			return BoltRsp::kDone;
		default:
			warning("unknown main menu button %d", num);
			return BoltRsp::kDone;
		}
	}
}

void FileMenu::setButtons() {
	for (int i = 0; i < kProfileCount; ++i) {
		_scene.getButton(kFirstFileButton + i).setGraphics(i == _game->getProfile() ? 1 : 0);
	}

	static const int kPlayButton = 1;
	if (_game->getProfile() != -1) {
		_scene.getButton(kPlayButton).setGraphics(1);
	}

	_scene.redraw();
}

} // End of namespace Funhouse
