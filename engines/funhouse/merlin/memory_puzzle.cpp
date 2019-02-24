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
	void load(const ConstSizedDataView<kSize> src, Boltlib &boltlib) {
		numFrames = src.readUint16BE(0);
		framesId = BltId(src.readUint32BE(2)); // Ex: 8642
		paletteId = BltId(src.readUint32BE(6)); // Ex: 861D
		colorCyclesId = BltId(src.readUint32BE(0xA));
		soundId = BltShortId(src.readUint16BE(0xE)); // Ex: 860C
	}

	uint16 numFrames;
	BltId framesId;
	BltId paletteId;
	BltId colorCyclesId;
	BltId soundId;
};

typedef ScopedArray<BltMemoryPuzzleItem> BltMemoryPuzzleItemList;

struct BltMemoryPuzzleItemFrame {
	static const uint32 kType = kBltMemoryPuzzleItemFrameList;
	static const uint kSize = 0xA;
	void load(const ConstSizedDataView<kSize> src, Boltlib &boltlib) {
		// FIXME: position at 0?
		pos.x = src.readInt16BE(0);
		pos.y = src.readInt16BE(2);
		imageId = BltId(src.readUint32BE(4)); // 8640
        type = src.readInt16BE(8);
	}

	Common::Point pos;
	BltId imageId;
    int16 type;
};

typedef ScopedArray<BltMemoryPuzzleItemFrame> BltMemoryPuzzleItemFrameList;

void MemoryPuzzle::init(Graphics *graphics, IBoltEventLoop *eventLoop, Boltlib &boltlib, BltId resId) {
	_graphics = graphics;
    _eventLoop = eventLoop;
    _animating = false;

	BltResourceList resourceList;
	loadBltResourceArray(resourceList, boltlib, resId);
	BltId sceneId = resourceList[1].value; // Ex: 8606
    BltId itemsId = resourceList[3].value; // Ex: 865D

	_scene.load(eventLoop, graphics, boltlib, sceneId);

	BltMemoryPuzzleItemList itemList;
	loadBltResourceArray(itemList, boltlib, itemsId);

	_itemList.alloc(itemList.size());
	for (uint i = 0; i < itemList.size(); ++i) {
		BltMemoryPuzzleItemFrameList frames;
		loadBltResourceArray(frames, boltlib, itemList[i].framesId);

		_itemList[i].frames.alloc(frames.size());
		for (uint j = 0; j < frames.size(); ++j) {
            ItemFrame& frame = _itemList[i].frames[j];
            frame.pos = frames[j].pos;
			frame.image.load(boltlib, frames[j].imageId);
            frame.type = (FrameType)frames[j].type;
		}

		_itemList[i].palette.load(boltlib, itemList[i].paletteId);
		if (itemList[i].colorCyclesId.isValid()) {
			_itemList[i].colorCycles.reset(new BltColorCycles);
			loadBltResource(*_itemList[i].colorCycles, boltlib, itemList[i].colorCyclesId);
		}
	}
}

void MemoryPuzzle::enter() {
	_scene.enter();
}

BoltCmd MemoryPuzzle::handleMsg(const BoltMsg &msg) {
    // TODO: Golly I hate this code.
    if (_animating) {
        if (!_animIsOrderedToEnd) {
            // Phase 1 - Advance frames, loop if end is reached until a maximum time has elapsed
            uint32 animTime = _eventLoop->getEventTime() - _animStartTime;
            if (animTime >= kSelectionDelay) {
                _animating = false;
                enter(); // Redraw
                return BoltCmd::kResend;
            } else {
                uint32 frameDelta = _eventLoop->getEventTime() - _frameTime;
                if (frameDelta < _frameDelay) {
                    return BoltCmd::kDone;
                } else { // Next frame
                    _frameTime += _frameDelay;
                    ++_frameNum;
                    if (_frameNum >= _itemList[_itemToAnimate].frames.size()) {
                        _frameNum = 0;
                    }

                    drawItemFrame(_itemToAnimate, _frameNum);

                    FrameType frameType = _itemList[_itemToAnimate].frames[_frameNum].type;
                    switch (frameType) {
                    case kProceed:
                        _frameDelay = kAnimPeriod;
                        break;
                    case kWaitForEnd:
                        _animIsOrderedToEnd = true;
                        _frameDelay = kAnimEndingDelay;
                        break;
                    default:
                        warning("Unknown frame type %d\n", (int)frameType);
                        break;
                    }

                    return BoltCmd::kResend;
                }
            }
        } else {
            // Phase 2 - Animation has been ordered to come to an end
            uint32 frameDelta = _eventLoop->getEventTime() - _frameTime;
            if (frameDelta < _frameDelay) {
                return BoltCmd::kDone;
            } else { // Next frame
                _frameTime += _frameDelay;
                _frameDelay = kAnimPeriod;
                ++_frameNum;
                if (_frameNum >= _itemList[_itemToAnimate].frames.size()) {
                    _animating = false;
                    enter(); // Redraw
                    return BoltCmd::kResend;
                } else {
                    drawItemFrame(_itemToAnimate, _frameNum);
                    return BoltCmd::kResend;
                }
            }
        }
    }

    // XXX: right-click to win instantly. TODO: remove.
    if (msg.type == BoltMsg::kRightClick) {
        return kWin;
    }

    if (msg.type == Scene::kClickButton) {
        return handleButtonClick(msg.num);
    }

    return _scene.handleMsg(msg);
}

BoltCmd MemoryPuzzle::handleButtonClick(int num) {
	debug(3, "Clicked button %d", num);
	// TODO: implement puzzle

	if (num >= 0 && num < _itemList.size()) {
        _animating = true;
        _itemToAnimate = num;
        _frameNum = 0;
        _animStartTime = _eventLoop->getEventTime();
        _frameTime = _eventLoop->getEventTime();
        _animIsOrderedToEnd = false;
        _frameDelay = kAnimPeriod;

        const Item &item = _itemList[_itemToAnimate];
        //applyPalette(_graphics, kFore, item.palette);
        // XXX: applyPalette doesn't work correctly. Manually apply palette.
        _graphics->setPlanePalette(kFore, &item.palette.data[BltPalette::kHeaderSize],
            0, 128);
        if (item.colorCycles) {
            // FIXME: color cycles are broken.
            applyColorCycles(_graphics, kFore, item.colorCycles.get());
        }
        else {
            _graphics->resetColorCycles();
        }

        drawItemFrame(_itemToAnimate, _frameNum);
	}

	return BoltCmd::kDone;
}

void MemoryPuzzle::drawItemFrame(int itemNum, int frameNum) {
    // Draw frame corresponding to item
    // TODO: implement animation
    const Item &item = _itemList[itemNum];
    const ItemFrame &frame = item.frames[frameNum];
    const Common::Point &origin = _scene.getOrigin();
    _graphics->clearPlane(kFore);
    frame.image.drawAt(_graphics->getPlaneSurface(kFore), frame.pos.x - origin.x, frame.pos.y - origin.y, true);
    _graphics->markDirty();
}

} // End of namespace Funhouse
