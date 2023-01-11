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

struct BltColorPuzzleInfo { // type 57
	static const uint32 kType = kBltColorPuzzleInfo;
	static const uint kSize = 0xa;
	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		pieceCount = src.getUint8At(0x0);
		variationSlot = src.getUint8At(0x1);
		// TODO: More fields
	}

	uint8 pieceCount;
	uint8 variationSlot;
};

void ColorPuzzle::init(MerlinGame *game, Boltlib &boltlib, int challengeIdx) {
	_game = game;
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
	BltId infoId         = resourceList[1].value; // Ex: 9001
	BltId sceneId        = resourceList[3].value;

	BltColorPuzzleInfo info;
	loadBltResource(info, boltlib, infoId);

	int difficultyLevel = _game->getDifficulty(kLogicDifficulty);
	int variation = (_game->getVariationSlot(info.variationSlot) + 1) % 4;
	debug(3, "Loading color puzzle difficulty %d, variation %d", difficultyLevel, variation);

	loadScene(_scene, _game->getEngine(), boltlib, sceneId);

	BltU16Values difficultyIds;
	loadBltResourceArray(difficultyIds, boltlib, difficultiesId);

	BltResourceList difficulty;
	loadBltResourceArray(difficulty, boltlib, BltShortId(difficultyIds[difficultyLevel].value));
	BltId numStatesId        = difficulty[0].value; // Ex: 8D00
	BltId statePaletteModsId = difficulty[1].value; // Ex: 8D1D
	BltId soundsId           = difficulty[2].value; // Ex: 8D52
	BltId solutionId         = difficulty[3 + variation].value; // Ex: 8D1E
	BltId initialId          = difficulty[7 + variation].value; // Ex: 8D22
	BltId moveSetId          = difficulty[11 + variation].value; // Ex: 8D2E

	BltU8Values numStates;
	loadBltResourceArray(numStates, boltlib, numStatesId);

	BltResourceList statePaletteMods;
	loadBltResourceArray(statePaletteMods, boltlib, statePaletteModsId);

	BltResourceList soundLists;
	loadBltResourceArray(soundLists, boltlib, soundsId);
	_soundLists.resize(soundLists.size());
	for (int i = 0; i < soundLists.size(); ++i) {
		_soundLists[i].load(boltlib, soundLists[i].value);
	}

	BltResourceList moveSet;
	loadBltResourceArray(moveSet, boltlib, moveSetId);

	BltU8Values solution;
	loadBltResourceArray(solution, boltlib, solutionId);

	loadBltResourceArray(_initial, boltlib, initialId);

	for (int i = 0; i < kNumPieces; ++i) {
		Piece &p = _pieces[i];

		p.numStates = numStates[i].value;
		loadBltResourceArray(p.palettes, boltlib, statePaletteMods[i].value);
		p.solution = solution[i].value;

		BltResourceList moveArray;
		loadBltResourceArray(moveArray, boltlib, moveSet[i].value);
		// For some reason, all entries in moveArray point to the same resource ID. Why this is the case is unknown.
		BltId transitionId = moveArray[0].value;
		loadBltResource(p.transition, boltlib, transitionId);
	}

	_state = _game->getChallengeState(challengeIdx).cast<State>();
	if (!_state || _state->difficulty != difficultyLevel || _state->variation != variation) {
		_state.reset(new State());
		_game->setChallengeState(challengeIdx, _state);

		_state->difficulty = difficultyLevel;
		_state->variation = variation;
		reset();
	}
}

void ColorPuzzle::enter() {
	_scene.enter();
	_morphPaletteMods = nullptr;
	draw();
	evaluate();
}

BoltRsp ColorPuzzle::handleMsg(const BoltMsg &msg) {
	_task.run(msg);
	return kDone;
}

void ColorPuzzle::handleReset() {
	// TODO: play reset sound
	reset();
	draw();
	evaluate();
}

void ColorPuzzle::handleUndo() {
	if (_undone) {
		// Redo the last move
		startMove(_redoPiece, _redoCurrState);
		_undone = false;
	} else {
		// TODO: play undo sound
		_state->state = _state->prevState;
		_undone = true;
		draw();
	}
}

BoltRsp ColorPuzzle::handleButtonClick(int num) {
	debug(3, "Clicked button %d", num);

	if (num >= 0 && num < kNumPieces) {
		startMove(num, _state->state[num]);

		_game->getEngine()->setNextMsg(BoltMsg::kDrive);
		return BoltRsp::kDone;
	}

	// TODO: clicking outside of pieces should show the solution
	_game->branchWin();
	return BoltRsp::kDone;
}

