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

#ifndef FUNHOUSE_MERLIN_MEMORY_PUZZLE_H
#define FUNHOUSE_MERLIN_MEMORY_PUZZLE_H

#include "funhouse/merlin/merlin.h"
#include "funhouse/scene.h"

namespace Funhouse {

class MemoryPuzzle : public Card {
public:
	void init(Graphics *graphics, IBoltEventLoop *eventLoop, Boltlib &boltlib, BltId resId);
	void enter();
	BoltCmd handleMsg(const BoltMsg &msg);

protected:
	BoltCmd handleButtonClick(int num);

private:
    // TODO: this value probably comes from boltlib.blt somewhere
    const uint32 kAnimPeriod = 50;
    const uint32 kSelectionDelay = 800;

    enum State {
        kIdle,
        kSelecting,
    };

	struct ItemFrame {
		Common::Point pos;
		BltImage image;
	};

	typedef ScopedArray<ItemFrame> ItemFrameList;

	struct Item {
		ItemFrameList frames;
		BltPalette palette;
		Common::ScopedPtr<BltColorCycles> colorCycles;
	};

	typedef ScopedArray<Item> ItemList;

    void drawItemFrame(int itemNum, int frameNum);

    State _state;

	Graphics *_graphics;
    IBoltEventLoop *_eventLoop;
	Scene _scene;
	ItemList _itemList;

    uint32 _selectionTime;
    uint32 _animFrameTime;
    int _animFrameNum;
    int _selectedItem;
};

} // End of namespace Funhouse

#endif
