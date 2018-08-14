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

#ifndef FUNHOUSE_MERLIN_POTION_PUZZLE_H
#define FUNHOUSE_MERLIN_POTION_PUZZLE_H

#include "funhouse/merlin/merlin.h"
#include "funhouse/boltlib/palette.h"

namespace Funhouse {
	
struct BltPotionPuzzleComboTableElement {
	static const uint32 kType = kBltPotionPuzzleComboTable;
	static const uint kSize = 0x6;
	void load(const ConstSizedDataView<kSize> src, Boltlib &boltlib) {
		a = src.readInt8(0);
		b = src.readInt8(1);
		c = src.readInt8(2);
		d = src.readInt8(3);
		movie = src.readUint16BE(4);
	}

	int8 a;
	int8 b;
	int8 c;
	int8 d;
	uint16 movie;
};

typedef ScopedArray<BltPotionPuzzleComboTableElement> BltPotionPuzzleComboTable;

class PotionPuzzle : public Card {
public:
	// From Card
	void init(MerlinGame *game, IBoltEventLoop *eventLoop, Boltlib &boltlib, BltId resId);
	void enter();
	BoltCmd handleMsg(const BoltMsg &msg);

private:
	static const int kNoIngredient = -1; // NOTE: original game uses 0xFE for this value...
	// TODO: Placing an ingredient should last as long as the "plunk" sound... I think.
	static const uint32 kPlacing1Time = 500;
	static const uint32 kPlacing2Time = 500;

	enum Mode {
		kWaitForPlayer,
		kTransition
	};

	void enterWaitForPlayerMode();
	void enterTransitionMode();

	BoltCmd driveWaitForPlayer(const BoltMsg &msg);
	BoltCmd driveTransition(const BoltMsg &msg);
	BoltCmd driveTimeout(const BoltMsg &msg);

	BoltCmd handleClick(Common::Point point);
	BoltCmd requestIngredient(int ingredient);
	BoltCmd requestUndo();
	BoltCmd performReaction();
	void reset();

	bool isValidIngredient(int ingredient) const;
	int getNumRemainingIngredients() const;
	void setTimeout(uint32 length);

	void draw();

	static const int kNumBowlPoints = 3;

	MerlinGame *_game;
	IBoltEventLoop *_eventLoop;
	Graphics *_graphics;
	BltImage _bgImage;
	BltPalette _bgPalette;
	Common::Point _origin;
	int _numIngredients;
	BltU8Values _ingredientNums;
	ScopedArray<BltImage> _ingredientImages;
	ScopedArray<Common::Point> _shelfPoints;
	Common::Point _bowlPoints[3];
	BltPotionPuzzleComboTable _reactionTable;
	
	Mode _mode;

	ScopedArray<bool> _shelfSlotOccupied; // False: Empty; True: Filled
	static const int kNumBowlSlots = 3;
	int _bowlSlots[kNumBowlSlots]; // Ingredients in bowl
	int _requestedIngredient;
	
	bool _timeoutActive;
	uint32 _timeoutStart;
	uint32 _timeoutLength;
};

} // End of namespace Funhouse

#endif
