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
		forePlaneId = BltId(src.getUint32BEAt(0x0));
		backPlaneId = BltId(src.getUint32BEAt(0x4));
		foreSpriteCount = src.getUint8At(0x8);
		foreSpritesId = BltId(src.getUint32BEAt(0xA));
		// FIXME: unknown fields at 0xD..0x16
		colorCyclesId = BltId(src.getUint32BEAt(0x16));
		buttonCount = src.getUint16BEAt(0x1A);
		buttonsId = BltId(src.getUint32BEAt(0x1C));
		origin.x = src.getInt16BEAt(0x20);
		origin.y = src.getInt16BEAt(0x22);
	}

	BltId forePlaneId;
	BltId backPlaneId;
	uint8 foreSpriteCount;
	BltId foreSpritesId;
	BltId colorCyclesId;
	uint16 buttonCount;
	BltId buttonsId;
	Common::Point origin;
};

struct BltPlane { // type 26
	static const uint32 kType = kBltPlane;
	static const uint kSize = 0x10;
	void load(Common::Span<const byte> src, Boltlib &bltFile) {
		imageId = BltId(src.getUint32BEAt(0x0));
		paletteId = BltId(src.getUint32BEAt(0x4));
		hotspotsId = BltId(src.getUint32BEAt(0x8));
	}

	BltId imageId;
	BltId paletteId;
	BltId hotspotsId;
};

struct BltButtonGraphicElement { // type 30
	static const uint32 kType = kBltButtonGraphicsList;
	static const uint kSize = 0xe;
	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		type = src.getUint16BEAt(0x0);
		alternateId = BltId(src.getUint32BEAt(0x2));
		hoveredId = BltId(src.getUint32BEAt(0x6));
		idleId = BltId(src.getUint32BEAt(0xa));
	}

	uint16 type;
	BltId alternateId; // Holds palette mods when type==kSprites, or sprites when type==kPaletteMod
	BltId hoveredId;
	BltId idleId;
};

typedef Common::Array<BltButtonGraphicElement> BltButtonGraphicsList;

struct BltButtonElement { // type 31
	static const uint32 kType = kBltButtonList;
	static const uint kSize = 0x14;
	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		type = src.getUint16BEAt(0x0);
		rect = Rect(src.subspan(0x2));
		plane = src.getUint16BEAt(0xa);
		numGraphics = src.getUint16BEAt(0xc);
		// FIXME: unknown field at 0xe. Always 0 in game data.
		graphicsId = BltId(src.getUint32BEAt(0x10));
	}

	enum HotspotType {
		Rectangle = 1,
		// 2 is visible display query (unused)
		HotspotQuery = 3,
	};

	uint16 type;
	Rect rect;
	uint16 plane;
	uint16 numGraphics;
	BltId graphicsId;
};

typedef Common::Array<BltButtonElement> BltButtonList;

Scene::Scene() : _engine(nullptr)
{ }

void Scene::init(FunhouseEngine *engine, int buttonCount) {
	_engine = engine;
	_buttons.resize(buttonCount);
}

void Scene::enter() {
	applyPalette(_engine->getGraphics(), kBack, _backPlane.palette);
	applyPalette(_engine->getGraphics(), kFore, _forePlane.palette);
	applyColorCycles(_engine->getGraphics(), kBack, _colorCycles.get());
	_engine->requestHover();
	redraw();
}

void Scene::redraw() {
	if (_backPlane.image && _backPlane.enableImage) {
		_backPlane.image.drawAt(_engine->getGraphics()->getPlaneSurface(kBack), 0, 0, false);
	} else {
		_engine->getGraphics()->clearPlane(kBack);
	}

	if (_forePlane.image && _forePlane.enableImage) {
		_forePlane.image.drawAt(_engine->getGraphics()->getPlaneSurface(kFore), 0, 0, false);
	} else {
		_engine->getGraphics()->clearPlane(kFore);
	}

	for (int i = 0; i < _foreSprites->size(); ++i) {
		Common::Point position = (*_foreSprites)[i]->pos - _origin;
		(*_foreSprites)[i]->image->drawAt(_engine->getGraphics()->getPlaneSurface(kFore), position.x, position.y, true);
	}

	drawButtons(getButtonAtPoint(_engine->getEventManager()->getMousePos()));

	_engine->getGraphics()->markDirty();
}

BoltRsp Scene::handleMsg(const BoltMsg &msg) {
	switch (msg.type) {
	case BoltMsg::kHover: {
		int hoveredButton = getButtonAtPoint(msg.point);
		drawButtons(hoveredButton);
		break;
	}

	case BoltMsg::kClick: {
		BoltMsg newMsg(kClickButton);
		newMsg.num = getButtonAtPoint(msg.point);
		_engine->setNextMsg(newMsg);
		break;
	}
	}

	return BoltRsp::kDone;
}

void Scene::loadForePlane(Boltlib &boltlib, BltId planeId) {
	loadPlane(_forePlane, boltlib, planeId);
}

void Scene::loadBackPlane(Boltlib &boltlib, BltId planeId) {
	loadPlane(_backPlane, boltlib, planeId);
}

void Scene::loadColorCycles(Boltlib &boltlib, BltId id) {
	_colorCycles.reset();
	if (id.isValid()) {
		_colorCycles.reset(new BltColorCycles);
		loadBltResource(*_colorCycles, boltlib, id);
	}
}

void Scene::loadForeSprites(Boltlib &boltlib, BltId id) {
	_foreSprites.reset();
	_foreSprites = loadBltSprites(boltlib, id);
}

Common::Point Scene::getOrigin() const {
	return _origin;
}

