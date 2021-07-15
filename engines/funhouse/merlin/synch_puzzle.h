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

#ifndef FUNHOUSE_MERLIN_SYNCH_PUZZLE_H
#define FUNHOUSE_MERLIN_SYNCH_PUZZLE_H

#include "funhouse/merlin/merlin.h"
#include "funhouse/merlin/popup_menu.h"
#include "funhouse/scene.h"

namespace Funhouse {

struct BltSynchPuzzleTransitionElement { // type 53
    static const uint32 kType = kBltSynchPuzzleTransition;
    static const uint kSize = 2;
    void load(Common::Span<const byte> src, Boltlib &boltlib) {
        item = src.getInt8At(0);
        count = src.getInt8At(1);
    }

    int8 item;
    int8 count;
};

typedef ScopedArray<BltSynchPuzzleTransitionElement> BltSynchPuzzleTransition;

class SynchPuzzle : public Card {
public:
	void init(MerlinGame *game, Boltlib &boltlib, int challengeIdx);
	void enter();
    BoltRsp handleMsg(const BoltMsg &msg);

private:
    static const int kTimeoutDelay = 250;

    struct Move {
        Move() : item(-1), count(0) {}
        int item;
        int count; // Negative for backwards; Positive for forwards
    };

    struct Item {
        int state;
        int solution;
        BltSprites sprites;
        ScopedArray<BltSynchPuzzleTransition> moveset;
    };

    typedef ScopedArray<Item> ItemArray;

    BoltRsp handlePopupButtonClick(int num);
    BoltRsp handleButtonClick(int num);
    void redraw();
    void idle();
    void setTimeout(int32 delay, std::function<void()> then);
    BoltRsp driveTransition();
    int getItemAtPosition(const Common::Point& pt);
    bool isSolved() const;

    MerlinGame *_game;
	Scene _scene;
    DynamicMode _mode;
    std::function<void()> _timeoutThen;

    ItemArray _items;

    ScopedArray<Move> _moveAgenda;
};

} // End of namespace Funhouse

#endif
