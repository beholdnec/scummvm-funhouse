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

#include "funhouse/merlin/color_puzzle.h"

namespace Funhouse {

void ColorPuzzle::init(MerlinGame *game, Boltlib &boltlib, BltId resId) {
    _game = game;
	_graphics = _game->getGraphics();
	_eventLoop = _game->getEventLoop();
	_morphPaletteMods = nullptr;
    _transitionActive = false;
    _morphActive = false;

    _popup.init(_game, boltlib, _game->getPopupResId(MerlinGame::kPuzzlePopup));

	BltResourceList resourceList;
	loadBltResourceArray(resourceList, boltlib, resId);
	BltId difficultiesId = resourceList[0].value;
	BltId sceneId        = resourceList[3].value;

	_scene.load(_eventLoop, _graphics, boltlib, sceneId);

	BltU16Values difficultyIds;
	loadBltResourceArray(difficultyIds, boltlib, difficultiesId);

    int puzzleNum = 0; // TODO: Choose a random puzzle 0..3.

	// TODO: Load player's chosen difficulty
	BltResourceList difficulty;
	loadBltResourceArray(difficulty, boltlib, BltShortId(difficultyIds[0].value));
	BltId numStatesId        = difficulty[0].value;
	BltId statePaletteModsId = difficulty[1].value;
    BltId solutionId         = difficulty[3 + puzzleNum].value; // Ex: 8D1E
    BltId initialId          = difficulty[7 + puzzleNum].value; // Ex: 8D22
    BltId moveSetId          = difficulty[11 + puzzleNum].value; // Ex: 8D2E

	BltU8Values numStates;
	loadBltResourceArray(numStates, boltlib, numStatesId);

	BltResourceList statePaletteMods;
	loadBltResourceArray(statePaletteMods, boltlib, statePaletteModsId);

    BltResourceList moveSet;
    loadBltResourceArray(moveSet, boltlib, moveSetId);

    BltU8Values solution;
    loadBltResourceArray(solution, boltlib, solutionId);

    BltU8Values initial;
    loadBltResourceArray(initial, boltlib, initialId);

	for (int i = 0; i < kNumPieces; ++i) {
		Piece &p = _pieces[i];

		p.numStates = numStates[i].value;
		loadBltResourceArray(p.palettes, boltlib, statePaletteMods[i].value);
		p.state = initial[i].value;
        p.solution = solution[i].value;

        BltResourceList moveArray;
        loadBltResourceArray(moveArray, boltlib, moveSet[i].value);
        // For some reason, all entries in moveArray point to the same resource ID. Why this is the case is unknown.
        BltId transitionId = moveArray[0].value;
        loadBltResource(p.transition, boltlib, transitionId);
	}
}

void ColorPuzzle::enter() {
	_scene.enter();
	_morphPaletteMods = nullptr;
	for (int i = 0; i < kNumPieces; ++i) {
		setPieceState(i, _pieces[i].state); // Update display
	}
}

BoltCmd ColorPuzzle::handleMsg(const BoltMsg &msg) {
    if (_morphActive) {
        return driveMorph();
    } else if (_transitionActive) {
        return driveTransition();
    }

    BoltCmd cmd = _popup.handleMsg(msg);
    if (cmd.type != BoltCmd::kPass) {
        return cmd;
    }

    if (msg.type == Scene::kClickButton) {
        return handleButtonClick(msg.num);
    }

    return _scene.handleMsg(msg);
}

BoltCmd ColorPuzzle::handleButtonClick(int num) {
	debug(3, "Clicked button %d", num);

	if (num >= 0 && num < kNumPieces) {
		selectPiece(num);

		_eventLoop->setMsg(BoltMsg::kDrive);
		return BoltCmd::kResend;
	}

	// TODO: clicking outside of pieces should show the solution
	return CardCmd::kWin;
}

BoltCmd ColorPuzzle::driveTransition() {
    assert(_transitionActive);

    if (_transitionStep < kNumTransitionSteps) {
        int pieceNum = _pieces[_selectedPiece].transition.piece[_transitionStep];
        int count = _pieces[_selectedPiece].transition.count[_transitionStep];
        ++_transitionStep;

        if (pieceNum >= 0) {
            // FIXME: This isn't how it should work...
            morphPiece(pieceNum, (_pieces[pieceNum].state + count) % _pieces[pieceNum].numStates);
            return BoltCmd::kResend;
        }

        return BoltCmd::kDone;
    }

    if (isSolved()) {
        return kWin;
    }

    _transitionActive = false;
    return BoltCmd::kResend;
}

BoltCmd ColorPuzzle::driveMorph() {
    assert(_morphActive);

    const uint32 delta = _eventLoop->getEventTime() - _morphStartTime;
    if (delta < kMorphDuration) {
        applyPaletteModBlended(_graphics, kFore, *_morphPaletteMods,
            _morphStartState, _morphEndState,
            Common::Rational(delta, kMorphDuration));

        _graphics->markDirty();
        return BoltCmd::kDone;
    }

    applyPaletteMod(_graphics, kFore, *_morphPaletteMods, _morphEndState);
    _graphics->markDirty();
    _morphPaletteMods = nullptr;
    _morphActive = false;
    return BoltCmd::kResend;
}

void ColorPuzzle::selectPiece(int piece) {
    _selectedPiece = piece;
    _transitionStep = 0;
    _transitionActive = true;
}

void ColorPuzzle::setPieceState(int piece, int state) {
	_pieces[piece].state = state;
	applyPaletteMod(_graphics, kFore, _pieces[piece].palettes, state);
	_graphics->markDirty();
}

void ColorPuzzle::morphPiece(int piece, int state) {
	debug(3, "morphing piece %d to state %d", piece, state);
	int oldState = _pieces[piece].state;
	_pieces[piece].state = state;
	startMorph(&_pieces[piece].palettes, oldState, state);
}

void ColorPuzzle::startMorph(BltPaletteMods *paletteMods, int startState, int endState) {
	_morphStartTime = _eventLoop->getEventTime();
	_morphPaletteMods = paletteMods;
	_morphStartState = startState;
	_morphEndState = endState;
    _morphActive = true;
}

bool ColorPuzzle::isSolved() const {
    bool solved = true;

    for (int i = 0; i < kNumPieces; ++i) {
        if (_pieces[i].state != _pieces[i].solution) {
            solved = false;
            break;
        }
    }

    return solved;
}

} // End of namespace Funhouse
