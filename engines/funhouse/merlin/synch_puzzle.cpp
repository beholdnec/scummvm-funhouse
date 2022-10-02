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

#include "funhouse/merlin/synch_puzzle.h"

namespace Funhouse {

struct BltSynchPuzzleInfo { // type 52
	static const uint32 kType = kBltSynchPuzzleInfo;
	static const uint kSize = 0x12;
	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		numItems = src.getUint8At(0x0);
		variationSlot = src.getUint8At(0x1);
		// TODO: More fields
	}

	uint8 numItems;
	uint8 variationSlot;
};

void SynchPuzzle::init(MerlinGame *game, Boltlib &boltlib, int challengeIdx) {
	_game = game;
	_modeCtx.init(_game->getEngine());

	uint16 resId = 0;
	switch (challengeIdx) {
	case 12: resId = 0x7D12; break;
	case 19: resId = 0x8114; break;
	case 25: resId = 0x8512; break;
	default: assert(false); break;
	}

	_game->setPopup(MerlinGame::kPuzzlePopup);

	BltResourceList resourceList;
	loadBltResourceArray(resourceList, boltlib, BltShortId(resId));
	BltId difficultiesId = resourceList[0].value; // Ex: 7D00
	BltId infoId         = resourceList[1].value; // Ex: 7D01
	BltId sceneId        = resourceList[4].value; // Ex: 7D0B

	BltU16Values difficultiesList;
	loadBltResourceArray(difficultiesList, boltlib, difficultiesId);

	BltSynchPuzzleInfo info;
	loadBltResource(info, boltlib, infoId);

	int difficultyLevel = _game->getDifficulty(kLogicDifficulty);
	int variation = (_game->getVariationSlot(info.variationSlot) + 1) % 4;
	debug(3, "Loading synch puzzle difficulty %d, variation %d", difficultyLevel, variation);

	BltId difficultyId = BltShortId(difficultiesList[difficultyLevel].value); // Ex: 7A72

	_moveAgenda.resize(info.numItems - 1);

	BltResourceList difficulty;
	loadBltResourceArray(difficulty, boltlib, difficultyId);
	BltId stateCountsId = difficulty[0].value; // Ex: 7A00
	BltId itemListId    = difficulty[1].value; // Ex: 7A3D
	BltId solutionId    = difficulty[3 + variation].value;  // Ex: 7A3E
	BltId initialId     = difficulty[7 + variation].value;  // Ex: 7A42
	BltId movesetsId    = difficulty[11 + variation].value; // Ex: 7A4E
	// 0: State counts for each item
	// 1: Sprites
	// 2: Sounds
	// 3-6: Solution states 0-3
	// 7-10: Initial states 0-3
	// 11-14: Movesets 0-3

	BltU8Values stateCounts;
	loadBltResourceArray(stateCounts, boltlib, stateCountsId);

	BltResourceList itemList;
	loadBltResourceArray(itemList, boltlib, itemListId);

	BltResourceList movesets;
	loadBltResourceArray(movesets, boltlib, movesetsId);

	loadBltResourceArray(_initial, boltlib, initialId);

	BltU8Values solution;
	loadBltResourceArray(solution, boltlib, solutionId);

	_items.resize(itemList.size());
	for (uint i = 0; i < _items.size(); ++i) {
		_items[i].solution = solution[i].value;
		_items[i].sprites = loadBltSprites(boltlib, itemList[i].value);

		BltResourceList moveset;
		loadBltResourceArray(moveset, boltlib, movesets[i].value);

		_items[i].moveset.resize(stateCounts[i].value);
		for (uint j = 0; j < stateCounts[i].value; ++j) {
			loadBltResourceArray(_items[i].moveset[j], boltlib, moveset[j].value);
		}
	}

	loadScene(_scene, _game->getEngine(), boltlib, sceneId);
	// XXX: The door puzzle mistakenly has an image attached to the foreground in the PC version.
	// In the CD-i version, the image is null.
	_scene.setPlaneImageEnable(kFore, false);

	_state = _game->getChallengeState(challengeIdx).cast<State>();
	if (!_state || _state->difficulty != difficultyLevel || _state->variation != variation) {
		_state.reset(new State());
		_game->setChallengeState(challengeIdx, _state);

		_state->difficulty = difficultyLevel;
		_state->variation = variation;
		_state->items.resize(info.numItems);
		for (uint i = 0; i < info.numItems; ++i) {
			_state->items[i] = _initial[i].value;
		}
	}
}

