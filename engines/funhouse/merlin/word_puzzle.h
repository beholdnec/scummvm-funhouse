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

namespace Funhouse {

class WordPuzzle : public Card {
public:
	void init(MerlinGame *game, Boltlib &boltlib, BltId resId);
	void enter();
	BoltCmd handleMsg(const BoltMsg &msg);

protected:
	BoltCmd handleButtonClick(int num);

private:
	static const int kNumLetters = 26;

	void arrangeButtons();

    MerlinGame *_game;
    PopupMenu _popup;
	Scene _scene;

	BltSprites _normalSprites;
	BltSprites _highlightedSprites;
	BltSprites _selectedSprites;
	BltU8Values _charWidths;

	int16 _centerX;
	int _numChars;
	int _numLines;
	BltU8Values _lineYPositions;
	BltU8Values _lineLengths;
	BltU8Values _solution;

	int _selectedChar; // -1 if no character selected
	int _letterAssignments[kNumLetters];
};

} // End of namespace Funhouse

#endif

