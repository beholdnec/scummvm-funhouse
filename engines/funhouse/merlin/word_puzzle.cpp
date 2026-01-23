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

#include "funhouse/merlin/word_puzzle.h"

namespace Funhouse {

struct BltWordPuzzleInfo {
	static const uint32 kType = kBltWordPuzzleInfo;
	static const uint kSize = 0x4;
	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		variationSlot = src.getUint8At(0x0);
		lineHeight = src.getUint8At(0x1);
		centerX = src.getInt16BEAt(0x2);
	}

	uint8 variationSlot;
	uint8 lineHeight;
	int16 centerX;
};
	
struct BltWordPuzzleVariantInfo {
	static const uint32 kType = kBltWordPuzzleVariantInfo;
	static const uint kSize = 0x4;
	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		charCount = src.getUint8At(0x0);
		lineCount = src.getUint8At(0x1);
		// TODO: more fields
	}

	uint8 charCount;
	uint8 lineCount;
};

void WordPuzzle::init(MerlinGame *game, Boltlib &boltlib, int challengeIdx) {
	_game = game;

	uint16 resId = 0;
	switch (challengeIdx) {
	case 0: resId = 0x61E3; break;
	case 7: resId = 0x69E1; break;
	case 17: resId = 0x65E1; break;
	default: assert(false); break;
	}

	_game->setPopup(MerlinGame::kPuzzlePopup);

	BltResourceList resourceList;
	loadBltResourceArray(resourceList, boltlib, BltShortId(resId));
	BltId difficultiesId          = resourceList[0].value;  // Ex: 6100
	BltId infoId                  = resourceList[1].value;  // Ex: 6101
	BltId normalSpriteListId      = resourceList[2].value;  // Ex: 61B4
	BltId highlightedSpriteListId = resourceList[3].value;  // Ex: 61B5
	BltId selectedSpriteListId    = resourceList[4].value;  // Ex: 61B6
	BltId glyphButtonGraphicsId   = resourceList[5].value; // Ex: 61D2
	BltId glyphPicsId             = resourceList[6].value; // Ex: 61B2
	BltId letterPicsId            = resourceList[7].value; // Ex: 61B1
	BltId blankPicsId             = resourceList[8].value; // Ex: 61B0
	BltId charWidthsId            = resourceList[10].value; // Ex: 61B3
	BltId resetSoundId            = resourceList[11].value; // Ex: 61D9

	BltWordPuzzleInfo puzzleInfo;
	loadBltResource(puzzleInfo, boltlib, infoId);
	_centerX = puzzleInfo.centerX;
	_lineHeight = puzzleInfo.lineHeight;

	int difficultyLevel = _game->getDifficulty(kWordsDifficulty);
	int variation = (_game->getVariationSlot(puzzleInfo.variationSlot) + 1) % 4;
	debug(3, "Loading word puzzle difficulty %d, variation %d", difficultyLevel, variation);

	_resetSound.load(boltlib, resetSoundId);

	_normalSprites = loadBltSprites(boltlib, normalSpriteListId);
	_highlightedSprites = loadBltSprites(boltlib, highlightedSpriteListId);
	_selectedSprites = loadBltSprites(boltlib, selectedSpriteListId);

	BltResourceList blankPicsIds;
	loadBltResourceArray(blankPicsIds, boltlib, blankPicsId);
	_blankPics.resize(kLetterCount);
	for (int i = 0; i < kLetterCount; ++i) {
		_blankPics[i].reset(new BltImage);
		_blankPics[i]->load(boltlib, blankPicsIds[i].value);
	}

	loadBltResourceArray(_charWidths, boltlib, charWidthsId);

	BltResourceList glyphButtonGraphicsIds;
	loadBltResourceArray(glyphButtonGraphicsIds, boltlib, glyphButtonGraphicsId);
	for (int i = 0; i < kGlyphCount - 1; ++i) {
		_glyphButtonGraphics[i] = loadButtonGraphics(boltlib, glyphButtonGraphicsIds[i].value);
		(*_glyphButtonGraphics[i])[0].idleSprite = (*_normalSprites)[i];
		(*_glyphButtonGraphics[i])[0].hoveredSprite = (*_highlightedSprites)[i];
		(*_glyphButtonGraphics[i])[1].idleSprite = (*_selectedSprites)[i];
		(*_glyphButtonGraphics[i])[1].hoveredSprite = (*_selectedSprites)[i];
		// Do not set glyph 52 (space)
	}

	BltU16Values difficulties;
	loadBltResourceArray(difficulties, boltlib, difficultiesId);
	BltId difficultyId = BltShortId(difficulties[difficultyLevel].value); // Ex: 5E18

	BltResourceList difficulty;
	loadBltResourceArray(difficulty, boltlib, difficultyId);
	BltId variantInfoId    = difficulty[variation].value;      // Ex: 5E00
	BltId lineLengthsId    = difficulty[4 + variation].value;  // Ex: 5E01
	BltId lineYPositionsId = difficulty[8 + variation].value;  // Ex: 5E02
	BltId solutionId       = difficulty[12 + variation].value; // Ex: 5E03
	BltId sceneId          = difficulty[16 + variation].value; // Ex: 5E05

	BltWordPuzzleVariantInfo variantInfo;
	loadBltResource(variantInfo, boltlib, variantInfoId);
	_charCount = variantInfo.charCount;
	_lineCount = variantInfo.lineCount;

	loadBltResourceArray(_lineLengths, boltlib, lineLengthsId);
	loadBltResourceArray(_lineYPositions, boltlib, lineYPositionsId);
	loadBltResourceArray(_solution, boltlib, solutionId);

	loadScene(_scene, _game->getEngine(), boltlib, sceneId);

	_state = _game->getChallengeState(challengeIdx).cast<State>();
	if (!_state || _state->difficulty != difficultyLevel || _state->variation != variation) {
		_state.reset(new State());
		_game->setChallengeState(challengeIdx, _state);

		_state->difficulty = difficultyLevel;
		_state->variation = variation;

		Common::RandomSource random_("RuneA");
		_state->runeA = random_.getRandomNumber(kLetterCount - 1);

		reset();
	}
	else {
		for (int i = 0; i < kLetterCount; ++i) {
			_letterIsPlaced[i] = false;
		}
		for (int i = 0; i < _state->board.size(); ++i) {
			if (_state->board[i] < kLetterCount)
				_letterIsPlaced[_state->board[i]] = true;
		}
	}
}

