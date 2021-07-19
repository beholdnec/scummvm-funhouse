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
		centerX = src.getInt16BEAt(2);
	}

	int16 centerX;
};
	
struct BltWordPuzzleVariantInfo {
	static const uint32 kType = kBltWordPuzzleVariantInfo;
	static const uint kSize = 0x4;
	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		numChars = src.getUint8At(0);
		numLines = src.getUint8At(1);
		// TODO: more fields
	}

	uint8 numChars;
	uint8 numLines;
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
	BltId charWidthsId            = resourceList[10].value; // Ex: 61B3
	BltId resetSoundId            = resourceList[11].value; // Ex: 61D9

	BltWordPuzzleInfo puzzleInfo;
	loadBltResource(puzzleInfo, boltlib, infoId);
	_centerX = puzzleInfo.centerX;

	_resetSound.load(boltlib, resetSoundId);

	_normalSprites.load(boltlib, normalSpriteListId);
	_highlightedSprites.load(boltlib, highlightedSpriteListId);
	_selectedSprites.load(boltlib, selectedSpriteListId);
	loadBltResourceArray(_charWidths, boltlib, charWidthsId);

	BltU16Values difficulties;
	loadBltResourceArray(difficulties, boltlib, difficultiesId);
	BltId difficultyId = BltShortId(difficulties[_game->getDifficulty(kWordsDifficulty)].value); // Ex: 5E18

	int puzzleVariant = 0; // TODO: Choose puzzle variant 0-3

	BltResourceList difficulty;
	loadBltResourceArray(difficulty, boltlib, difficultyId);
	BltId variantInfoId    = difficulty[puzzleVariant].value; // Ex: 5E00
	BltId lineLengthsId    = difficulty[4 + puzzleVariant].value; // Ex: 5E01
	BltId lineYPositionsId = difficulty[8 + puzzleVariant].value; // Ex: 5E02
	BltId solutionId       = difficulty[12 + puzzleVariant].value; // Ex: 5E03
	BltId sceneId          = difficulty[16 + puzzleVariant].value; // Ex: 5E05

	BltWordPuzzleVariantInfo variantInfo;
	loadBltResource(variantInfo, boltlib, variantInfoId);
	_numChars = variantInfo.numChars;
	_numLines = variantInfo.numLines;

	loadBltResourceArray(_lineLengths, boltlib, lineLengthsId);
	loadBltResourceArray(_lineYPositions, boltlib, lineYPositionsId);
	loadBltResourceArray(_solution, boltlib, solutionId);

	loadScene(_scene, _game->getEngine(), boltlib, sceneId);

	reset();
}

void WordPuzzle::enter() {
	_scene.enter();
	setupButtons();
}

BoltRsp WordPuzzle::handleMsg(const BoltMsg &msg) {
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
}

BoltRsp WordPuzzle::handlePopupButtonClick(int num) {
	switch (num) {
	case 0: // Return
		_game->branchReturn();
		return BoltRsp::kDone;
	case 3: // Reset
		return handleReset();
	default:
		warning("Unhandled popup button %d", num);
		return BoltRsp::kDone;
	}
}

BoltRsp WordPuzzle::handleReset() {
	_game->dismissPopup();
	_resetSound.play(_game->getEngine()->_mixer);
	reset();
	setupButtons();
	return BoltRsp::kDone;
}

BoltRsp WordPuzzle::handleButtonClick(int num) {
	debug(3, "Clicked button %d", num);

	if (num == -1) {
		return BoltRsp::kDone;
	}

	int selectedLetter = glyphToLetter(_selectedGlyph);
	int selectedRune = glyphToRune(_selectedGlyph);

	int clickedLetter = -1;
	if (num >= 0 && num < kNumLetters) {
		clickedLetter = num;
	}

	int clickedRune = -1;
	if (num >= kNumLetters) {
		clickedRune = reinterpret_cast<intptr_t>(_scene.getButton(num).getUserData());
	}

	// TODO: implement unselecting
	// TODO: prevent assigning a letter to more than one rune
	// TODO: assigned letters should disappear from the box
	if (_selectedGlyph == -1) {
		if (num >= 0 && num < kNumLetters) {
			// Select letter
			_selectedGlyph = letterToGlyph(num);
		}
		else {
			// Select rune
			// Note that a rune will be selected even if the player clicks on a rune that has been
			// assigned to a letter.
			_selectedGlyph = runeToGlyph(reinterpret_cast<intptr_t>(_scene.getButton(num).getUserData()));
		}
	}
	else if (selectedLetter != -1) {
		if (clickedRune != -1) {
			// Assign selected letter to rune
			mapRuneAndLetter(clickedRune, selectedLetter);
			_selectedGlyph = -1;
		}
		else if (clickedLetter != -1) {
			// Select another letter
			_selectedGlyph = letterToGlyph(clickedLetter);
		}
	}
	else if (selectedRune != -1) {
		if (clickedLetter != -1) {
			// Assign selected rune to letter
			mapRuneAndLetter(selectedRune, clickedLetter);
			_selectedGlyph = -1;
		}
		else if (clickedRune != -1) {
			if (selectedRune != clickedRune && (_runeToLetterMap[selectedRune] != -1 || _runeToLetterMap[clickedRune] != -1)) {
				// Swap rune assignments (FIXME: is this correct behavior?)
				int oldSelectedRuneLetter = _runeToLetterMap[selectedRune];
				int oldClickedRuneLetter = _runeToLetterMap[clickedRune];
				mapRuneAndLetter(selectedRune, oldClickedRuneLetter);
				mapRuneAndLetter(clickedRune, oldSelectedRuneLetter);
				_selectedGlyph = -1;
			}
			else {
				// Select another rune
				_selectedGlyph = runeToGlyph(clickedRune);
			}
		}
	}

	setupButtons();

	if (isSolved()) {
		_game->branchWin();
	}

	return BoltRsp::kDone;
}

