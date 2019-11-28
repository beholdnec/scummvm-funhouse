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

#include "funhouse/merlin/difficulty_menu.h"
#include "funhouse/merlin/merlin.h"

namespace Funhouse {
    
void DifficultyMenu::init(MerlinGame *game, Boltlib &boltlib, BltId resId) {
	_game = game;

	loadScene(_scene, _game->getEngine(), boltlib, resId);
}

void DifficultyMenu::enter() {
  _scene.enter();
}

BoltCmd DifficultyMenu::handleMsg(const BoltMsg &msg) {
	if (msg.type == Scene::kClickButton) {
		return handleButtonClick(msg.num);
	}

	return _scene.handleMsg(msg);
}

BoltCmd DifficultyMenu::handleButtonClick(int num) {
	// Words
	if (num >= 12 && num <= 14) {
		for (int i = 12; i <= 14; ++i) {
			_scene.setButtonGraphics(i, num == i ? 1 : 0);
		}
		return BoltCmd::kDone;
	}

	// Shapes
	if (num >= 15 && num <= 17) {
		for (int i = 15; i <= 17; ++i) {
			_scene.setButtonGraphics(i, num == i ? 1 : 0);
		}
		return BoltCmd::kDone;
	}

	// Action
	if (num >= 18 && num <= 20) {
		for (int i = 18; i <= 20; ++i) {
			_scene.setButtonGraphics(i, num == i ? 1 : 0);
		}
		return BoltCmd::kDone;
	}

	// Memory
	if (num >= 21 && num <= 23) {
		for (int i = 21; i <= 23; ++i) {
			_scene.setButtonGraphics(i, num == i ? 1 : 0);
		}
		return BoltCmd::kDone;
	}

	// Logic
	if (num >= 24 && num <= 26) {
		for (int i = 24; i <= 26; ++i) {
			_scene.setButtonGraphics(i, num == i ? 1 : 0);
		}
		return BoltCmd::kDone;
	}

	switch (num) {
	case -1: // No button
		return Card::kEnd;
	default:
		warning("unknown main menu button %d", num);
		return BoltCmd::kDone;
	}
}

} // End of namespace Funhouse
