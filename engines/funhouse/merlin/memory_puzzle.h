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
#include "funhouse/merlin/popup.h"
#include "funhouse/scene.h"
#include "common/random.h"

namespace Funhouse {

class MemoryPuzzle : public Card {
public:
    MemoryPuzzle();

	void init(MerlinGame *game, Boltlib &boltlib, BltId resId);
	void enter();
	BoltCmd handleMsg(const BoltMsg &msg);

protected:
	BoltCmd handleButtonClick(int num);

private:
    // TODO: this value probably comes from boltlib.blt somewhere
    const uint32 kAnimPeriod = 50;
    const uint32 kSelectionDelay = 800;
    const uint32 kAnimEndingDelay = 400;

    enum FrameType {
        kProceed = 1,
        kWaitForEnd = -1,
    };

	struct ItemFrame {
		Common::Point pos;
		BltImage image;
        FrameType type;
	};

	typedef ScopedArray<ItemFrame> ItemFrameList;

	struct Item {
		ItemFrameList frames;
		BltPalette palette;
		Common::ScopedPtr<BltColorCycles> colorCycles;
	};

	typedef ScopedArray<Item> ItemList;

    void startPlayback();
    BoltCmd drivePlayback();
    void startAnimation(int itemNum);
    BoltCmd driveAnimation();
    void drawItemFrame(int itemNum, int frameNum);

    MerlinGame *_game;
	Graphics *_graphics;
    IBoltEventLoop *_eventLoop;
	Scene _scene;
    Popup _popup;
	ItemList _itemList;
    int _maxMemorize;

    Common::RandomSource _random;
    int _curMemorize;
    int _matches;
    ScopedArray<int> _solution;

    bool _playbackActive;
    int _playbackStep;

    bool _animationActive;
    bool _animationEnding;
    int _itemToAnimate;
    uint32 _animStartTime;
    uint32 _frameTime;
    uint32 _frameDelay;
    int _frameNum;
};

} // End of namespace Funhouse

#endif
