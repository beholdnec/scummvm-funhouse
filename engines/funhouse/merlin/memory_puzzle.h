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

#include <functional>

#include "funhouse/boltlib/sound.h"
#include "funhouse/merlin/merlin.h"
#include "funhouse/merlin/popup_menu.h"
#include "funhouse/scene.h"
#include "common/random.h"

namespace Funhouse {

enum {
    kFrameTimer = 0,
    kAnimTimer = 1,
    kMPTimerCount,
};

struct Timer {
    bool armed = false;
    int id = -1;
    int32 ticks = 0;
    int32 elapse = 0;
    std::function<void()> fn;
};

class Mode {
public:
    void onEnter(std::function<void()> fn);
    void onMsg(std::function<void(const BoltMsg &msg)> fn);
    void onTimer(int id, std::function<void()> fn);
    int32 getTimerTicks(int id) const;
    void setTimer(int id, int32 ticks, int32 elapse, bool arm);

private:
    friend class MemoryPuzzle;

    std::function<void()> _onEnter;
    std::function<void(const BoltMsg &msg)> _onMsg;

    bool _active = false;
    Timer _timers[kMPTimerCount];
};

class MemoryPuzzle : public Card {
public:
    MemoryPuzzle();

	void init(MerlinGame *game, Boltlib &boltlib, int challengeIdx);
	void enter();
	BoltRsp handleMsg(const BoltMsg &msg);

private:
    const uint32 kFrameDelayMs = 50;
    const uint32 kMinAnimPlayTimeMs = 400;

	struct ItemFrame {
		Common::Point pos;
		BltImage image;
        int16 delayFrames; // In units of 50ms; -1 triggers pause and wind-down
	};

	typedef ScopedArray<ItemFrame> ItemFrameList;

	struct Item {
		ItemFrameList frames;
		BltPalette palette;
		Common::ScopedPtr<BltColorCycles> colorCycles;
		BltSound sound;
	};

	typedef ScopedArray<Item> ItemList;

	BoltRsp handlePopupButtonClick(int num);
	BoltRsp handleButtonClick(int num);
    void startPlayback();
    void startAnimation(int itemNum, BltSound& sound);
    void drawItemFrame(int itemNum, int frameNum);

    void idle();
    void playbackNext();
    void animPlaying();
    void animWindingDown();
    void animStopping();

    MerlinGame *_game;
	Scene _scene;
    PopupMenu _popup;
	ItemList _itemList;
    int _finalGoal;
    uint16 _foo; // Parameter used to determine puzzle variant? Also used to override animation timing!
	BltSoundList _failSound;

    Common::RandomSource _random;
    int _goal;
    int _matches;
    ScopedArray<int> _solution;

    Mode _animMode;
    std::function<void()> _animThen; // Function to call when anim is finished
    int _playbackStep = 0;
    int _animItem;
    int _animFrame;
    int _animSubFrame;
    uint32 _animPlayTime;
    uint32 _animSoundTime; // in ms
};

} // End of namespace Funhouse

#endif
