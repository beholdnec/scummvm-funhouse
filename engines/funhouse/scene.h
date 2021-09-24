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

#ifndef BOLT_SCENE_H
#define BOLT_SCENE_H

#define FORBIDDEN_SYMBOL_ALLOW_ALL // fix #include <functional>

#include "common/array.h"
#include "common/rect.h"

#include "funhouse/bolt.h"
#include "funhouse/boltlib/boltlib.h"
#include "funhouse/boltlib/palette.h"
#include "funhouse/boltlib/sprites.h"

namespace Funhouse {

enum ButtonGraphicsType {
	kPaletteMods = 1,
	kSprite = 2
};

struct ButtonGraphics {
	ButtonGraphicsType graphicsType;

	// If graphicsType == kPaletteMods
	BltPaletteMods alternatePaletteMods;
	BltPaletteMods hoveredPaletteMods;
	BltPaletteMods idlePaletteMods;

	// If graphicsType == kSprites
	SharedSprite alternateSprite;
	SharedSprite hoveredSprite;
	SharedSprite idleSprite;
};

typedef Common::SharedPtr<Common::Array<ButtonGraphics> > SharedButtonGraphics;
SharedButtonGraphics loadButtonGraphics(Boltlib &boltlib, BltId id);

enum SceneDrawFlags {
	kDrawBack = (1 << 0),
	kDrawFore = (1 << 1),
	kDrawForeSprites = (1 << 2),
	kDrawButtons = (1 << 3),
	kDrawAll = kDrawBack | kDrawFore | kDrawForeSprites | kDrawButtons,
};

class Scene {
public:
	enum SceneMsg {
		kClickButton = BoltMsg::kSceneMsgs,
	};

	enum HotspotType {
		kRect = 1,
		kDisplayQuery = 2, // Query the visible image (FIXME: which plane?)
		kHotspotQuery = 3 // Query the hotspot image
	};

	class Button {
	public:
		void setEnable(bool enable);
		void setGraphics(SharedButtonGraphics graphicsSet);
		void setState(bool state);
		void setInstance(int instance);
		void setHotspot(HotspotType type, Rect hotspot);
		void setPlane(uint16 plane);

	private:
		friend class Scene;

		bool _enable = false;
		SharedButtonGraphics _graphicsSet;
		bool _state = false; // True if hovered
		int _instance = 0; // selects graphics from set; not the clearest name but the original programmers used it.

		uint16 _plane; // 0: fore; 1: back

		HotspotType _hotspotType;
		// If hotspotType == kRect: this field holds the rectangular area of the button.
		// If hotspotType == kDisplayQuery: this field holds the min and max color indices of the button in the visible plane.
		// If hotspotType == kHotspotQuery: this field holds the min and max color indices of the button in the hotspot image.
		Rect _hotspot;
	};

	Scene();

	void init(FunhouseEngine *engine, int buttonCount);
	void enter();
	void redraw(SceneDrawFlags flags = kDrawAll);
	BoltRsp handleMsg(const BoltMsg &msg);

	void loadForePlane(Boltlib &boltlib, BltId planeId);
	void loadBackPlane(Boltlib &boltlib, BltId planeId);
	void loadColorCycles(Boltlib &boltlib, BltId id);
	void loadForeSprites(Boltlib &boltlib, BltId id);

	Common::Point getOrigin() const;
	void setOrigin(const Common::Point &origin);
	void setPlaneImageEnable(int plane, bool enable);
	SharedSpriteList& getForeSprites();

	Button& getButton(int num);

private:
	struct Plane {
		bool enableImage = true;
		BltImage image;
		BltPalette palette;
		BltImage hotspots;
	};
	
	void loadPlane(Plane &plane, Boltlib &boltlib, BltId planeId);
	// Return the button at a given point, or -1 if there is no button.
	int getButtonAtPoint(const Common::Point &pt);
	void drawButton(const Button &button);
	void drawButtonGraphics(const ButtonGraphics &buttonGraphics, bool state, int plane);
	void updateButtons(const Common::Point *cursor);
	void drawAllButtons();

	FunhouseEngine *_engine;

	Common::Point _origin;
	Plane _forePlane;
	Plane _backPlane;
	Common::ScopedPtr<BltColorCycles> _colorCycles;

	Common::Array<int> _currButtonInstances;
	Common::Array<Button> _buttons;
	SharedSpriteList _foreSprites;

	int _hoveredButton = -1;
};

void loadScene(Scene &scene, FunhouseEngine *engine, Boltlib &boltlib, BltId sceneId);

} // End of namespace Funhouse

#endif