void WordPuzzle::enter() {
	_scene.enter();
	draw();
}

BoltRsp WordPuzzle::handleMsg(const BoltMsg &msg) {
	BoltRsp cmd = _game->handlePopup(msg);
	if (cmd != BoltRsp::kReject) {
		return cmd;
	}

	switch (msg.type) {
	case Scene::kClickButton:
		return handleButtonClick(msg.num);
	case BoltMsg::kHover:
		_scene.handleMsg(msg);
		draw();
		return kContinue;
	default:
		return _scene.handleMsg(msg);
	}

	return kContinue;
}

void WordPuzzle::handleReset() {
	_resetSound.play(_game->getEngine()->_mixer);
	reset();
	draw();
}

BoltRsp WordPuzzle::handleButtonClick(int num) {
	debug(3, "Clicked button %d", num);

	if (num == -1) {
		return BoltRsp::kContinue;
	}

	clickGlyph(getGlyphFromButton(num));

	draw();

	if (isSolved()) {
		_game->branchWin();
	}

	return BoltRsp::kContinue;
}

void WordPuzzle::reset() {
	_selectedGlyph = -1;

	for (int i = 0; i < kLetterCount; ++i) {
		_letterIsPlaced[i] = false;
	}

	_state->board.resize(_charCount);
	for (int i = 0; i < _charCount; ++i) {
		if (_solution[i].value == kSpace) {
			_state->board[i] = kSpace;
		} else {
			_state->board[i] = kLetterCount + (_solution[i].value + _state->runeA) % kLetterCount;
		}
	}

	_state->prevBoard = _state->board;

	_game->setUndoAvailable(false);
}

void WordPuzzle::clickGlyph(int glyph) {
	debug(3, "Clicked glyph %d", glyph);

	if (glyph == kSpace) {
		return;
	}

	if (_selectedGlyph < 0) {
		_selectedGlyph = glyph;
	} else if (_selectedGlyph == glyph) {
		// Deselect glyph
		_selectedGlyph = -1;
	} else {
		swapGlyphs(_selectedGlyph, glyph);
		_selectedGlyph = -1;
	}
}

static bool isVowel(int glyph) {
	// A, E, I, O, U, Y
	return glyph == 0 || glyph == 4 || glyph == 8 || glyph == 14 || glyph == 20 || glyph == 24;
}

