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

#include "funhouse/merlin/memory_puzzle.h"

#include "funhouse/boltlib/boltlib.h"

namespace Funhouse {

struct BltMemoryPuzzleItem {
	static const uint32 kType = kBltMemoryPuzzleItemList;
	static const uint kSize = 0x10;
	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		numFrames = src.getUint16BEAt(0);
		framesId = BltId(src.getUint32BEAt(2)); // Ex: 8642
		paletteId = BltId(src.getUint32BEAt(6)); // Ex: 861D
		colorCyclesId = BltId(src.getUint32BEAt(0xA));
		soundId = BltShortId(src.getUint16BEAt(0xE)); // Ex: 860C
	}

	uint16 numFrames;
	BltId framesId;
	BltId paletteId;
	BltId colorCyclesId;
	BltId soundId;
};

typedef Common::Array<BltMemoryPuzzleItem> BltMemoryPuzzleItemList;

struct BltMemoryPuzzleItemFrame {
	static const uint32 kType = kBltMemoryPuzzleItemFrameList;
	static const uint kSize = 0xA;
	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		// FIXME: position at 0?
		pos.x = src.getInt16BEAt(0x0);
		pos.y = src.getInt16BEAt(0x2);
		imageId = BltId(src.getUint32BEAt(0x4)); // 8640
		delayFrames = src.getInt16BEAt(0x8);
	}

	Common::Point pos;
	BltId imageId;
	int16 delayFrames;
};

typedef Common::Array<BltMemoryPuzzleItemFrame> BltMemoryPuzzleItemFrameList;

static const int32 kMillisPerFrame = 1000 / 60;

void MemoryPuzzle::init(MerlinGame *game, Boltlib &boltlib, int challengeIdx) {
	_game = game;

	uint16 resId = 0;
	switch (challengeIdx) {
	case 3: resId = 0x865E; break;
	case 11: resId = 0x8797; break;
	case 23: resId = 0x887B; break;
	default: assert(false); break;
	}

	_game->setPopup(MerlinGame::kPuzzlePopup);

	BltResourceList resourceList;
	loadBltResourceArray(resourceList, boltlib, BltShortId(resId));
	BltId infosId     = resourceList[0].value; // Ex: 8600
	BltId sceneId     = resourceList[1].value; // Ex: 8606
	BltId failSoundId = resourceList[2].value; // Ex: 8608
	BltId itemsId     = resourceList[3].value; // Ex: 865D

	BltMemoryPuzzleInfos infos;
	loadBltResourceArray(infos, boltlib, infosId);
	_puzzleInfo = infos[_game->getDifficulty(kMemoryDifficulty)];

	loadScene(_scene, _game->getEngine(), boltlib, sceneId);

	BltMemoryPuzzleItemList itemList;
	loadBltResourceArray(itemList, boltlib, itemsId);

	_itemList.resize(itemList.size());
	for (uint i = 0; i < itemList.size(); ++i) {
		BltMemoryPuzzleItemFrameList frames;
		loadBltResourceArray(frames, boltlib, itemList[i].framesId);

		_itemList[i].frames.resize(frames.size());
		for (uint j = 0; j < frames.size(); ++j) {
			ItemFrame& frame = _itemList[i].frames[j];
			frame.pos = frames[j].pos;
			frame.image.load(boltlib, frames[j].imageId);
			frame.delayFrames = frames[j].delayFrames;
		}

		_itemList[i].palette.load(boltlib, itemList[i].paletteId);
		if (itemList[i].colorCyclesId.isValid()) {
			_itemList[i].colorCycles.reset(new BltColorCycles);
			loadBltResource(*_itemList[i].colorCycles, boltlib, itemList[i].colorCyclesId);
		}

		_itemList[i].sound.load(boltlib, itemList[i].soundId);
	}

	_failSound.load(boltlib, failSoundId);

	_solution.resize(_puzzleInfo.solutionLength);
	makeShuffledSequence(_puzzleInfo.pieceCount, spanOf(_solution));
	resetGoal();
}

