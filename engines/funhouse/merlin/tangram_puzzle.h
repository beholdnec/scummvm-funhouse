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

#ifndef FUNHOUSE_MERLIN_TANGRAM_PUZZLE_H
#define FUNHOUSE_MERLIN_TANGRAM_PUZZLE_H

#include "funhouse/merlin/merlin.h"
#include "funhouse/merlin/popup.h"
#include "funhouse/boltlib/palette.h"

namespace Funhouse {
	
class TangramPuzzle : public Card {
public:
	void init(MerlinGame *game, Boltlib &boltlib, BltId resId);
	void enter();
	BoltCmd handleMsg(const BoltMsg &msg);

private:
    struct Piece {
		Piece() : placed(false) {}

		BltImage placedImage;
		BltImage unplacedImage;
		bool placed;
        Common::Point pos; // Note that `pos` is relevant only if `placed` is true.
    };

    typedef ScopedArray<Piece> PieceArray;

    int getPieceAtPosition(const Common::Point& pos);
    void drawPieces();

    MerlinGame *_game;
	Graphics *_graphics;

    Popup _popup;

    // Main resources
	BltImage _bgImage;
	BltPalette _palette;
	BltColorCycles _colorCycles;

    // Per-difficulty resources
    BltPalette _forePalette;
	int _gridSpacing;

    // Puzzle state
    PieceArray _pieces;
    int _pieceInHand; // -1 if no piece is in hand
	Common::Point _grabPos;
};

} // End of namespace Funhouse

#endif