void WordPuzzle::swapGlyphs(int from, int to) {
	debug(3, "Swapping glyphs %d and %d", from, to);

	for (int i = 0; i < _charCount; ++i) {
		_state->prevBoard[i] = _state->board[i];
		if (_state->board[i] == from) {
			_state->board[i] = to;
		} else if (_state->board[i] == to) {
			_state->board[i] = from;
		}
	}

	bool fromBoard = from >= kLetterCount || _letterIsPlaced[from];
	bool toBoard = to >= kLetterCount || _letterIsPlaced[to];
	if (fromBoard && !toBoard) { // from board to rack
		if (from < kLetterCount) {
			_letterIsPlaced[from] = false;
		}
		if (to < kLetterCount) {
			_letterIsPlaced[to] = true;
		}
	} else if (!fromBoard && toBoard) { // from rack to board
		if (from < kLetterCount) {
			_letterIsPlaced[from] = true;
		}
		if (to < kLetterCount) {
			_letterIsPlaced[to] = false;
		}
	}

	// Lock vowels
	for (int i = 0; i < _charCount; ++i) {
		if (isVowel(_state->board[i]) && _state->board[i] == _solution[i].value) {
			_scene.getButton(kLetterCount + i).setEnable(false);
		}
	}
}

void WordPuzzle::computeBoardRects() {
	_boardRects.resize(_charCount);

	int curChar = 0;
	for (int lineNum = 0; lineNum < _lineCount; ++lineNum) {
		int lineLength = _lineLengths[lineNum].value;

		int lineLengthInPixels = 0;
		for (int charNumber = 0; charNumber < lineLength; ++charNumber) {
			int glyph = _state->board[curChar + charNumber];
			lineLengthInPixels += _charWidths[glyph].value;
		}

		int x = _centerX - lineLengthInPixels / 2;
		int y = _lineYPositions[lineNum].value;

		for (int charNum = 0; charNum < lineLength; ++charNum) {
			int glyph = _state->board[curChar];

			_boardRects[curChar].left = x;
			_boardRects[curChar].top = y;
			_boardRects[curChar].right = x + _charWidths[glyph].value - 1;
			_boardRects[curChar].bottom = y + _lineHeight - 1;

			x += _charWidths[glyph].value;

			++curChar;
		}
	}
}

void WordPuzzle::draw() {
	computeBoardRects();

	for (int i = 0; i < kLetterCount; ++i) {
		_scene.getButton(i).setEnable(!_letterIsPlaced[i]);
	}
	
	// Setup board buttons
	for (int i = 0; i < _charCount; ++i) {
		Scene::Button &button = _scene.getButton(kLetterCount + i);
		button.setHotspot(Scene::HotspotType::kRect, _boardRects[i]);
		if (_state->board[i] == kSpace) {
			button.setEnable(false);
			button.setGraphics(nullptr);
		} else {
			button.setGraphics(_glyphButtonGraphics[_state->board[i]]);
		}
	}

	_scene.redraw();

	// Draw blank rectangles over rack letters that have been placed
	// (How delightfully hacky!)
	SharedSpriteList blankSprites(new Common::Array<SharedSprite>);
	for (int i = 0; i < kLetterCount; ++i) {
		if (_letterIsPlaced[i]) {
			SharedSprite sprite(new Sprite);
			sprite->pos.x = _scene.getButton(i).getHotspot().left;
			sprite->pos.y = _scene.getButton(i).getHotspot().top;
			sprite->image = _blankPics[i];
			(*blankSprites).push_back(sprite);
		}
	}
	drawSprites(_game->getEngine()->getGraphics()->getPlaneSurface(kFore), blankSprites, false, _scene.getOrigin());

	// Draw board sprites
	_boardSprites.reset(new Common::Array<SharedSprite>(_charCount));
	for (int i = 0; i < _charCount; ++i) {
		(*_boardSprites)[i].reset(new Sprite);
		(*_boardSprites)[i]->pos.x = _boardRects[i].left;
		(*_boardSprites)[i]->pos.y = _boardRects[i].top;
		if (_state->board[i] == kSpace) {
			(*_boardSprites)[i]->image = nullptr;
		} else if (_state->board[i] == _selectedGlyph) {
			(*_boardSprites)[i]->image = (*_selectedSprites)[_state->board[i]]->image;
		} else if (_state->board[i] == getGlyphFromButton(_scene.getHoveredButton())) {
			(*_boardSprites)[i]->image = (*_highlightedSprites)[_state->board[i]]->image;
		} else {
			(*_boardSprites)[i]->image = (*_normalSprites)[_state->board[i]]->image;
		}
	}
	drawSprites(_game->getEngine()->getGraphics()->getPlaneSurface(kFore), _boardSprites, true, _scene.getOrigin());
}

bool WordPuzzle::isSolved() {
	for (int i = 0; i < _solution.size(); ++i) {
		if (_state->board[i] != _solution[i].value) {
			return false;
		}
	}

	return true;
}

int WordPuzzle::getGlyphFromButton(int button) const {
	if (button < 0) {
		return -1;
	} else if (button < kLetterCount) {
		return button;
	} else {
		return _state->board[button - kLetterCount];
	}
}

} // End of namespace Funhouse
