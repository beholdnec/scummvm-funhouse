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

#ifndef FUNHOUSE_MERLIN_COLOR_PUZZLE_H
#define FUNHOUSE_MERLIN_COLOR_PUZZLE_H

#include "funhouse/merlin/merlin.h"
#include "funhouse/scene.h"

namespace Funhouse {

struct BltColorPuzzleTransition { // type 58
    static const uint32 kType = kBltColorPuzzleTransition;
    static const uint kSize = 8;
    void load(const ConstSizedDataView<kSize> src, Boltlib &boltlib) {
        for (int i = 0; i < 4; ++i) {
            piece[i] = src.readInt8(i * 2);
            count[i] = src.readUint8(i * 2 + 1);
        }
    }

    int8 piece[4];
    uint8 count[4];
};

class ColorPuzzle : public Card {
public:
	void init(Graphics *graphics, IBoltEventLoop *eventLoop, Boltlib &boltlib, BltId resId);
	void enter();
	BoltCmd handleMsg(const BoltMsg &msg);

private:
	// All color puzzles in Merlin's Apprentice have 4 pieces.
	static const int kNumPieces = 4;
    static const int kNumTransitionSteps = 4;
	// FIXME: morph duration is probably set in game data
	// or it may last as long as the sound
	static const uint kMorphDuration = 500;

	struct Piece {
		int numStates;
		BltPaletteMods palettes;
		int currentState;
        BltColorPuzzleTransition transition;
	};

	BoltCmd handleButtonClick(int num);

	void enterWaitForPlayerMode();
	void enterTransitionMode();
	void selectPiece(int piece);
	void setPieceState(int piece, int state);
	void morphPiece(int piece, int state);
    void startMorph(BltPaletteMods *paletteMods, int startState, int endState);

	Graphics *_graphics;
	IBoltEventLoop *_eventLoop;
	Scene _scene;

	Piece _pieces[kNumPieces];

    // TRANSITIONING

    bool _transitioning;
    int _selectedPiece;
    int _transitionStep;

	// MORPHING

    bool _morphing;
	uint32 _morphStartTime;
	BltPaletteMods *_morphPaletteMods;
	int _morphStartState;
	int _morphEndState;
};

} // End of namespace Funhouse

#endif
