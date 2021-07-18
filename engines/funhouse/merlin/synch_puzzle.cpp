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
        numItems = src.getUint8At(0);
        // TODO: More fields
    }

    uint8 numItems;
};

void SynchPuzzle::init(MerlinGame *game, Boltlib &boltlib, int challengeIdx) {
    _game = game;
    _mode.init(_game->getEngine());

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
    BltId difficultyId = BltShortId(difficultiesList[_game->getDifficulty(kLogicDifficulty)].value); // Ex: 7A72

    BltSynchPuzzleInfo info;
    loadBltResource(info, boltlib, infoId);

    _moveAgenda.alloc(info.numItems - 1);

    // Each difficulty has 4 possible puzzles. Choose one.
    int puzzleNum = 0; // TODO: choose a random puzzle number 0-3

    BltResourceList difficulty;
    loadBltResourceArray(difficulty, boltlib, difficultyId);
    BltId stateCountsId = difficulty[0].value; // Ex: 7A00
    BltId itemListId    = difficulty[1].value; // Ex: 7A3D
    BltId solutionId    = difficulty[3 + puzzleNum].value; // Ex: 7A3E
    BltId initialId     = difficulty[7 + puzzleNum].value; // Ex: 7A42
    BltId movesetsId    = difficulty[11 + puzzleNum].value; // Ex: 7A4E
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

    BltU8Values initial;
    loadBltResourceArray(initial, boltlib, initialId);

    BltU8Values solution;
    loadBltResourceArray(solution, boltlib, solutionId);

    _items.alloc(itemList.size());
    for (uint i = 0; i < _items.size(); ++i) {
        _items[i].state = initial[i].value;
        _items[i].solution = solution[i].value;
        _items[i].sprites.load(boltlib, itemList[i].value);

        BltResourceList moveset;
        loadBltResourceArray(moveset, boltlib, movesets[i].value);

        _items[i].moveset.alloc(stateCounts[i].value);
        for (uint j = 0; j < stateCounts[i].value; ++j) {
            loadBltResourceArray(_items[i].moveset[j], boltlib, moveset[j].value);
        }
    }

	loadScene(_scene, _game->getEngine(), boltlib, sceneId);
}

void SynchPuzzle::enter() {
    redraw();
    idle();
}

BoltRsp SynchPuzzle::handleMsg(const BoltMsg &msg) {
    _mode.react(msg);
    return kDone;
}

BoltRsp SynchPuzzle::handlePopupButtonClick(int num) {
	switch (num) {
	case 0: // Return
        _game->branchReturn();
		return BoltRsp::kDone;
	default:
		warning("Unhandled popup button %d", num);
		return BoltRsp::kDone;
	}
}

BoltRsp SynchPuzzle::handleButtonClick(int num) {
	debug(3, "Clicked button %d", num);
    return BoltRsp::kDone;
}

void SynchPuzzle::redraw() {
    _scene.enter();

    for (uint i = 0; i < _items.size(); ++i) {
        const Item& item = _items[i];
        const Common::Point& spritePos = item.sprites.getSpritePosition(item.state);
        const BltImage* spriteImage = item.sprites.getSpriteImage(item.state);
        const Common::Point& origin = _scene.getOrigin();
        spriteImage->drawAt(_game->getGraphics()->getPlaneSurface(kFore), spritePos.x - origin.x, spritePos.y - origin.y, true);
    }
}

void SynchPuzzle::idle() {
    _mode.transition();
    _mode.onEnter([]() {
    });
    _mode.onMsg([this](const BoltMsg& msg) {
        BoltRsp cmd = _game->handlePopup(msg);
        if (cmd != BoltRsp::kPass) {
            return cmd;
        }

        if (msg.type == BoltMsg::kPopupButtonClick) {
            return handlePopupButtonClick(msg.num);
        }

        if (msg.type == Scene::kClickButton) {
            return handleButtonClick(msg.num);
        }

        if (msg.type == BoltMsg::kClick) {
            int itemNum = getItemAtPosition(msg.point);
            if (itemNum != -1) {
                const Item& item = _items[itemNum];
                const BltSynchPuzzleTransition& transition = item.moveset[item.state];
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
}

void SynchPuzzle::setTimeout(int32 delay, std::function<void()> then) {
    _mode.transition();
    _mode.onEnter([this, delay]() {
        _timer.start(delay, true);
    });
    _mode.onTimer(&_timer, [this]() {
        _timeoutThen();
    });
    _timeoutThen = then;
}

BoltRsp SynchPuzzle::driveTransition() {
    for (int i = 0; i < _moveAgenda.size(); ++i) {
        if (_moveAgenda[i].item != -1 && _moveAgenda[i].count != 0) {
            Item &item = _items[_moveAgenda[i].item];

            if (_moveAgenda[i].count > 0) {
                --_moveAgenda[i].count;

                ++item.state;
                if (item.state >= item.sprites.getSpriteCount()) {
                    item.state = 0;
                }
            } else {
                ++_moveAgenda[i].count;

                --item.state;
                if (item.state < 0) {
                    item.state = item.sprites.getSpriteCount() - 1;
                }
            }

            enter(); // Redraw the scene

            setTimeout(kTimeoutDelay, [this]() {
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
		const Common::Point spritePos = item.sprites.getSpritePosition(item.state);
		const BltImage* spriteImage = item.sprites.getSpriteImage(item.state);
        const Common::Point &origin = _scene.getOrigin();
        if (spriteImage->query(pt.x - (spritePos.x - origin.x), pt.y - (spritePos.y - origin.y)) != 0) {
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
        if (_items[i].state != _items[i].solution) {
            solved = false;
            break;
        }
    }

    return solved;
}

} // End of namespace Funhouse
