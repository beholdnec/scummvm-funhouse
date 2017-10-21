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

#include "bolt/merlin/color_puzzle.h"

namespace Bolt {

void ColorPuzzle::init(Graphics *graphics, IBoltEventLoop *eventLoop, Boltlib &boltlib, BltId resId) {
	_graphics = graphics;
	_eventLoop = eventLoop;

	_morphPaletteMods = nullptr;

	BltResourceList resourceList;
	loadBltResourceArray(resourceList, boltlib, resId);
	BltId difficultiesId = resourceList[0].value;
	BltId sceneId = resourceList[3].value;

	_scene.load(eventLoop, graphics, boltlib, sceneId);

	BltU16Values difficultyIds;
	loadBltResourceArray(difficultyIds, boltlib, difficultiesId);
	// TODO: Load player's chosen difficulty
	BltResourceList difficulty;
	loadBltResourceArray(difficulty, boltlib, BltShortId(difficultyIds[0].value));
	BltId numStatesId = difficulty[0].value;
	BltId statePaletteModsId = difficulty[1].value;

	BltU8Values numStates;
	loadBltResourceArray(numStates, boltlib, numStatesId);
	BltResourceList statePaletteMods;
	loadBltResourceArray(statePaletteMods, boltlib, statePaletteModsId);

	for (int i = 0; i < kNumPieces; ++i) {
		Piece &p = _pieces[i];
		p.numStates = numStates[i].value;
		loadBltResourceArray(p.palettes, boltlib, statePaletteMods[i].value);
		// FIXME: What is initial state? Is it random or chosen from a predefined set?
		p.currentState = 1;
	}
}

void ColorPuzzle::enter() {
	_mode = kWaitForPlayer;
	_scene.enter();
	_morphPaletteMods = nullptr;

	for (int i = 0; i < kNumPieces; ++i) {
		setPieceState(i, _pieces[i].currentState); // Update display
	}
}

BoltCmd ColorPuzzle::handleMsg(const BoltMsg &msg) {
	switch (_mode) {
	case kWaitForPlayer:
		return driveWaitForPlayer(msg);
	case kTransition:
		return driveTransition(msg);
	default:
		assert(false && "Invalid color puzzle mode");
		return BoltCmd::kDone;
	}
}

BoltCmd ColorPuzzle::driveWaitForPlayer(const BoltMsg &msg) {
	if (msg.type == Scene::kClickButton) {
		return handleButtonClick(msg.num);
	}

	return _scene.handleMsg(msg);
}

BoltCmd ColorPuzzle::driveTransition(const BoltMsg &msg) {
	// TODO: eliminate kDrive events. Transition should be driven primarily by SmoothAnimation and
	// AudioEnded events, once those event types are implemented.
	if (msg.type != BoltMsg::kDrive) {
		return BoltCmd::kDone;
	}

	const uint32 progress = _eventLoop->getEventTime() - _morphStartTime;
	if (progress >= kMorphDuration) {
		applyPaletteMod(_graphics, kFore, *_morphPaletteMods, _morphEndState);
		_graphics->markDirty();
		_morphPaletteMods = nullptr;
		enterWaitForPlayerMode();
		return BoltCmd::kResend;
	}

	applyPaletteModBlended(_graphics, kFore, *_morphPaletteMods,
		_morphStartState, _morphEndState,
		Common::Rational(progress, kMorphDuration));
	_graphics->markDirty();
	return BoltCmd::kDone;
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
	_mode = kWaitForPlayer;
	// TODO: show cursor
}

void ColorPuzzle::enterTransitionMode() {
	_mode = kTransition;
	// TODO: hide cursor
}

void ColorPuzzle::selectPiece(int piece) {
	// TODO: change states according to puzzle definition
	morphPiece(piece, (_pieces[piece].currentState + 1) % _pieces[piece].numStates);
	enterTransitionMode();
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
}

} // End of namespace Bolt