void ColorPuzzle::enterIdle() {
	_task.setNext([=](const BoltMsg& msg) { return runIdle(msg); });
}

BoltRsp ColorPuzzle::runIdle(const BoltMsg& msg) {
	BoltRsp cmd = _game->handlePopup(msg);
	if (cmd != BoltRsp::kPass) {
		return cmd;
	}

	switch (msg.type) {
	case Scene::kClickButton:
		return handleButtonClick(msg.num);
	default:
		return _scene.handleMsg(msg);
	}

	return kDone;
}

void ColorPuzzle::enterMorph() {
	_task.setNext([=](const BoltMsg& msg) { return runMorph(msg); });

	_game->getEngine()->startTimer(_morphTimer, 0);
	_game->getEngine()->requestSmoothAnimation();
}

BoltRsp ColorPuzzle::runMorph(const BoltMsg& msg) {
	_game->getEngine()->runTimer(msg, _morphTimer);

	switch (msg.type) {
	case BoltMsg::kSmoothAnimation:
		if (driveMorph()) {
			driveMove();
		}
		else {
			_game->getEngine()->requestSmoothAnimation();
		}
		break;
	}

	return kDone;
}

void ColorPuzzle::evaluate() {
	if (isSolved()) {
		_game->branchWin();
		return;
	}

	enterIdle();
}

void ColorPuzzle::startMove(int piece, int currState) {
	_state->prevState = _state->state;
	_undone = false;
	_redoPiece = piece;
	_redoCurrState = currState;
	_selectedPiece = piece;
	_transitionStep = 0;
	_game->setUndoAvailable(true);
	driveMove();
}

void ColorPuzzle::driveMove() {
	if (_transitionStep < kNumTransitionSteps) {
		int pieceNum = _pieces[_selectedPiece].transition.piece[_transitionStep];
		int count = _pieces[_selectedPiece].transition.count[_transitionStep];
		++_transitionStep;

		if (pieceNum >= 0) {
			// FIXME: This isn't how it should work...
			morphPiece(pieceNum, (_state->state[pieceNum] + count) % _pieces[pieceNum].numStates);
			return;
		}
	}

	evaluate();
}

void ColorPuzzle::morphPiece(int piece, int state) {
	debug(3, "morphing piece %d to state %d", piece, state);
	int oldState = _state->state[piece];
	_state->state[piece] = state;
	startMorph(&_pieces[piece].palettes, oldState, state, _soundLists[piece].pickSound());
}

void ColorPuzzle::startMorph(BltPaletteMods *paletteMods, int startState, int endState, BltSound &sound) {
	_morphPaletteMods = paletteMods;
	_morphStartState = startState;
	_morphEndState = endState;
	_morphDuration = sound.getNumSamples() / 22; // FIXME: duration seems too long...

	sound.play(_game->getEngine()->_mixer);

	enterMorph();
}

bool ColorPuzzle::driveMorph() {
	if (_morphTimer.ticks < _morphDuration) {
		applyPaletteModBlended(_game->getGraphics(), kFore, *_morphPaletteMods,
							   _morphStartState, _morphEndState,
							   Common::Rational(_morphTimer.ticks, _morphDuration));

		_game->getGraphics()->markDirty();
		return false;
	}

	applyPaletteMod(_game->getGraphics(), kFore, *_morphPaletteMods, _morphEndState);
	_game->getGraphics()->markDirty();
	_morphPaletteMods = nullptr;
	return true;
}

bool ColorPuzzle::isSolved() const {
	bool solved = true;

	for (int i = 0; i < kNumPieces; ++i) {
		if (_state->state[i] != _pieces[i].solution) {
			solved = false;
			break;
		}
	}

	return solved;
}

void ColorPuzzle::draw() {
	for (int i = 0; i < kNumPieces; ++i) {
		applyPaletteMod(_game->getGraphics(), kFore, _pieces[i].palettes, _state->state[i]);
	}
	_scene.redraw();
}

void ColorPuzzle::reset() {
	_state->state.resize(kNumPieces);
	_state->prevState.resize(kNumPieces);
	for (uint i = 0; i < kNumPieces; ++i) {
		_state->state[i] = _initial[i].value;
	}
	_state->prevState = _state->state;
	_undone = false;
	_game->setUndoAvailable(false);
}

} // End of namespace Funhouse
