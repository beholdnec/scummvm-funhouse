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

#include "common/array.h"
#include "common/rect.h"

#include "funhouse/bolt.h"
#include "funhouse/boltlib/boltlib.h"
#include "funhouse/boltlib/palette.h"
#include "funhouse/boltlib/sprites.h"

namespace Funhouse {

class Scene {
public:
	enum SceneMsg {
		kClickButton = BoltMsg::kMaxBoltMsg
	};

	enum HotspotType {
		kRect = 1,
		kDisplayQuery = 2, // Query the visible image (FIXME: which plane?)
		kHotspotQuery = 3 // Query the hotspot image
	};

	enum GraphicsType {
		kPaletteMods = 1,
		kSprites = 2
	};

	struct ButtonGraphics {
		GraphicsType graphicsType;

		// If graphicsType == kPaletteMods
		BltPaletteMods hoveredPaletteMods;
		BltPaletteMods idlePaletteMods;

		// If graphicsType == kSprites
		BltSprites hoveredSprites;
		BltSprites idleSprites;
	};

	class Button {
	public:
		Button();

		void setEnable(bool enable);
		void* getUserData() const;
		void setUserData(void *userData);
		void setGraphics(int num);
		void setHotspot(HotspotType type, Rect hotspot);
		void setPlane(uint16 plane);
		void loadGraphicsSet(Boltlib &boltlib, BltId id);
		void overrideGraphics(Common::Point position, BltImage* hoveredImage, BltImage* idleImage);

	private:
		friend class Scene;

		bool _enable;
		void* _userData;
		ScopedArray<ButtonGraphics> _graphicsSet;
		int _graphicsNum;

		uint16 _plane; // ??? TODO: remove?

		HotspotType _hotspotType;
		// If hotspotType == kRect: this field holds the rectangular area of the button.
		// If hotspotType == kDisplayQuery: this field holds the min and max color indices of the button in the visible plane.
		// If hotspotType == kHotspotQuery: this field holds the min and max color indices of the button in the hotspot image.
		Rect _hotspot;

		bool _overrideGraphics;
		BltImage *_overrideHoveredImage;
		BltImage *_overrideIdleImage;
		Common::Point _overridePosition;
	};

	Scene();

	void init(FunhouseEngine *engine, int numButtons, int numSprites);
	void enter();
	void redraw();
	BoltCmd handleMsg(const BoltMsg &msg);

	void loadBackPlane(Boltlib &boltlib, BltId planeId);
	void loadForePlane(Boltlib &boltlib, BltId planeId);
	void loadColorCycles(Boltlib &boltlib, BltId id);
	void loadSprites(Boltlib &boltlib, BltId id);

	Common::Point getOrigin() const;
	void setOrigin(const Common::Point &origin);

	void setSpriteImageNum(int num, int imageNum);

	Button& getButton(int num);

private:
    struct Plane {
        BltImage image;
        BltPalette palette;
        BltImage hotspots;
    };
	
	void loadPlane(Plane &plane, Boltlib &boltlib, BltId planeId);
    // Return the button at a given point, or -1 if there is no button.
    int getButtonAtPoint(const Common::Point &pt);
    void drawButton(const Button &button, bool hovered);
	void drawButtons(int hoveredButton);

	FunhouseEngine *_engine;
	Graphics *_graphics;

	Common::Point _origin;
    Plane _forePlane;
    Plane _backPlane;
    Common::ScopedPtr<BltColorCycles> _colorCycles;

	ScopedArray<Button> _buttons;
	BltSprites _sprites;
};

void loadScene(Scene &scene, FunhouseEngine *engine, Boltlib &boltlib, BltId sceneId);

} // End of namespace Funhouse

#endif
