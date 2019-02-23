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
    _state = kIdle;

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
    switch (_state) {
    case kSelecting: {
        uint32 progress = _eventLoop->getEventTime() - _selectionTime;
        if (progress < kSelectionDelay) {
            uint32 animProgress = _eventLoop->getEventTime() - _animFrameTime;
            if (animProgress < _animDelay) {
                return BoltCmd::kDone;
            } else { // Next frame
                if (!_animEnding) {
                    ++_animFrameNum;
                    if (_animFrameNum >= _itemList[_selectedItem].frames.size()) {
                        _animFrameNum = 0;
                    }
                    drawItemFrame(_selectedItem, _animFrameNum);
                } else {
                    if (_animFrameNum < _itemList[_selectedItem].frames.size() - 1) {
                        ++_animFrameNum;
                        drawItemFrame(_selectedItem, _animFrameNum);
                    }
                }
                _animFrameTime += _animDelay;

                FrameType frameType = _itemList[_selectedItem].frames[_animFrameNum].type;
                switch (frameType) {
                case kProceed:
                    _animDelay = kAnimPeriod;
                    break;
                case kWaitForEnd:
                    _animEnding = true; // TODO: insert delay
                    _animDelay = kAnimEndingDelay;
                    break;
                default:
                    warning("Unknown frame type %d\n", (int)frameType);
                    break;
                }

                return BoltCmd::kResend;
            }
        } else { // Done selecting
            _state = kIdle;
            // Redraw
            enter();
            return BoltCmd::kResend;
        }
        break;
    }

    case kIdle: {
        // XXX: right-click to win instantly. TODO: remove.
        if (msg.type == BoltMsg::kRightClick) {
            return kWin;
        }

        if (msg.type == Scene::kClickButton) {
            return handleButtonClick(msg.num);
        }

        return _scene.handleMsg(msg);
    }

    default:
        assert(false && "Invalid state"); // Unreachable
        return BoltCmd::kDone;
    }
}

BoltCmd MemoryPuzzle::handleButtonClick(int num) {
	debug(3, "Clicked button %d", num);
	// TODO: implement puzzle

	if (num >= 0 && num < _itemList.size()) {
        _state = kSelecting;
        _selectionTime = _eventLoop->getEventTime();
        _animFrameTime = _selectionTime;
        _animDelay = kAnimPeriod;
        _selectedItem = num;
        _animFrameNum = 0;
        _animEnding = false;

        const Item &item = _itemList[_selectedItem];
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

        drawItemFrame(_selectedItem, _animFrameNum);
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
