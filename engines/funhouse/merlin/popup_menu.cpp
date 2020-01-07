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

#include "funhouse/merlin/popup_menu.h"

#include "funhouse/boltlib/boltlib.h"
#include "funhouse/graphics.h"
#include "funhouse/merlin/merlin.h"

namespace Funhouse {
    
struct BltRect {
	static const uint32 kType = kBltRect;
	static const uint32 kSize = 8;
	void load(Common::Span<const byte> src, Boltlib &bltFile) {
        rect = Rect(src);
	}

    Rect rect;
};

struct BltPopup {
	static const uint32 kType = kBltPopup;
	static const uint32 kSize = 0x12;
	void load(Common::Span<const byte> src, Boltlib &bltFile) {
        numButtons = src.getUint16BEAt(0);
        bgImageId = BltId(src.getUint32BEAt(2));
        paletteId = BltId(src.getUint32BEAt(6));
        hotspotListId = BltId(src.getUint32BEAt(0xA));
        spriteListId = BltId(src.getUint32BEAt(0xE));
	}

    uint16 numButtons;
    BltId bgImageId;
    BltId paletteId;
    BltId hotspotListId;
    BltId spriteListId;
};

void PopupMenu::init(MerlinGame *game, Boltlib &boltlib, BltId id) {
    _game = game;
    _eventLoop = _game->getEventLoop();
    _graphics = _game->getGraphics();
    _active = false;

    BltPopup popup;
    loadBltResource(popup, boltlib, id);
    _buttons.alloc(popup.numButtons);
    _bgImage.load(boltlib, popup.bgImageId);
    _palette.load(boltlib, popup.paletteId);

    BltResourceList hotspotList;
    loadBltResourceArray(hotspotList, boltlib, popup.hotspotListId);

    BltResourceList spriteList;
    loadBltResourceArray(spriteList, boltlib, popup.spriteListId);

    for (int i = 0; i < popup.numButtons; ++i) {
        BltRect hotspotRect;
        loadBltResource(hotspotRect, boltlib, hotspotList[i].value);

        _buttons[i].hotspot = hotspotRect.rect;
        _buttons[i].hovered.load(boltlib, spriteList[i * 2].value);
        _buttons[i].unhovered.load(boltlib, spriteList[i * 2 + 1].value);
    }
}

void PopupMenu::dismiss() {
	_active = false;
}

BoltCmd PopupMenu::handleMsg(const BoltMsg &msg) {
    if (msg.type == BoltMsg::kRightClick) {
        if (!_active) {
            activate();
        } else {
            _active = false;
            _game->redraw();
        }

        return BoltCmd::kDone;
    }

	if (msg.type == BoltMsg::kPopupButtonClick) {
		return BoltCmd::kPass;
	}

    if (!_active) {
        return BoltCmd::kPass;
    }

    if (msg.type == BoltMsg::kClick || msg.type == BoltMsg::kHover) {
        int num = getButtonAt(msg.point);
        if (num != -1) {
            for (int i = 0; i < _buttons.size(); ++i) {
				const BltSprites &sprites = (i == num) ? _buttons[i].hovered : _buttons[i].unhovered;
				const Common::Point &spritePos = sprites.getSpritePosition(0);
				const BltImage *spriteImage = sprites.getSpriteImage(0);
                spriteImage->drawAt(_graphics->getPlaneSurface(kBack), spritePos.x, spritePos.y, true);
            }

            if (msg.type == BoltMsg::kClick) {
                return handleButtonClick(num);
            }
        }
    }

    return BoltCmd::kDone;
}

BoltCmd PopupMenu::handleButtonClick(int num) {
	BoltMsg msg(BoltMsg::kPopupButtonClick);
	msg.num = num;
	_game->getEngine()->setMsg(msg);
    return BoltCmd::kResend;
}

void PopupMenu::activate() {
    _active = true;

    // The original engine does something hacky here: Only colors 121-127 are applied.
    static const int kFirstPopupColor = 121;
    static const int kNumPopupColors = 7;
    _graphics->setPlanePalette(kBack, &_palette.data[6 + kFirstPopupColor * 3], kFirstPopupColor, kNumPopupColors);

    static const int kPopupX = -32;
    static const int kPopupY = 168;
    _bgImage.drawAt(_graphics->getPlaneSurface(kBack), kPopupX, kPopupY, true);

    _graphics->markDirty();
}

int PopupMenu::getButtonAt(const Common::Point &pt) const {
    for (int i = 0; i < _buttons.size(); ++i) {
        if (_buttons[i].hotspot.contains(pt)) {
            return i;
        }
    }

    return -1;
}

} // End of namespace Funhouse
