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
#include "funhouse/boltlib/sound.h"

namespace Funhouse {

void ColorPuzzle::init(MerlinGame *game, Boltlib &boltlib, int challengeIdx) {
	_game = game;
	_mode.init(_game->getEngine());
	_morphPaletteMods = nullptr;

	uint16 resId = 0;
	switch (challengeIdx) {
	case 18: resId = 0x8C13; break;
	case 24: resId = 0x9014; break;
	default: assert(false); break;
	}

	_game->setPopup(MerlinGame::kPuzzlePopup);

	BltResourceList resourceList;
	loadBltResourceArray(resourceList, boltlib, BltShortId(resId));
	BltId difficultiesId = resourceList[0].value;
	BltId sceneId        = resourceList[3].value;

	loadScene(_scene, _game->getEngine(), boltlib, sceneId);

	BltU16Values difficultyIds;
	loadBltResourceArray(difficultyIds, boltlib, difficultiesId);

	int puzzleNum = 0; // TODO: Choose a random puzzle 0..3.

	BltResourceList difficulty;
	loadBltResourceArray(difficulty, boltlib, BltShortId(difficultyIds[_game->getDifficulty(kLogicDifficulty)].value));
	BltId numStatesId        = difficulty[0].value; // Ex: 8D00
	BltId statePaletteModsId = difficulty[1].value; // Ex: 8D1D
	BltId soundsId           = difficulty[2].value; // Ex: 8D52
	BltId solutionId         = difficulty[3 + puzzleNum].value; // Ex: 8D1E
	BltId initialId          = difficulty[7 + puzzleNum].value; // Ex: 8D22
	BltId moveSetId          = difficulty[11 + puzzleNum].value; // Ex: 8D2E

	BltU8Values numStates;
	loadBltResourceArray(numStates, boltlib, numStatesId);

	BltResourceList statePaletteMods;
	loadBltResourceArray(statePaletteMods, boltlib, statePaletteModsId);

	BltResourceList soundLists;
	loadBltResourceArray(soundLists, boltlib, soundsId);
	_soundLists.alloc(soundLists.size());
	for (int i = 0; i < soundLists.size(); ++i) {
		_soundLists[i].load(boltlib, soundLists[i].value);
	}

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

	idleMode();
}

BoltRsp ColorPuzzle::handleMsg(const BoltMsg &msg) {
	_mode.react(msg);
	return kDone;
}

BoltRsp ColorPuzzle::handlePopupButtonClick(int num) {
	switch (num) {
	case 0: // Return
		_game->branchReturn();
		return BoltRsp::kDone;
	default:
		warning("Unhandled popup button %d", num);
		return BoltRsp::kDone;
	}
}

BoltRsp ColorPuzzle::handleButtonClick(int num) {
	debug(3, "Clicked button %d", num);

	if (num >= 0 && num < kNumPieces) {
		selectPiece(num);

		_game->getEngine()->setNextMsg(BoltMsg::kDrive);
		return BoltRsp::kDone;
	}

	// TODO: clicking outside of pieces should show the solution
	_game->branchWin();
	return BoltRsp::kDone;
}

void ColorPuzzle::idleMode() {
	_mode.transition();
	_mode.onMsg([this](const BoltMsg& msg) {
		BoltRsp cmd = _game->handlePopup(msg);
		if (cmd != BoltRsp::kPass) {
			return cmd;
		}

		switch (msg.type) {
		case BoltMsg::kPopupButtonClick:
			return handlePopupButtonClick(msg.num);
		case Scene::kClickButton:
			return handleButtonClick(msg.num);
		default:
			return _scene.handleMsg(msg);
		}
	});
}

BoltRsp ColorPuzzle::driveTransition() {
	if (_transitionStep < kNumTransitionSteps) {
		int pieceNum = _pieces[_selectedPiece].transition.piece[_transitionStep];
		int count = _pieces[_selectedPiece].transition.count[_transitionStep];
		++_transitionStep;

		if (pieceNum >= 0) {
			// FIXME: This isn't how it should work...
			morphPiece(pieceNum, (_pieces[pieceNum].state + count) % _pieces[pieceNum].numStates);
			_game->getEngine()->setNextMsg(BoltMsg::kDrive);
			return BoltRsp::kDone;
		}

		return BoltRsp::kDone;
	}

	if (isSolved()) {
		_game->branchWin();
		return BoltRsp::kDone;
	}

	idleMode();
	_game->getEngine()->setNextMsg(BoltMsg::kDrive);
	return BoltRsp::kDone;
}

BoltRsp ColorPuzzle::driveMorph() {
	if (_morphTimer.ticks < kMorphDuration) {
		applyPaletteModBlended(_game->getGraphics(), kFore, *_morphPaletteMods,
			_morphStartState, _morphEndState,
			Common::Rational(_morphTimer.ticks, kMorphDuration));

		_game->getGraphics()->markDirty();
		return BoltRsp::kDone;
	}

	applyPaletteMod(_game->getGraphics(), kFore, *_morphPaletteMods, _morphEndState);
	_game->getGraphics()->markDirty();
	_morphPaletteMods = nullptr;
	idleMode();
	driveTransition();
	return BoltRsp::kDone;
}

void ColorPuzzle::selectPiece(int piece) {
	_selectedPiece = piece;
	_transitionStep = 0;
	driveTransition();
}

void ColorPuzzle::setPieceState(int piece, int state) {
	_pieces[piece].state = state;
	applyPaletteMod(_game->getGraphics(), kFore, _pieces[piece].palettes, state);
	_game->getGraphics()->markDirty();
}

void ColorPuzzle::morphPiece(int piece, int state) {
	debug(3, "morphing piece %d to state %d", piece, state);
	int oldState = _pieces[piece].state;
	_pieces[piece].state = state;
	startMorph(&_pieces[piece].palettes, oldState, state);
	_soundLists[piece].play(_game->getEngine()->_mixer);
}

void ColorPuzzle::startMorph(BltPaletteMods *paletteMods, int startState, int endState) {
	_morphPaletteMods = paletteMods;
	_morphStartState = startState;
	_morphEndState = endState;

	_mode.transition();
	_mode.onEnter([this]() {
		_morphTimer.start(0, false);
		_game->getEngine()->requestSmoothAnimation();
	});
	_mode.onMsg([this](const BoltMsg& msg) {
		switch (msg.type) {
		case BoltMsg::kSmoothAnimation:
			driveMorph();
			_game->getEngine()->requestSmoothAnimation();
			break;
		}
	});
	_mode.onTimer(&_morphTimer, nullptr);
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