void Scene::setOrigin(const Common::Point &origin) {
	_origin = origin;
}

void Scene::setPlaneImageEnable(int plane, bool enableImage) {
	Plane &p = (plane == kBack) ? _backPlane : _forePlane;
	p.enableImage = enableImage;
}

SharedSpriteList& Scene::getForeSprites() {
	return _foreSprites;
}

void Scene::Button::setEnable(bool enable) {
	_enable = enable;
}

void Scene::Button::setGraphics(SharedButtonGraphics graphicsSet) {
	_graphicsSet = graphicsSet;
}

void Scene::Button::setGraphicsIdx(int idx) {
	_graphicsIdx = idx;
}

void Scene::Button::setHotspot(HotspotType type, Rect hotspot) {
	_hotspotType = type;
	_hotspot = hotspot;
}

void Scene::Button::setPlane(uint16 plane) {
	_plane = plane;
}

SharedButtonGraphics loadButtonGraphics(Boltlib &boltlib, BltId id) {
	if (!id.isValid()) {
		return nullptr;
	}

	BltButtonGraphicsList buttonGraphics;
	loadBltResourceArray(buttonGraphics, boltlib, id);

	SharedButtonGraphics graphicsSet(new Common::Array<ButtonGraphics>(buttonGraphics.size()));

	for (uint i = 0; i < buttonGraphics.size(); ++i) {
		(*graphicsSet)[i].graphicsType = static_cast<ButtonGraphicsType>(buttonGraphics[i].type);
		if (buttonGraphics[i].type == kPaletteMods) {
			if (buttonGraphics[i].alternateId.isValid()) {
				SharedSpriteList alternateSprites = loadBltSprites(boltlib, buttonGraphics[i].alternateId);
				(*graphicsSet)[i].alternateSprite = (*alternateSprites)[0];
			}
			loadBltResourceArray((*graphicsSet)[i].hoveredPaletteMods, boltlib, buttonGraphics[i].hoveredId);
			loadBltResourceArray((*graphicsSet)[i].idlePaletteMods, boltlib, buttonGraphics[i].idleId);
		}
		else if (buttonGraphics[i].type == kSprite) {
			loadBltResourceArray((*graphicsSet)[i].alternatePaletteMods, boltlib, buttonGraphics[i].alternateId);
			// TODO: load sprites to a common location, not individually per button
			if (buttonGraphics[i].hoveredId.isValid()) {
				SharedSpriteList hoveredSprites = loadBltSprites(boltlib, buttonGraphics[i].hoveredId);
				(*graphicsSet)[i].hoveredSprite = (*hoveredSprites)[0];
			}
			if (buttonGraphics[i].idleId.isValid()) {
				SharedSpriteList idleSprites = loadBltSprites(boltlib, buttonGraphics[i].idleId);
				(*graphicsSet)[i].idleSprite = (*idleSprites)[0];
			}
		}
	}

	return graphicsSet;
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
			if (button._hotspotType == kRect) {
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

	if (button._graphicsSet && !button._graphicsSet->empty()) {
		const ButtonGraphics& graphicsSet = (*button._graphicsSet)[button._graphicsIdx];
		if (graphicsSet.graphicsType == kPaletteMods) {
			if (graphicsSet.alternateSprite) {
				SharedImage spriteImage = graphicsSet.alternateSprite->image;
				if (spriteImage) {
					Common::Point pos = graphicsSet.alternateSprite->pos - _origin;
					spriteImage->drawAt(_engine->getGraphics()->getPlaneSurface(button._plane), pos.x, pos.y, true);
				}
			}
			const BltPaletteMods &paletteMod = hovered ? graphicsSet.hoveredPaletteMods : graphicsSet.idlePaletteMods;
			applyPaletteMod(_engine->getGraphics(), button._plane, paletteMod, 0);
		}
		else if (graphicsSet.graphicsType == kSprite) {
			if (!graphicsSet.alternatePaletteMods.empty()) {
				applyPaletteMod(_engine->getGraphics(), button._plane, graphicsSet.alternatePaletteMods, 0);
			}
			SharedSprite sprite = hovered ? graphicsSet.hoveredSprite : graphicsSet.idleSprite;
			if (sprite) {
				Common::Point pos = sprite->pos - _origin;
				if (sprite->image) {
					sprite->image->drawAt(_engine->getGraphics()->getPlaneSurface(button._plane), pos.x, pos.y, true);
				}
			}
		}
	}
}

void Scene::drawButtons(int hoveredButton) {
	for (int i = 0; i < (int)_buttons.size(); ++i) {
		drawButton(_buttons[i], i == hoveredButton);
	}
	_engine->getGraphics()->markDirty();
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

	scene.init(engine, sceneInfo.buttonCount);
	scene.setOrigin(sceneInfo.origin);
	scene.loadBackPlane(boltlib, sceneInfo.backPlaneId);
	scene.loadForePlane(boltlib, sceneInfo.forePlaneId);
	scene.loadColorCycles(boltlib, sceneInfo.colorCyclesId);
	scene.loadForeSprites(boltlib, sceneInfo.foreSpritesId);

	BltButtonList buttons;
	loadBltResourceArray(buttons, boltlib, sceneInfo.buttonsId);

	for (uint i = 0; i < buttons.size(); ++i) {
		Scene::Button &button = scene.getButton(i);
		button.setEnable(true);
		button.setHotspot(static_cast<Scene::HotspotType>(buttons[i].type), buttons[i].rect);
		button.setPlane(buttons[i].plane);
		button.setGraphics(loadButtonGraphics(boltlib, buttons[i].graphicsId));
		button.setGraphicsIdx(0);
	}
}

} // End of namespace Funhouse
