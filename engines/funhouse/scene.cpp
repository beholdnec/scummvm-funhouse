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

#include "common/events.h"

namespace Funhouse {

struct BltScene { // type 32
	static const uint32 kType = kBltScene;
	static const uint32 kSize = 0x24;
	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		forePlaneId = BltId(src.getUint32BEAt(0));
		backPlaneId = BltId(src.getUint32BEAt(4));
		numSprites = src.getUint8At(0x8);
		spritesId = BltId(src.getUint32BEAt(0xA));
		// FIXME: unknown fields at 0xD..0x16
		colorCyclesId = BltId(src.getUint32BEAt(0x16));
		numButtons = src.getUint16BEAt(0x1A);
		buttonsId = BltId(src.getUint32BEAt(0x1C));
		origin.x = src.getInt16BEAt(0x20);
		origin.y = src.getInt16BEAt(0x22);
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
    void load(Common::Span<const byte> src, Boltlib &bltFile) {
        imageId = BltId(src.getUint32BEAt(0));
        paletteId = BltId(src.getUint32BEAt(4));
        hotspotsId = BltId(src.getUint32BEAt(8));
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

	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		type = src.getUint16BEAt(0);
		// FIXME: unknown field at 2. It points to an image in sliding puzzles.
		hoveredId = BltId(src.getUint32BEAt(6));
		idleId = BltId(src.getUint32BEAt(0xA));
	}

	uint16 type;
	BltId hoveredId;
	BltId idleId;
};

typedef ScopedArray<BltButtonGraphicElement> BltButtonGraphicsList;

struct BltButtonElement { // type 31
	static const uint32 kType = kBltButtonList;
	static const uint kSize = 0x14;
	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		type = src.getUint16BEAt(0);
		rect = Rect(src.subspan(2));
		plane = src.getUint16BEAt(0xA);
		numGraphics = src.getUint16BEAt(0xC);
		// FIXME: unknown field at 0xE. Always 0 in game data.
		graphicsId = BltId(src.getUint32BEAt(0x10));
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

Scene::Scene() : _engine(nullptr), _graphics(nullptr)
{ }

void Scene::init(FunhouseEngine *engine, int numButtons, int numSprites)
{
	_engine = engine;
	_graphics = _engine->getGraphics();

	_buttons.alloc(numButtons);
}

void Scene::enter() {
	applyPalette(_graphics, kBack, _backPlane.palette);
	applyPalette(_graphics, kFore, _forePlane.palette);
	applyColorCycles(_graphics, kBack, _colorCycles.get());
	redraw();
}

void Scene::redraw() {
	if (_backPlane.image) {
		_backPlane.image.drawAt(_graphics->getPlaneSurface(kBack), 0, 0, false);
	} else {
		_graphics->clearPlane(kBack);
	}

	if (_forePlane.image) {
		_forePlane.image.drawAt(_graphics->getPlaneSurface(kFore), 0, 0, false);
	} else {
		_graphics->clearPlane(kFore);
	}

    for (int i = 0; i < _sprites.getSpriteCount(); ++i) {
		Common::Point position = _sprites.getSpritePosition(i) - _origin;
		// FIXME: Are sprites drawn to back or fore plane? Is it selectable?
		_sprites.getSpriteImage(i)->drawAt(_graphics->getPlaneSurface(kFore), position.x, position.y, true);
    }

    _graphics->markDirty();
}

BoltCmd Scene::handleMsg(const BoltMsg &msg) {
	switch (msg.type) {
	case BoltMsg::kHover: {
		int hoveredButton = getButtonAtPoint(msg.point);
		drawButtons(hoveredButton);
		break;
	}

	case BoltMsg::kClick: {
		BoltMsg newMsg(kClickButton);
		newMsg.num = getButtonAtPoint(msg.point);
		_engine->setMsg(newMsg);
		return BoltCmd::kResend;
	}
	}

	return BoltCmd::kDone;
}

void Scene::loadBackPlane(Boltlib &boltlib, BltId planeId) {
	loadPlane(_backPlane, boltlib, planeId);
}

void Scene::loadForePlane(Boltlib &boltlib, BltId planeId) {
	loadPlane(_forePlane, boltlib, planeId);
}

void Scene::loadColorCycles(Boltlib &boltlib, BltId id) {
	_colorCycles.reset();
	if (id.isValid()) {
		_colorCycles.reset(new BltColorCycles);
		loadBltResource(*_colorCycles, boltlib, id);
	}
}

void Scene::loadSprites(Boltlib &boltlib, BltId id) {
	_sprites.load(boltlib, id);
}

Common::Point Scene::getOrigin() const {
	return _origin;
}

void Scene::setOrigin(const Common::Point &origin) {
	_origin = origin;
}

void Scene::setSpriteImageNum(int num, int imageNum) {
	_sprites.setSpriteImageNum(num, imageNum);
}

Scene::Button::Button() : _enable(false), _userData(nullptr), _graphicsNum(0), _overrideGraphics(false)
{ }

void Scene::Button::setEnable(bool enable) {
	_enable = enable;
}

void* Scene::Button::getUserData() const {
	return _userData;
}

void Scene::Button::setUserData(void *userData) {
	_userData = userData;
}

void Scene::Button::setGraphics(int num) {
	assert(num >= 0 && num < _graphicsSet.size());
	_graphicsNum = num;
}

void Scene::Button::setHotspot(HotspotType type, Rect hotspot) {
	_hotspotType = type;
	_hotspot = hotspot;
}

void Scene::Button::setPlane(uint16 plane) {
	_plane = plane;
}

void Scene::Button::loadGraphicsSet(Boltlib &boltlib, BltId id) {
	BltButtonGraphicsList buttonGraphics;
	loadBltResourceArray(buttonGraphics, boltlib, id);

	_graphicsSet.alloc(buttonGraphics.size());
	for (uint j = 0; j < buttonGraphics.size(); ++j) {
		_graphicsSet[j].graphicsType = static_cast<GraphicsType>(buttonGraphics[j].type);
		if (buttonGraphics[j].type == kPaletteMods) {
			loadBltResourceArray(_graphicsSet[j].hoveredPaletteMods, boltlib, buttonGraphics[j].hoveredId);
			loadBltResourceArray(_graphicsSet[j].idlePaletteMods, boltlib, buttonGraphics[j].idleId);
		}
		else if (buttonGraphics[j].type == kSprites) {
			_graphicsSet[j].hoveredSprites.load(boltlib, buttonGraphics[j].hoveredId);
			_graphicsSet[j].idleSprites.load(boltlib, buttonGraphics[j].idleId);
		}
	}
}

void Scene::Button::overrideGraphics(Common::Point position, BltImage* hoveredImage, BltImage* idleImage) {
	_overrideGraphics = true;
	_overridePosition = position;
	_overrideHoveredImage = hoveredImage;
	_overrideIdleImage = idleImage;
}

Scene::Button& Scene::getButton(int num) {
	return _buttons[num];
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
		if (button._enable) {
			if (button._overrideGraphics) {
				// For buttons with overridden graphics, the hotspot is the image.
				if (button._overrideIdleImage->getRect(button._overridePosition).contains(_origin + pt)) {
					return i;
				}
			} else if (button._hotspotType == kRect) {
				if (button._hotspot.contains(_origin + pt)) {
					return i;
				}
			} else if (button._hotspotType == kHotspotQuery) {
				byte color = button._plane ? backHotspotColor : foreHotspotColor;
				if (color >= button._hotspot.left && color <= button._hotspot.right) {
					return i;
				}
			}
		}
	}