void SynchPuzzle::enter() {
	_scene.enter();
	redraw();
	idle();
}

BoltRsp SynchPuzzle::handleMsg(const BoltMsg &msg) {
	_modeCtx.react(msg);
	return kDone;
}

void SynchPuzzle::handleReset() {
	for (uint i = 0; i < _items.size(); ++i) {
		_state->items[i] = _initial[i].value;
	}
	redraw();
}

BoltRsp SynchPuzzle::handleButtonClick(int num) {
	debug(3, "Clicked button %d", num);
	return BoltRsp::kDone;
}

void SynchPuzzle::redraw() {
	_scene.redraw();

	for (uint i = 0; i < _items.size(); ++i) {
		const Item& item = _items[i];
		SharedSprite sprite = (*item.sprites)[_state->items[i]];
		Common::Point pos = sprite->pos - _scene.getOrigin();
		sprite->image->drawAt(_game->getGraphics()->getPlaneSurface(kFore), pos.x, pos.y, true);
	}
}

void SynchPuzzle::idle() {
	_idleMode = {};
	_idleMode.onEnter([]() {
	});
	_idleMode.onMsg([this](const BoltMsg &msg) {
		BoltRsp cmd = _game->handlePopup(msg);
		if (cmd != BoltRsp::kPass) {
			return cmd;
		}

		if (msg.type == Scene::kClickButton) {
			return handleButtonClick(msg.num);
		}

		if (msg.type == BoltMsg::kClick) {
			int itemNum = getItemAtPosition(msg.point);
			if (itemNum != -1) {
				const Item& item = _items[itemNum];
				const BltSynchPuzzleTransition& transition = item.moveset[_state->items[itemNum]];
				for (int i = 0; i < transition.size(); ++i) {
					_moveAgenda[i].item = transition[i].item;
					_moveAgenda[i].count = transition[i].count;
				}

				// TODO: hide cursor during transition
				driveTransition();
				_game->getEngine()->setNextMsg(BoltMsg::kDrive);
				return BoltRsp::kDone;
			}
		}

		// TODO: when clicking outside the pieces, a preview of the solution should be shown.

		return _scene.handleMsg(msg);
	});

	_modeCtx.setNextMode(&_idleMode);
}

BoltRsp SynchPuzzle::driveTransition() {
	for (int i = 0; i < _moveAgenda.size(); ++i) {
		if (_moveAgenda[i].item != -1 && _moveAgenda[i].count != 0) {
			int itemIdx = _moveAgenda[i].item;
			Item &item = _items[itemIdx];

			if (_moveAgenda[i].count > 0) {
				--_moveAgenda[i].count;

				++_state->items[itemIdx];
				if (_state->items[itemIdx] >= item.sprites->size()) {
					_state->items[itemIdx] = 0;
				}
			} else {
				++_moveAgenda[i].count;

				--_state->items[itemIdx];
				if (_state->items[itemIdx] < 0) {
					_state->items[itemIdx] = item.sprites->size() - 1;
				}
			}

			enter(); // Redraw the scene

			_game->setTimeout(&_modeCtx, kTimeoutDelay, [this]() {
				driveTransition();
			});
			_game->getEngine()->setNextMsg(BoltMsg::kDrive);
			return BoltRsp::kDone;
		}
	}

	// Agenda is empty; check win condition and return to idle state
	if (isSolved()) {
		_game->branchWin();
		return BoltRsp::kDone;
	}

	idle();
	_game->getEngine()->setNextMsg(BoltMsg::kDrive);
	return BoltRsp::kDone;
}

int SynchPuzzle::getItemAtPosition(const Common::Point &pt) {
	int result = -1;

	for (int i = 0; i < _items.size(); ++i) {
		const Item &item = _items[i];
		SharedSprite sprite = (*item.sprites)[_state->items[i]];
		Common::Point pos = sprite->pos - _scene.getOrigin();
		if (sprite->image->query(pt.x - pos.x, pt.y - pos.y) != 0) {
			result = i;
			// Don't break early. All items must be queried, since later items
			// may overlap earlier items.
		}
	}

	return result;
}

bool SynchPuzzle::isSolved() const {
	bool solved = true;

	for (int i = 0; i < _items.size(); ++i) {
		if (_state->items[i] != _items[i].solution) {
			solved = false;
			break;
		}
	}

	return solved;
}

} // End of namespace Funhouse
