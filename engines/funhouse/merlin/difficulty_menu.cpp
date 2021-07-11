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
	setupButtons();
}

BoltRsp DifficultyMenu::handleMsg(const BoltMsg &msg) {
	if (msg.type == Scene::kClickButton) {
		return handleButtonClick(msg.num);
	}

	return _scene.handleMsg(msg);
}

static const int kPlayButton = 3;
static const int kAllBeginnerButton = 4;
static const int kAllAdvancedButton = 5;
static const int kAllExpertButton = 6;
static const int kFirstDifficultyButton = 12;

BoltRsp DifficultyMenu::handleButtonClick(int num) {
    if (num >= kFirstDifficultyButton && num < kFirstDifficultyButton + 3 * kNumDifficultyCategories) {
        DifficultyCategory category = static_cast<DifficultyCategory>((num - kFirstDifficultyButton) / 3);
        int level = (num - kFirstDifficultyButton) % 3;
        _game->setDifficulty(category, level);
        setupButtons();
        return BoltRsp::kDone;
    }

	switch (num) {
	case -1: // No button
		return BoltRsp::kDone;
	case 1: // Game Pieces
		_game->branchScript(1);
		return BoltRsp::kDone;
	case kPlayButton: // Play
		if (isReadyToPlay()) {
			_game->branchScript(2);
			return BoltRsp::kDone;
		}
		return BoltRsp::kDone;
	case kAllBeginnerButton: // Beginner
		setAllDifficulties(0);
		return BoltRsp::kDone;
	case kAllAdvancedButton: // Advanced
		setAllDifficulties(1);
		return BoltRsp::kDone;
	case kAllExpertButton: // Expert
		setAllDifficulties(2);
		return BoltRsp::kDone;
	default:
		warning("unknown main menu button %d", num);
		return BoltRsp::kDone;
	}
}

bool DifficultyMenu::isReadyToPlay() const {
    for (int i = 0; i < kNumDifficultyCategories; ++i) {
        if (_game->getDifficulty(static_cast<DifficultyCategory>(i)) < 0) {
            return false;
        }
    }

    return  true;
}

void DifficultyMenu::setAllDifficulties(int difficulty) {
    for (int i = 0; i < kNumDifficultyCategories; ++i) {
        _game->setDifficulty(static_cast<DifficultyCategory>(i), difficulty);
    }
	setupButtons();
}

void DifficultyMenu::setupButtons() {
	for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < kNumDifficultyCategories; ++j) {
            _scene.getButton(kFirstDifficultyButton + 3 * j + i).setGraphics(i == _game->getDifficulty(static_cast<DifficultyCategory>(j)) ? 1 : 0);
        }
	}

	_scene.getButton(kPlayButton).setGraphics(isReadyToPlay() ? 1 : 0);

	_scene.redraw();
}

} // End of namespace Funhouse