	return -1;
}

void Scene::drawButton(const Button &button, bool hovered) {
	if (!button._enable) {
		return;
	}

	if (button._overrideGraphics) {
		BltImage* image = hovered ? button._overrideHoveredImage : button._overrideIdleImage;
		Common::Point position = button._overridePosition - _origin;
		image->drawAt(_graphics->getPlaneSurface(button._plane), position.x, position.y, true);
	} else if (button._graphicsSet) {
		const ButtonGraphics& graphicsSet = button._graphicsSet[button._graphicsNum];
		if (graphicsSet.graphicsType == kPaletteMods) {
			const BltPaletteMods &paletteMod = hovered ? graphicsSet.hoveredPaletteMods : graphicsSet.idlePaletteMods;
			applyPaletteMod(_graphics, button._plane, paletteMod, 0);
		}
		else if (graphicsSet.graphicsType == kSprites) {
			const BltSprites &spriteList = hovered ? graphicsSet.hoveredSprites : graphicsSet.idleSprites;
			if (spriteList.getSpriteCount() > 0) {
				Common::Point pos = spriteList.getSpritePosition(0) - _origin;
				const BltImage* spriteImage = spriteList.getSpriteImage(0);
				if (spriteImage) {
					spriteImage->drawAt(_graphics->getPlaneSurface(button._plane), pos.x, pos.y, true);
				}
			}
		}
	}
}

void Scene::drawButtons(int hoveredButton) {
	for (int i = 0; i < _buttons.size(); ++i) {
		drawButton(_buttons[i], (int)i == hoveredButton);
	}
	_graphics->markDirty();
}

void Scene::loadPlane(Plane &plane, Boltlib &boltlib, BltId planeId) {
	BltPlane bltPlane;
	loadBltResource(bltPlane, boltlib, planeId);

	plane.image.load(boltlib, bltPlane.imageId);
	plane.palette.load(boltlib, bltPlane.paletteId);
	plane.hotspots.load(boltlib, bltPlane.hotspotsId);
}

void loadScene(Scene &scene, FunhouseEngine *engine, Boltlib &boltlib, BltId sceneId) {
	BltScene sceneInfo;
	loadBltResource(sceneInfo, boltlib, sceneId);

	scene.setOrigin(sceneInfo.origin);
	scene.loadBackPlane(boltlib, sceneInfo.backPlaneId);
	scene.loadForePlane(boltlib, sceneInfo.forePlaneId);
	scene.loadColorCycles(boltlib, sceneInfo.colorCyclesId);
	scene.loadSprites(boltlib, sceneInfo.spritesId);

	BltButtonList buttons;
	loadBltResourceArray(buttons, boltlib, sceneInfo.buttonsId);

	scene.init(engine, sceneInfo.numButtons, sceneInfo.numSprites);

	for (uint i = 0; i < buttons.size(); ++i) {
		Scene::Button &button = scene.getButton(i);
		button.setEnable(true);
		button.setHotspot(static_cast<Scene::HotspotType>(buttons[i].type), buttons[i].rect);
		button.setPlane(buttons[i].plane);
		button.loadGraphicsSet(boltlib, buttons[i].graphicsId);
	}
}

} // End of namespace Funhouse