void WordPuzzle::reset() {
	_selectedGlyph = -1;
	for (int i = 0; i < kNumLetters; ++i) {
		_runeToLetterMap[i] = -1;
		_letterToRuneMap[i] = -1;
	}
}

int WordPuzzle::glyphToRune(int glyph) const {
	if (glyph >= kNumLetters && glyph < kNumLetters + kNumLetters) {
		return glyph - kNumLetters;
	}

	return -1;
}

int WordPuzzle::glyphToLetter(int glyph) const {
	if (glyph >= 0 && glyph < kNumLetters) {
		return glyph;
	}

	return -1;
}

int WordPuzzle::runeToGlyph(int rune) const {
	assert(rune >= 0 && rune < kNumLetters);
	return kNumLetters + rune;
}

int WordPuzzle::letterToGlyph(int letter) const {
	assert(letter >= 0 && letter < kNumLetters);
	return letter;
}

void WordPuzzle::mapRuneAndLetter(int rune, int letter) {
	assert(rune == -1 || (rune >= 0 && rune < kNumLetters));
	assert(letter == -1 || (letter >= 0 && letter < kNumLetters));

	// Remove existing mappings
	if (rune != -1 && _runeToLetterMap[rune] != -1) {
		_letterToRuneMap[_runeToLetterMap[rune]] = -1;
		_runeToLetterMap[rune] = -1;
	}
	if (letter != -1 && _letterToRuneMap[letter] != -1) {
		_runeToLetterMap[_letterToRuneMap[letter]] = -1;
		_letterToRuneMap[letter] = -1;
	}

	// Add new mapping
	if (rune != -1) {
		_runeToLetterMap[rune] = letter;
	}
	if (letter != -1) {
		_letterToRuneMap[letter] = rune;
	}
}

void WordPuzzle::setupButtons() {
	int curChar = 0;
	for (int lineNum = 0; lineNum < _numLines; ++lineNum) {
		int lineLength = _lineLengths[lineNum].value;

		int lineLengthInPixels = 0;
		for (int charNumber = 0; charNumber < lineLength; ++charNumber) {
			int ch = _solution[curChar + charNumber].value;
			if (ch >= 0 && ch < kNumLetters) {
				int glyph = _runeToLetterMap[ch];
				if (glyph == -1) {
					glyph = runeToGlyph(ch);
				}
				lineLengthInPixels += _charWidths[glyph].value;
			} else {
				lineLengthInPixels += _charWidths[ch].value;
			}
		}

		int x = _centerX - lineLengthInPixels / 2;
		int y = _lineYPositions[lineNum].value;

		for (int charNum = 0; charNum < lineLength; ++charNum) {
			int ch = _solution[curChar].value;
			if (ch >= 0 && ch < kNumLetters) {
				int glyph = _runeToLetterMap[ch];
				if (glyph == -1) {
					glyph = runeToGlyph(ch);
				}
				BltImage* selectedSprite = _selectedSprites.getImageFromSet(glyph);
				BltImage* highlightedSprite = (_selectedGlyph == glyph) ? selectedSprite : _highlightedSprites.getImageFromSet(glyph);
				BltImage* normalSprite = (_selectedGlyph == glyph) ? selectedSprite : _normalSprites.getImageFromSet(glyph);
				Scene::Button &button = _scene.getButton(kNumLetters + curChar);
				button.overrideGraphics(Common::Point(x, y), highlightedSprite, normalSprite);
				button.setUserData(reinterpret_cast<void*>(ch));
				x += _charWidths[glyph].value;
			} else {
				x += _charWidths[ch].value;
			}

			++curChar;
		}
	}

	// Redraw entire scene (FIXME: avoid doing this)
	_scene.redraw();
}

bool WordPuzzle::isSolved() {
	for (int i = 0; i < _solution.size(); ++i) {
		// The puzzle is solved when all runes in the solution are mapped to their corresponding letters
		int ch = _solution[i].value;
		if (ch >= 0 && ch < kNumLetters) {
			if (_runeToLetterMap[ch] != ch) {
				return false;
			}
		}
	}

	return true;
}

} // End of namespace Funhouse
