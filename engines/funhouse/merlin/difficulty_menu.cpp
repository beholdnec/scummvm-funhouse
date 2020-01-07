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

BoltCmd DifficultyMenu::handleMsg(const BoltMsg &msg) {
	if (msg.type == Scene::kClickButton) {
		return handleButtonClick(msg.num);
	}

	return _scene.handleMsg(msg);
}

static const int kPlayButton = 3;
static const int kAllBeginnerButton = 4;
static const int kAllAdvancedButton = 5;
static const int kAllExpertButton = 6;
static const int kFirstWordsDifficultyButton = 12;
static const int kFirstShapesDifficultyButton = 15;
static const int kFirstActionDifficultyButton = 18;
static const int kFirstMemoryDifficultyButton = 21;
static const int kFirstLogicDifficultyButton = 24;

BoltCmd DifficultyMenu::handleButtonClick(int num) {
	// Words
	if (num >= kFirstWordsDifficultyButton && num < kFirstWordsDifficultyButton + 3) {
		_game->setWordsDifficulty(num - kFirstWordsDifficultyButton);
		setupButtons();
		return BoltCmd::kDone;
	}

	// Shapes
	if (num >= kFirstShapesDifficultyButton && num < kFirstShapesDifficultyButton + 3) {
		_game->setShapesDifficulty(num - kFirstShapesDifficultyButton);
		setupButtons();
		return BoltCmd::kDone;
	}

	// Action
	if (num >= kFirstActionDifficultyButton && num < kFirstActionDifficultyButton + 3) {
		_game->setActionDifficulty(num - kFirstActionDifficultyButton);
		setupButtons();
		return BoltCmd::kDone;
	}

	// Memory
	if (num >= kFirstMemoryDifficultyButton && num < kFirstMemoryDifficultyButton + 3) {
		_game->setMemoryDifficulty(num - kFirstMemoryDifficultyButton);
		setupButtons();
		return BoltCmd::kDone;
	}

	// Logic
	if (num >= kFirstLogicDifficultyButton && num < kFirstLogicDifficultyButton + 3) {
		_game->setLogicDifficulty(num - kFirstLogicDifficultyButton);
		setupButtons();
		return BoltCmd::kDone;
	}

	switch (num) {
	case -1: // No button
		return BoltCmd::kDone;
	case kPlayButton: // Play
		if (isReadyToPlay()) {
			return Card::kEnd;
		}
		return BoltCmd::kDone;
	case kAllBeginnerButton: // Beginner
		setAllDifficulties(0);
		return BoltCmd::kDone;
	case kAllAdvancedButton: // Advanced
		setAllDifficulties(1);
		return BoltCmd::kDone;
	case kAllExpertButton: // Expert
		setAllDifficulties(2);
		return BoltCmd::kDone;
	default:
		warning("unknown main menu button %d", num);
		return BoltCmd::kDone;
	}
}

bool DifficultyMenu::isReadyToPlay() const {
	return _game->getWordsDifficulty() >= 0 &&
		_game->getShapesDifficulty() >= 0 &&
		_game->getActionDifficulty() >= 0 &&
		_game->getMemoryDifficulty() >= 0 &&
		_game->getLogicDifficulty() >= 0;
}

void DifficultyMenu::setAllDifficulties(int difficulty) {
	_game->setWordsDifficulty(difficulty);
	_game->setShapesDifficulty(difficulty);
	_game->setActionDifficulty(difficulty);
	_game->setMemoryDifficulty(difficulty);
	_game->setLogicDifficulty(difficulty);
	setupButtons();
}

void DifficultyMenu::setupButtons() {
	for (int i = 0; i < 3; ++i) {
		_scene.getButton(kFirstWordsDifficultyButton + i).setGraphics(i == _game->getWordsDifficulty() ? 1 : 0);
		_scene.getButton(kFirstShapesDifficultyButton + i).setGraphics(i == _game->getShapesDifficulty() ? 1 : 0);
		_scene.getButton(kFirstActionDifficultyButton + i).setGraphics(i == _game->getActionDifficulty() ? 1 : 0);
		_scene.getButton(kFirstMemoryDifficultyButton + i).setGraphics(i == _game->getMemoryDifficulty() ? 1 : 0);
		_scene.getButton(kFirstLogicDifficultyButton + i).setGraphics(i == _game->getLogicDifficulty() ? 1 : 0);
	}

	_scene.getButton(kPlayButton).setGraphics(isReadyToPlay() ? 1 : 0);

	_scene.redraw();
}

} // End of namespace Funhouse
