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

#include "funhouse/merlin/submenu.h"

#include "funhouse/boltlib/boltlib.h"
#include "funhouse/graphics.h"

namespace Funhouse {
    
struct BltRect {
	static const uint32 kType = kBltRect;
	static const uint32 kSize = 8;
	void load(const ConstSizedDataView<kSize> src, Boltlib &bltFile) {
        rect = Rect(src);
	}

    Rect rect;
    uint16 numButtons;
    BltId bgImageId;
    BltId paletteId;
    BltId hotspotListId;
    BltId spriteListId;
};

struct BltSubmenu {
	static const uint32 kType = kBltSubmenu;
	static const uint32 kSize = 0x12;
	void load(const ConstSizedDataView<kSize> src, Boltlib &bltFile) {
        numButtons = src.readUint16BE(0);
        bgImageId = BltId(src.readUint32BE(2));
        paletteId = BltId(src.readUint32BE(6));
        hotspotListId = BltId(src.readUint32BE(0xA));
        spriteListId = BltId(src.readUint32BE(0xE));
	}

    uint16 numButtons;
    BltId bgImageId;
    BltId paletteId;
    BltId hotspotListId;
    BltId spriteListId;
};

void Submenu::init(Graphics *graphics, Boltlib &boltlib, BltId id) {
    _graphics = graphics;

    _enabled = false;

    BltSubmenu submenu;
    loadBltResource(submenu, boltlib, id);
    _buttons.alloc(submenu.numButtons);
    _bgImage.load(boltlib, submenu.bgImageId);
    _palette.load(boltlib, submenu.paletteId);

    BltResourceList hotspotList;
    loadBltResourceArray(hotspotList, boltlib, submenu.hotspotListId);

    BltResourceList spriteList;
    loadBltResourceArray(spriteList, boltlib, submenu.spriteListId);

    for (int i = 0; i < submenu.numButtons; ++i) {
        BltRect hotspotRect;
        loadBltResource(hotspotRect, boltlib, hotspotList[i].value);

        _buttons[i].hotspot = hotspotRect.rect;
        _buttons[i].hovered.load(boltlib, spriteList[i * 2].value);
        _buttons[i].unhovered.load(boltlib, spriteList[i * 2 + 1].value);
    }
}

void Submenu::enable(bool en) {
    _enabled = en;
    if (_enabled) {
        // TODO: The palette is special. Only certain colors are applied.
        applyPalette(_graphics, kFore, _palette);
        _bgImage.drawAt(_graphics->getPlaneSurface(kFore), 0, 0, true); // TODO: fix position
        _graphics->markDirty();
    }
}

BoltCmd Submenu::handleMsg(const BoltMsg &msg) {
    if (!_enabled) {
        return BoltCmd::kDone;
    }

    if (msg.type == BoltMsg::kClick || msg.type == BoltMsg::kHover) {
        int num = getButtonAt(msg.point);
        if (num != -1) {
            for (int i = 0; i < _buttons.size(); ++i) {
                const Sprite &sp = (i == num) ? _buttons[i].hovered.getSprite(0) : _buttons[i].unhovered.getSprite(0);
                sp.image->drawAt(_graphics->getPlaneSurface(kFore), sp.pos.x, sp.pos.y, true);
            }

            if (msg.type == BoltMsg::kClick) {
                warning("Submenu button %d not implemented", num);
            }
        }
    }

    return BoltCmd::kDone;
}

int Submenu::getButtonAt(const Common::Point &pt) const {
    for (int i = 0; i < _buttons.size(); ++i) {
        if (_buttons[i].hotspot.contains(pt)) {
            return i;
        }
    }

    return -1;
}

} // End of namespace Funhouse
