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
#include "funhouse/merlin/popup_menu.h"
#include "funhouse/boltlib/palette.h"

namespace Funhouse {
	
struct BltPotionPuzzleComboTableElement {
	static const uint32 kType = kBltPotionPuzzleComboTable;
	static const uint kSize = 0x6;
	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		a = src.getInt8At(0);
		b = src.getInt8At(1);
		c = src.getInt8At(2);
		d = src.getInt8At(3);
		movie = src.getUint16BEAt(4);
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
	void init(MerlinGame *game, Boltlib &boltlib, int challengeIdx);
	void enter();
	BoltRsp handleMsg(const BoltMsg &msg);

private:
	static const int kNoIngredient = -1; // NOTE: original game uses 0xFE for this value...
	// TODO: Placing an ingredient should last as long as the "plunk" sound... I think.
	static const uint32 kPlacing1Time = 500;
	static const uint32 kPlacing2Time = 500;

	void idle();
	BoltRsp handleIdle(const BoltMsg &msg);
	void evaluate();

	BoltRsp handleClick(Common::Point point);
	BoltRsp requestIngredient(int ingredient);
	BoltRsp requestUndo();
	BoltRsp performReaction();
	void reset();

	bool isValidIngredient(int ingredient) const;
	int getNumRemainingIngredients() const;
	void setTimeout(int32 delay, std::function<void()> then);

	void draw();

	static const int kNumBowlPoints = 3;

	MerlinGame *_game;
	DynamicMode _mode;
	Timer _timer;
	BltImage _bgImage;
	BltPalette _bgPalette;
	Common::Point _origin;
	int _numIngredients;
	ScopedArray<BltImage> _ingredientImages;
	ScopedArray<Common::Point> _shelfPoints;
	Common::Point _bowlPoints[3];
	BltPotionPuzzleComboTable _reactionTable;

	ScopedArray<bool> _shelfSlotOccupied; // False: Empty; True: Filled
	static const int kNumBowlSlots = 3;
	int _bowlSlots[kNumBowlSlots]; // Ingredients in bowl
	int _requestedIngredient;
};

} // End of namespace Funhouse

#endif
