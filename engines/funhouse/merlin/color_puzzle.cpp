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

void ColorPuzzle::init(Graphics *graphics, IBoltEventLoop *eventLoop, Boltlib &boltlib, BltId resId) {
	_graphics = graphics;
	_eventLoop = eventLoop;
    _morphing = false;
	_morphPaletteMods = nullptr;
    _transitioning = false;

	BltResourceList resourceList;
	loadBltResourceArray(resourceList, boltlib, resId);
	BltId difficultiesId = resourceList[0].value;
	BltId sceneId        = resourceList[3].value;

	_scene.load(eventLoop, graphics, boltlib, sceneId);

	BltU16Values difficultyIds;
	loadBltResourceArray(difficultyIds, boltlib, difficultiesId);

	// TODO: Load player's chosen difficulty
	BltResourceList difficulty;
	loadBltResourceArray(difficulty, boltlib, BltShortId(difficultyIds[0].value));
	BltId numStatesId        = difficulty[0].value;
	BltId statePaletteModsId = difficulty[1].value;
    BltId moveSetId          = difficulty[11].value; // Ex: 8D2E; FIXME: there are four moveset id's?

	BltU8Values numStates;
	loadBltResourceArray(numStates, boltlib, numStatesId);

	BltResourceList statePaletteMods;
	loadBltResourceArray(statePaletteMods, boltlib, statePaletteModsId);

    BltResourceList moveSet;
    loadBltResourceArray(moveSet, boltlib, moveSetId);

	for (int i = 0; i < kNumPieces; ++i) {
		Piece &p = _pieces[i];

		p.numStates = numStates[i].value;
		loadBltResourceArray(p.palettes, boltlib, statePaletteMods[i].value);
		// FIXME: What is initial state? Is it random or chosen from a predefined set?
		p.currentState = 1;

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
		setPieceState(i, _pieces[i].currentState); // Update display
	}
}

BoltCmd ColorPuzzle::handleMsg(const BoltMsg &msg) {
    if (_transitioning) {
        if (_morphing) {
            const uint32 progress = _eventLoop->getEventTime() - _morphStartTime;
            if (progress < kMorphDuration) {
                applyPaletteModBlended(_graphics, kFore, *_morphPaletteMods,
                    _morphStartState, _morphEndState,
                    Common::Rational(progress, kMorphDuration));

                _graphics->markDirty();
                return BoltCmd::kDone;
            } else { // Done morphing
                applyPaletteMod(_graphics, kFore, *_morphPaletteMods, _morphEndState);
                _graphics->markDirty();
                _morphing = false;
                _morphPaletteMods = nullptr;
                return BoltCmd::kResend;
            }
        } else {
            if (_transitionStep < kNumTransitionSteps) {
                int pieceNum = _pieces[_selectedPiece].transition.piece[_transitionStep];
                int count = _pieces[_selectedPiece].transition.count[_transitionStep];
                ++_transitionStep;

                if (pieceNum >= 0) {
                    morphPiece(pieceNum, (_pieces[pieceNum].currentState + count) % _pieces[pieceNum].numStates);
                    return BoltCmd::kResend;
                }

                return BoltCmd::kDone;
            } else { // Done transitioning
                _transitioning = false;
                return BoltCmd::kResend;
            }
        }
    }

    // Handle input
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
	// TODO: check win condition: all pieces must be in state 0.
	return CardCmd::kWin;
}

void ColorPuzzle::enterWaitForPlayerMode() {
	// TODO: show cursor
}

void ColorPuzzle::enterTransitionMode() {
	// TODO: hide cursor
}

void ColorPuzzle::selectPiece(int piece) {
    enterTransitionMode();
    _transitioning = true;
    _selectedPiece = piece;
    _transitionStep = 0;
}

void ColorPuzzle::setPieceState(int piece, int state) {
	_pieces[piece].currentState = state;
	applyPaletteMod(_graphics, kFore, _pieces[piece].palettes, state);
	_graphics->markDirty();
}

void ColorPuzzle::morphPiece(int piece, int state) {
	debug(3, "morphing piece %d to state %d", piece, state);
	int oldState = _pieces[piece].currentState;
	_pieces[piece].currentState = state;
	startMorph(&_pieces[piece].palettes, oldState, state);
}

void ColorPuzzle::startMorph(BltPaletteMods *paletteMods, int startState, int endState) {
	_morphStartTime = _eventLoop->getEventTime();
	_morphPaletteMods = paletteMods;
	_morphStartState = startState;
	_morphEndState = endState;
    _morphing = true;
}

} // End of namespace Funhouse
