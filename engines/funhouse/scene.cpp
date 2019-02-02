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

#include "funhouse/scene.h"

#include "funhouse/bolt.h"
#include "funhouse/boltlib/palette.h"

namespace Funhouse {

struct BltScene { // type 32
	static const uint32 kType = kBltScene;
	static const uint32 kSize = 0x24;
	void load(const ConstSizedDataView<kSize> src, Boltlib &boltlib) {
		forePlaneId = BltId(src.readUint32BE(0));
		backPlaneId = BltId(src.readUint32BE(4));
		numSprites = src.readUint8(0x8);
		spritesId = BltId(src.readUint32BE(0xA));
		// FIXME: unknown fields at 0xD..0x16
		colorCyclesId = BltId(src.readUint32BE(0x16));
		numButtons = src.readUint16BE(0x1A);
		buttonsId = BltId(src.readUint32BE(0x1C));
		origin.x = src.readInt16BE(0x20);
		origin.y = src.readInt16BE(0x22);
	}

	BltId forePlaneId;
	BltId backPlaneId;
	uint8 numSprites;
	BltId spritesId;
	BltId colorCyclesId;
	uint16 numButtons;
	BltId buttonsId;
	Common::Point origin;
};

struct BltPlane { // type 26
    static const uint32 kType = kBltPlane;
    static const uint kSize = 0x10;
    void load(const ConstSizedDataView<kSize> src, Boltlib &bltFile) {
        imageId = BltId(src.readUint32BE(0));
        paletteId = BltId(src.readUint32BE(4));
        hotspotsId = BltId(src.readUint32BE(8));
    }

    BltId imageId;
    BltId paletteId;
    BltId hotspotsId;
};

struct BltButtonGraphicElement { // type 30
	static const uint32 kType = kBltButtonGraphicsList;
	static const uint kSize = 0xE;
	enum GraphicsType {
		PaletteMods = 1,
		Sprites = 2
	};

	void load(const ConstSizedDataView<kSize> src, Boltlib &boltlib) {
		type = src.readUint16BE(0);
		// FIXME: unknown field at 2. It points to an image in sliding puzzles.
		hoveredId = BltId(src.readUint32BE(6));
		idleId = BltId(src.readUint32BE(0xA));
	}

	uint16 type;
	BltId hoveredId;
	BltId idleId;
};

typedef ScopedArray<BltButtonGraphicElement> BltButtonGraphicsList;

struct BltButtonElement { // type 31
	static const uint32 kType = kBltButtonList;
	static const uint kSize = 0x14;
	void load(const ConstSizedDataView<kSize> src, Boltlib &boltlib) {
		type = src.readUint16BE(0);
		rect = Rect(src.slice(2));
		plane = src.readUint16BE(0xA);
		numGraphics = src.readUint16BE(0xC);
		// FIXME: unknown field at 0xE. Always 0 in game data.
		graphicsId = BltId(src.readUint32BE(0x10));
	}

	enum HotspotType {
		Rectangle = 1,
		// 2 is regular display query (unused)
		HotspotQuery = 3
	};

