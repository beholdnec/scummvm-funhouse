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

#ifndef FUNHOUSE_MERLIN_WORD_PUZZLE_H
#define FUNHOUSE_MERLIN_WORD_PUZZLE_H

#include "funhouse/merlin/merlin.h"
#include "funhouse/merlin/popup_menu.h"
#include "funhouse/scene.h"
#include "funhouse/boltlib/sound.h"

/*
* Welcome to the word puzzle code. In order to keep this code as clear as
* possible, the following vocabulary is used.
*
* Rack: The location where letters are held before they are placed on the board.
* Not all letters are used in the solution. Named after the little wooden tile
* holders in Scrabble, which is called a letter rack.
*
* Board: The location where the player places letters to form the solution.
*
* Letter: An English letter A-Z. Represented by the number 0-25.
*
* Rune: A mysterious rune marking an unplaced letter. There are 26 possible
* runes. Represented by the number 26-51. The game allows the player to swap
* runes with other runes.
*
* Glyph: A value representing a letter, rune or space. The values are:
*     -1:    No glyph.
*     0-25:  Letter A-Z.
*     26-51: Rune 1-26.
*     52:    Space.
 */

namespace Funhouse {

class WordPuzzle : public Card {
public:
	void init(MerlinGame *game, Boltlib &boltlib, int challengeIdx);
	void enter() override;
	BoltRsp handleMsg(const BoltMsg &msg) override;
	void handleReset() override;

private:
	static const int kLetterCount = 26;
	static const int kGlyphCount = 53; // 26 letters + 26 runes + 1 space
	static const uint8 kSpace = 52;

	void idle();
	BoltRsp handleButtonClick(int num);

	void reset();
	void clickGlyph(int glyph);
	void swapGlyphs(int from, int to);
	void computeBoardRects();
	void draw();
	bool isSolved();
	int getGlyphFromButton(int button) const;

	MerlinGame *_game;
	Scene _scene;
	ModeContext _modeCtx;
	DynamicMode _idleMode;
	BltSoundList _resetSound;

	SharedSpriteList _normalSprites;
	SharedSpriteList _highlightedSprites;
	SharedSpriteList _selectedSprites;
	BltU8Values _charWidths;
	SharedButtonGraphics _glyphButtonGraphics[kGlyphCount];
	Common::Array<SharedImage> _blankPics;

	int16 _centerX;
	uint8 _lineHeight;
	int _charCount;
	int _lineCount;
	BltU8Values _lineYPositions;
	BltU8Values _lineLengths;
	BltU8Values _solution;

	int _selectedGlyph = -1; // -1: No selection; 0-25: English; 26-51: Runes
	bool _letterIsPlaced[kLetterCount] = { 0 }; // False if letter is in rack; true if letter is placed on board
	Common::Array<uint8> _board;
	Common::Array<uint8> _prevBoard;
	Common::Array<Rect> _boardRects;
	SharedSpriteList _boardSprites;
	int _runeA; // Rune assigned to letter A (0-25). Randomly assigned once at load time.
};

} // End of namespace Funhouse

#endif

