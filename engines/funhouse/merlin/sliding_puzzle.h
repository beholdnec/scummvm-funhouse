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

#ifndef FUNHOUSE_MERLIN_SLIDING_PUZZLE_H
#define FUNHOUSE_MERLIN_SLIDING_PUZZLE_H

#include "funhouse/merlin/merlin.h"
#include "funhouse/merlin/popup_menu.h"
#include "funhouse/scene.h"

namespace Funhouse {

class SlidingPuzzle : public Card {
public:
	void init(MerlinGame *game, Boltlib &boltlib, int challengeIdx);
	void enter();
	BoltRsp handleMsg(const BoltMsg &msg);
	void setSprites();

private:
	BoltRsp handlePopupButtonClick(int num);
	BoltRsp handleButtonClick(int num);

	MerlinGame *_game;
	Scene _scene;

	static const int kNumButtons = 4;

	BltU8Values _moveTables[kNumButtons * 2]; // 0-3: backward; 4-7: forward
	ScopedArray<int> _pieces;
};

} // End of namespace Funhouse

#endif
