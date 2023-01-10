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

#define FORBIDDEN_SYMBOL_ALLOW_ALL // fix #include <functional>

#include "funhouse/boltlib/sound.h"
#include "funhouse/merlin/merlin.h"
#include "funhouse/merlin/popup_menu.h"
#include "funhouse/scene.h"
#include "common/random.h"

namespace Funhouse {
	
struct BltMemoryPuzzleInfo {
	static const uint32 kType = kBltMemoryPuzzleInfos;
	static const uint kSize = 0x10;
	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		pieceCount = src.getUint16BEAt(0x0);
		solutionLength = src.getUint16BEAt(0x2);
		// TODO: the rest of the fields appear to be timing parameters
		foo = src.getUint16BEAt(0x8);
		failTimeout = src.getUint16BEAt(0xc);
		// FIXME: At 0xe there is an alternate fail timeout used in a different situation. Please investigate.
	}

	uint16 pieceCount;
	uint16 solutionLength;
	uint16 foo;
	uint16 failTimeout;
};

typedef Common::Array<BltMemoryPuzzleInfo> BltMemoryPuzzleInfos;

class MemoryPuzzle : public Card {
public:
	void init(MerlinGame *game, Boltlib &boltlib, int challengeIdx);
	void enter() override;
	BoltRsp handleMsg(const BoltMsg &msg) override;
	void handleReset() override;

private:
	const uint32 kFrameDelayMs = 50;
	const uint32 kMinAnimPlayTimeMs = 400;

	struct ItemFrame {
		Common::Point pos;
		BltImage image;
		int16 delayFrames; // In units of 50ms; -1 triggers pause and wind-down
	};

	typedef Common::Array<ItemFrame> ItemFrameList;

	struct Item {
		ItemFrameList frames;
		BltPalette palette;
		Common::ScopedPtr<BltColorCycles> colorCycles;
		BltSound sound;
	};

	typedef Common::Array<Item> ItemList;

	BoltRsp handleButtonClick(int num);
	void startPlayback();
	void startAnimation(int itemNum, BltSound& sound, std::function<void()> then);
	void drawItemFrame(int itemNum, int frameNum);

	void enterIdle();
	BoltRsp runIdle(const BoltMsg &msg);
	void playbackNext();
	void enterAnimPlaying();
	BoltRsp animPlaying(const BoltMsg &msg);
	void enterAnimWindingDown();
	BoltRsp animWindingDown(const BoltMsg &msg);
	void enterAnimStopping();
	BoltRsp animStopping(const BoltMsg &msg);

	MerlinGame *_game;
	Scene _scene;
	ItemList _itemList;
	BltMemoryPuzzleInfo _puzzleInfo;
	BltSoundList _failSound;

	int _goal;
	int _matches;
	Common::Array<int> _solution;

	TaskRunner _task;
	Timer _frameTimer;
	Timer _animTimer;
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