void MemoryPuzzle::enter() {
	_scene.enter();
	startPlayback();
}

BoltRsp MemoryPuzzle::handleMsg(const BoltMsg &msg) {
	_task.run(msg);
	return kDone;
}

void MemoryPuzzle::handleReset() {
	// TODO: generate new solution?
	resetGoal();
	startPlayback();
}

BoltRsp MemoryPuzzle::handleButtonClick(int num) {
	debug(3, "Clicked button %d", num);

	if (num >= 0 && num < _itemList.size()) {
		if (_solution[_matches] == num) {
			// Earn a new match
			++_matches;
			startAnimation(num, _itemList[num].sound, [=]() {
				enterIdle();
			});
		} else {
			// Mismatch
			// TODO: Implement reset after a certain number of fails (see _puzzleInfo.failsToReset)
			_matches = 0;
			startAnimation(num, _failSound.pickSound(), [=]() {
				_game->setTimeout(_task, _puzzleInfo.failTimeout, [=]() {
					startPlayback();
				});
			});
		}
	}

	return BoltRsp::kDone;
}

void MemoryPuzzle::resetGoal() {
	_matches = 0;
	_goal = _puzzleInfo.goalStep; // FIXME: should be goalStep - 1?
}

void MemoryPuzzle::startPlayback() {
	_playbackStep = 0;
	playbackNext();
}

void MemoryPuzzle::startAnimation(int itemNum, BltSound& sound, std::function<void()> then) {
	debug(3, "Starting animation for item %d", itemNum);

	_animItem = itemNum;
	_animFrame = 0;
	_animSubFrame = 0;
	_animSoundTime = sound.getNumSamples() / 22; // This approximation is used by the original engine.
	_animPlayTime = _animSoundTime;
	_animThen = then;
	if (_puzzleInfo.foo == 0x4d) {
		warning("Overriding animation time for foo 0x4d");
		// Special case for Vials puzzle
		_animPlayTime = 400;
	}
	else if (_animPlayTime < kMinAnimPlayTimeMs) {
		_animPlayTime = kMinAnimPlayTimeMs;
	}

	drawItemFrame(_animItem, _animFrame);

	Item &item = _itemList[_animItem];
	//applyPalette(_graphics, kFore, item.palette);
	// XXX: applyPalette doesn't work correctly. Manually apply palette.
	_game->getGraphics()->setPlanePalette(kFore, &item.palette.data[BltPalette::kHeaderSize],
		0, 128);
	if (item.colorCycles) {
		applyColorCycles(_game->getGraphics(), kFore, item.colorCycles.get());
	} else {
		_game->getGraphics()->resetColorCycles();
	}
	
	sound.play(_game->getEngine()->_mixer);

	enterAnimPlaying();
}

void MemoryPuzzle::playbackNext() {
	if (_playbackStep < _goal) {
		startAnimation(_solution[_playbackStep], _itemList[_solution[_playbackStep]].sound, [=]() {
			playbackNext();
		});
		++_playbackStep;
	}
	else {
		enterIdle();
	}
}

void MemoryPuzzle::enterIdle() {
	_task.setNext([=](const BoltMsg &msg) { return runIdle(msg); });

	if (_matches >= _solution.size()) {
		_game->branchWin();
	}
	else if (_matches >= _goal) {
		_goal += _puzzleInfo.goalStep;
		_matches = 0;
		_game->setTimeout(_task, _puzzleInfo.goalAchieveTimeout, [=]() {
			startPlayback();
		});
	}
}

BoltRsp MemoryPuzzle::runIdle(const BoltMsg &msg) {
	BoltRsp cmd;

	if ((cmd = _game->handlePopup(msg)) != BoltRsp::kPass) {
		return cmd;
	}

	switch (msg.type) {
	case Scene::kClickButton:
		return handleButtonClick(msg.num);
	}

	return _scene.handleMsg(msg);
}