	uint16 type;
	Rect rect;
	uint16 plane;
	uint16 numGraphics;
	BltId graphicsId;
};

typedef ScopedArray<BltButtonElement> BltButtonList;

void Scene::load(IBoltEventLoop *eventLoop, Graphics *graphics, Boltlib &boltlib, BltId sceneId)
{
	_eventLoop = eventLoop;
	_graphics = graphics;

	BltScene sceneInfo;
	loadBltResource(sceneInfo, boltlib, sceneId);

	_origin = sceneInfo.origin;

    BltPlane bltForePlane;
	loadBltResource(bltForePlane, boltlib, sceneInfo.forePlaneId);
    _forePlane.image.load(boltlib, bltForePlane.imageId);
    _forePlane.palette.load(boltlib, bltForePlane.paletteId);
    _forePlane.hotspots.load(boltlib, bltForePlane.hotspotsId);

    setBackPlane(boltlib, sceneInfo.backPlaneId);

    _sprites.load(boltlib, sceneInfo.spritesId);

	BltButtonList buttons;
	loadBltResourceArray(buttons, boltlib, sceneInfo.buttonsId);
	_buttons.alloc(buttons.size());
	for (uint i = 0; i < buttons.size(); ++i) {
		_buttons[i].hotspotType = static_cast<HotspotType>(buttons[i].type);
		_buttons[i].plane = buttons[i].plane;
		_buttons[i].hotspot = buttons[i].rect;

		BltButtonGraphicsList buttonGraphics;
		loadBltResourceArray(buttonGraphics, boltlib, buttons[i].graphicsId);

		_buttons[i].graphics.alloc(buttonGraphics.size());
		for (uint j = 0; j < buttonGraphics.size(); ++j) {
			_buttons[i].graphics[j].graphicsType = static_cast<GraphicsType>(buttonGraphics[j].type);
			if (buttonGraphics[j].type == kPaletteMods) {
				loadBltResourceArray(_buttons[i].graphics[j].hoveredPaletteMods, boltlib, buttonGraphics[j].hoveredId);
				loadBltResourceArray(_buttons[i].graphics[j].idlePaletteMods, boltlib, buttonGraphics[j].idleId);
			}
			else if (buttonGraphics[j].type == kSprites) {
                _buttons[i].graphics[j].hoveredSprites.load(boltlib, buttonGraphics[j].hoveredId);
                _buttons[i].graphics[j].idleSprites.load(boltlib, buttonGraphics[j].idleId);
			}
		}
	}

	_colorCycles.reset();
	if (sceneInfo.colorCyclesId.isValid()) {
		_colorCycles.reset(new BltColorCycles);
		loadBltResource(*_colorCycles, boltlib, sceneInfo.colorCyclesId);
	}
}

void Scene::enter() {
	applyPalette(_graphics, kBack, _backPlane.palette);
	if (_backPlane.image) {
		_backPlane.image.drawAt(_graphics->getPlaneSurface(kBack), 0, 0, false);
	}
	else {
		_graphics->clearPlane(kBack);
	}

	applyPalette(_graphics, kFore, _forePlane.palette);
	if (_forePlane.image) {
		_forePlane.image.drawAt(_graphics->getPlaneSurface(kFore), 0, 0, false);
	}
	else {
		_graphics->clearPlane(kFore);
	}

	applyColorCycles(_graphics, kBack, _colorCycles.get());

    redrawSprites();

	_graphics->markDirty();
}

void Scene::redrawSprites() {
    // TODO: don't redraw fore plane when executing the enter method.
    if (_forePlane.image) {
        _forePlane.image.drawAt(_graphics->getPlaneSurface(kFore), 0, 0, false);
    }
    else {
        _graphics->clearPlane(kFore);
    }

    // Draw sprites
    for (size_t i = 0; i < _sprites.getNumSprites(); ++i) {
        Common::Point pos = _sprites.getSprite(i).pos - _origin;
        // FIXME: Are sprites drawn to back or fore plane? Is it somehow selectable?
        _sprites.getSprite(i).image->drawAt(_graphics->getPlaneSurface(kFore), pos.x, pos.y, true);
    }

    _graphics->markDirty();
}

BoltCmd Scene::handleMsg(const BoltMsg &msg) {
	switch (msg.type) {
	case BoltMsg::kHover: {
		int hoveredButton = getButtonAtPoint(msg.point);
		for (uint i = 0; i < _buttons.size(); ++i) {
			drawButton(_buttons[i], (int)i == hoveredButton);
		}
		_graphics->markDirty();
		break;
	}

	case BoltMsg::kClick: {
		BoltMsg newMsg(kClickButton);
		newMsg.num = getButtonAtPoint(msg.point);
		_eventLoop->setMsg(newMsg);
		return BoltCmd::kResend;
	}
	}

	return BoltCmd::kDone;
}

void Scene::setBackPlane(Boltlib &boltlib, BltId id) {
    BltPlane bltBackPlane;
    loadBltResource(bltBackPlane, boltlib, id);
    _backPlane.image.load(boltlib, bltBackPlane.imageId);
    _backPlane.palette.load(boltlib, bltBackPlane.paletteId);
    _backPlane.hotspots.load(boltlib, bltBackPlane.hotspotsId);
}

int Scene::getButtonAtPoint(const Common::Point &pt) {
	byte foreHotspotColor = 0;
	if (_forePlane.hotspots) {
		foreHotspotColor = _forePlane.hotspots.query(pt.x, pt.y);
	}

	byte backHotspotColor = 0;
	if (_backPlane.hotspots) {
		backHotspotColor = _backPlane.hotspots.query(pt.x, pt.y);
	}

	for (int i = 0; i < (int)_buttons.size(); ++i) {
		const Button &button = _buttons[i];
		if (button.hotspotType == kRect) {
			if (button.hotspot.contains(_origin + pt)) {
				return i;
			}
		}
		else if (button.hotspotType == kHotspotQuery) {
			byte color = button.plane ? backHotspotColor : foreHotspotColor;
			if (color >= button.hotspot.left && color <= button.hotspot.right) {
				return i;
			}
		}
	}

	return -1;
}

void Scene::drawButton(const Button &button, bool hovered) {
	// TODO: support states other than 0
	if (button.graphics) {
		if (button.graphics[0].graphicsType == kPaletteMods) {
			const BltPaletteMods &paletteMod = hovered ? button.graphics[0].hoveredPaletteMods : button.graphics[0].idlePaletteMods;
			applyPaletteMod(_graphics, button.plane, paletteMod, 0);
		}
		else if (button.graphics[0].graphicsType == kSprites) {
			const BltSprites &spriteList = hovered ? button.graphics[0].hoveredSprites : button.graphics[0].idleSprites;
			if (spriteList.getNumSprites() > 0) {
				const Sprite &sprite = spriteList.getSprite(0);
				Common::Point pos = sprite.pos - _origin;
				if (sprite.image) {
					sprite.image->drawAt(_graphics->getPlaneSurface(button.plane), pos.x, pos.y, true);
				}
			}
		}
	}
}

} // End of namespace Funhouse
