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
#include "funhouse/merlin/popup_menu.h"
#include "funhouse/scene.h"

namespace Funhouse {

struct BltColorPuzzleTransition { // type 58
    static const uint32 kType = kBltColorPuzzleTransition;
    static const uint kSize = 8;
    void load(Common::Span<const byte> src, Boltlib &boltlib) {
        for (int i = 0; i < 4; ++i) {
            piece[i] = src.getInt8At(i * 2);
            count[i] = src.getUint8At(i * 2 + 1);
        }
    }

    int8 piece[4];
    uint8 count[4];
};

class BltSoundList;

class ColorPuzzle : public Card {
public:
	void init(MerlinGame *game, Boltlib &boltlib, int challengeIdx);
	void enter();
	BoltRsp handleMsg(const BoltMsg &msg);

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
		int state;
        int solution;
        BltColorPuzzleTransition transition;
	};

	BoltRsp handlePopupButtonClick(int num);
	BoltRsp handleButtonClick(int num);

	void idleMode();
    BoltRsp driveTransition();
    BoltRsp driveMorph();
	void selectPiece(int piece);
	void setPieceState(int piece, int state);
	void morphPiece(int piece, int state);
    void startMorph(BltPaletteMods *paletteMods, int startState, int endState);
    bool isSolved() const;

    MerlinGame *_game;
	DynamicMode _mode;
	Scene _scene;
	ScopedArray<BltSoundList> _soundLists;

	Piece _pieces[kNumPieces];

    int _selectedPiece;
    int _transitionStep;

	Timer _morphTimer;
	BltPaletteMods *_morphPaletteMods;
	int _morphStartState;
	int _morphEndState;
};

} // End of namespace Funhouse

#endif