void MemoryPuzzle::enterAnimPlaying() {
	// On starting an animation, delay for one frame to match the original engine.
	_game->setTimeout(_task, 1 * kMillisPerFrame, [=]() {
		_game->getEngine()->startTimer(_frameTimer, kFrameDelayMs);
		_game->getEngine()->startTimer(_animTimer, _animSoundTime);

		_task.setNext([=](const BoltMsg& msg) {
			return animPlaying(msg);
		});
	});
}

BoltRsp MemoryPuzzle::animPlaying(const BoltMsg &msg) {
	_game->getEngine()->runTimer(msg, _frameTimer);
	_game->getEngine()->runTimer(msg, _animTimer);

	debug(5, "frametimer ticks %d elapse %d", _frameTimer.ticks, _frameTimer.elapse);
	if (_game->getEngine()->queryTimer(msg, _frameTimer))
	{
		const Item& item = _itemList[_animItem];
		const ItemFrame& frame = item.frames[_animFrame];

		_frameTimer.ticks -= kFrameDelayMs;

		if (_animTimer.ticks >= _animPlayTime) {
			if (frame.delayFrames == -1) {
				_animFrame++;
				_animSubFrame = 0;
				drawItemFrame(_animItem, _animFrame);
				enterAnimWindingDown();
				debug("winding down animation...");
			}
			else {
				enterAnimStopping();
			}
		}
		else {
			if (frame.delayFrames == -1) {
				// Do not advance frames
			}
			else {
				++_animSubFrame;
				if (_animSubFrame >= frame.delayFrames) {
					++_animFrame;
					if (_animFrame >= item.frames.size()) {
						_animFrame = 0;
					}
					_animSubFrame = 0;
					drawItemFrame(_animItem, _animFrame);
				}
			}
		}
	}

	return kDone;
}

void MemoryPuzzle::enterAnimWindingDown() {
	_task.setNext([=](const BoltMsg &msg) { return animWindingDown(msg); });
}

BoltRsp MemoryPuzzle::animWindingDown(const BoltMsg &msg) {
	_game->getEngine()->runTimer(msg, _frameTimer);
	_game->getEngine()->runTimer(msg, _animTimer);

	if (_game->getEngine()->queryTimer(msg, _frameTimer))
	{
		_frameTimer.ticks -= kFrameDelayMs;

		const Item& item = _itemList[_animItem];

		if (_animFrame >= item.frames.size()) {
			enterAnimStopping();
			return kDone;
		}

		const ItemFrame& frame = item.frames[_animFrame];

		if (frame.delayFrames != -1) {
			++_animSubFrame;
			if (_animSubFrame >= frame.delayFrames) {
				++_animFrame;
				_animSubFrame = 0;
				drawItemFrame(_animItem, _animFrame);
			}
		}
		else {
			_animFrame++;
			_animSubFrame = 0;
			drawItemFrame(_animItem, _animFrame);
		}
	}

	return kDone;
}

void MemoryPuzzle::enterAnimStopping() {
	_task.setNext([=](const BoltMsg &msg) { return animStopping(msg); });
}

BoltRsp MemoryPuzzle::animStopping(const BoltMsg &msg) {
	_game->getEngine()->runTimer(msg, _animTimer);
	if (_game->getEngine()->queryTimer(msg, _animTimer))
	{
		drawItemFrame(_animItem, -1);
		_animThen();
	}

	return kDone;
}

void MemoryPuzzle::drawItemFrame(int itemNum, int frameNum) {
	_game->getGraphics()->clearPlane(kFore);

	const Item &item = _itemList[itemNum];
	if (frameNum >= 0 && frameNum < item.frames.size()) {
		const ItemFrame &frame = item.frames[frameNum];
		const Common::Point &origin = _scene.getOrigin();
		frame.image.drawAt(_game->getGraphics()->getPlaneSurface(kFore), frame.pos.x - origin.x, frame.pos.y - origin.y, true);
	}

	_game->getGraphics()->markDirty();
}

} // End of namespace Funhouse
