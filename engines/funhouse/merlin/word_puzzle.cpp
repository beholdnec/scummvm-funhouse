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

	Common::RandomSource random_("RuneA");
	_runeA = random_.getRandomNumber(kLetterCount - 1);

	reset();
}

void WordPuzzle::enter() {
	_scene.enter();
	draw();
	idle();
}

BoltRsp WordPuzzle::handleMsg(const BoltMsg &msg) {
	_modeCtx.react(msg);
	return kDone;
}

void WordPuzzle::handleReset() {
	_resetSound.play(_game->getEngine()->_mixer);
	reset();
	draw();
}

void WordPuzzle::idle() {
	_idleMode = {};
	_idleMode.onMsg([this](const BoltMsg& msg) {
		BoltRsp cmd = _game->handlePopup(&_modeCtx, msg);
		if (cmd != BoltRsp::kPass) {
			return cmd;
		}

		switch (msg.type) {
		case Scene::kClickButton:
			return handleButtonClick(msg.num);
		default:
			return _scene.handleMsg(msg);
		}
	});

	_modeCtx.setNextMode(&_idleMode);
}

BoltRsp WordPuzzle::handleButtonClick(int num) {
	debug(3, "Clicked button %d", num);

	if (num == -1) {
		return BoltRsp::kDone;
	}

	if (num < kLetterCount) {
		clickGlyph(num);
	} else {
		clickGlyph(_board[num - kLetterCount]);
	}

	draw();

	if (isSolved()) {
		_game->branchWin();
	}

	return BoltRsp::kDone;
}

void WordPuzzle::reset() {
	_selectedGlyph = -1;

	for (int i = 0; i < kLetterCount; ++i) {
		_rack[i] = true;
	}

	_board.resize(_charCount);
	for (int i = 0; i < _charCount; ++i) {
		if (_solution[i].value == kSpace) {
			_board[i] = kSpace;
		} else {
			_board[i] = kLetterCount + (_solution[i].value + _runeA) % kLetterCount;
		}
	}

	_prevBoard = _board;

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

void WordPuzzle::swapGlyphs(int from, int to) {
	debug(3, "Swapping glyphs %d and %d", from, to);

	for (int i = 0; i < _charCount; ++i) {
		_prevBoard[i] = _board[i];
		if (_board[i] == from) {
			_board[i] = to;
		} else if (_board[i] == to) {
			_board[i] = from;
		}
	}

	// TODO: mark letters placed
}

void WordPuzzle::computeBoardRects() {
	_boardRects.resize(_charCount);

	int curChar = 0;
	for (int lineNum = 0; lineNum < _lineCount; ++lineNum) {
		int lineLength = _lineLengths[lineNum].value;

		int lineLengthInPixels = 0;
		for (int charNumber = 0; charNumber < lineLength; ++charNumber) {
			int glyph = _board[curChar + charNumber];
			lineLengthInPixels += _charWidths[glyph].value;
		}

		int x = _centerX - lineLengthInPixels / 2;
		int y = _lineYPositions[lineNum].value;

		for (int charNum = 0; charNum < lineLength; ++charNum) {
			int glyph = _board[curChar];

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
	
	// Setup board buttons
	for (int i = 0; i < _charCount; ++i) {
		Scene::Button &button = _scene.getButton(kLetterCount + i);
		button.setHotspot(Scene::HotspotType::kRect, _boardRects[i]);
		if (_board[i] == kSpace) {
			button.setEnable(false);
			button.setGraphics(nullptr);
		} else {
			button.setEnable(true);
			button.setGraphics(_glyphButtonGraphics[_board[i]]);
		}
	}

	_scene.redraw();

	// Draw board sprites
	_boardSprites.reset(new Common::Array<SharedSprite>(_charCount));
	for (int i = 0; i < _charCount; ++i) {
		(*_boardSprites)[i].reset(new Sprite);
		(*_boardSprites)[i]->pos.x = _boardRects[i].left;
		(*_boardSprites)[i]->pos.y = _boardRects[i].top;
		if (_board[i] == kSpace) {
			(*_boardSprites)[i]->image = nullptr;
		} else {
			(*_boardSprites)[i]->image = (*_normalSprites)[_board[i]]->image;
		}
	}
	drawSprites(_game->getEngine()->getGraphics()->getPlaneSurface(kFore), _boardSprites, true, _scene.getOrigin());
}

bool WordPuzzle::isSolved() {
	for (int i = 0; i < _solution.size(); ++i) {
		if (_board[i] != _solution[i].value) {
			return false;
		}
	}

	return true;
}

} // End of namespace Funhouse
