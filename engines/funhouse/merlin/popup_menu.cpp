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
	static const uint32 kSize = 0x8;
	void load(Common::Span<const byte> src, Boltlib &bltFile) {
		rect = Rect(src);
	}

	Rect rect;
};

struct BltPopup {
	static const uint32 kType = kBltPopup;
	static const uint32 kSize = 0x12;
	void load(Common::Span<const byte> src, Boltlib &bltFile) {
		numButtons = src.getUint16BEAt(0x0);
		bgImageId = BltId(src.getUint32BEAt(0x2));
		paletteId = BltId(src.getUint32BEAt(0x6));
		hotspotListId = BltId(src.getUint32BEAt(0xa));
		spriteListId = BltId(src.getUint32BEAt(0xe));
	}

	uint16 numButtons;
	BltId bgImageId;
	BltId paletteId;
	BltId hotspotListId;
	BltId spriteListId;
};

void PopupMenu::init(MerlinGame *game, Boltlib &boltlib, BltId id) {
	_game = game;
	_active = false;

	BltPopup popup;
	loadBltResource(popup, boltlib, id);
	_buttons.resize(popup.numButtons);
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
		_buttons[i].hovered = (*loadBltSprites(boltlib, spriteList[i * 2].value))[0];
		_buttons[i].unhovered = (*loadBltSprites(boltlib, spriteList[i * 2 + 1].value))[0];
	}
}

void PopupMenu::dismiss() {
	_active = false;
	_game->redraw();
}

void PopupMenu::setButtonEnable(int idx, bool enable) {
	_buttons[idx].enable = enable;
}

BoltRsp PopupMenu::react(const BoltMsg &msg) {
	if (msg.type == BoltMsg::kRightClick) {
		if (!_active) {
			activate();
		} else {
			dismiss();
		}

		return BoltRsp::kDone;
	}

	if (!_active) {
		return BoltRsp::kPass;
	}

	if (msg.type == BoltMsg::kClick || msg.type == BoltMsg::kHover) {
		int num = getButtonAt(msg.point);
		for (int i = 0; i < _buttons.size(); ++i) {
			SharedSprite sprite = (i == num) ? _buttons[i].hovered : _buttons[i].unhovered;
			sprite->image->drawAt(_game->getGraphics()->getPlaneSurface(kFore), sprite->pos.x, sprite->pos.y, true);
		}
		if (num != -1 && _buttons[num].enable) {
			if (msg.type == BoltMsg::kClick) {
				return handleButtonClick(num);
			}
		}
	}

	return BoltRsp::kDone;
}

BoltRsp PopupMenu::handleButtonClick(int num) {
	_game->handlePopupButtonClick(num);
	return BoltRsp::kDone;
}

bool PopupMenu::isActive() const {
	return _active;
}

void PopupMenu::activate() {
	_active = true;

	// The original engine does something hacky here: Only colors 121-127 are applied.
	static const int kFirstPopupColor = 121;
	static const int kNumPopupColors = 7;
	_game->getGraphics()->setPlanePalette(kFore, &_palette.data[6 + kFirstPopupColor * 3], kFirstPopupColor, kNumPopupColors);

	static const int kPopupX = -32;
	static const int kPopupY = 168;
	_bgImage.drawAt(_game->getGraphics()->getPlaneSurface(kFore), kPopupX, kPopupY, true);

	_game->getGraphics()->markDirty();

	_game->getEngine()->requestHover();
}

int PopupMenu::getButtonAt(const Common::Point &pt) const {
	for (int i = 0; i < _buttons.size(); ++i) {
		if (_buttons[i].enable && _buttons[i].hotspot.contains(pt)) {
			return i;
		}
	}

	return -1;
}

} // End of namespace Funhouse
