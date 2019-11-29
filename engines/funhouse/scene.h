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

	void setButtonEnable(int num, bool enable);
	void* getButtonUserData(int num) const;
	void setButtonUserData(int num, void *userData);
	void loadButtonGraphicsSet(int num, Boltlib &boltlib, BltId id);
	void setButtonGraphics(int num, int graphicsNum);
	void setButtonPlane(int num, uint16 plane);
	void setButtonHotspot(int num, HotspotType type, Rect hotspot);
	void overrideButtonGraphics(int buttonNumber, Common::Point position, BltImage* hoveredImage, BltImage* idleImage);

private:
    struct Plane {
        BltImage image;
        BltPalette palette;
        BltImage hotspots;
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

	typedef ScopedArray<ButtonGraphics> ButtonGraphicsArray;

	struct Button {
		Button();

		bool enable;
		void* userData;
		ButtonGraphicsArray graphicsSet;
		int graphicsNum;

		uint16 plane; // ??? TODO: remove?

		HotspotType hotspotType;
        // If hotspotType == kRect: this field holds the rectangular area of the button.
        // If hotspotType == kDisplayQuery: this field holds the min and max color indices of the button in the visible plane.
        // If hotspotType == kHotspotQuery: this field holds the min and max color indices of the button in the hotspot image.
		Rect hotspot;

		bool overrideGraphics;
		BltImage *overrideHoveredImage;
		BltImage *overrideIdleImage;
		Common::Point overridePosition;
	};

	typedef ScopedArray<Button> ButtonArray;
	
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

	ButtonArray _buttons;
	BltSprites _sprites;
};

void loadScene(Scene &scene, FunhouseEngine *engine, Boltlib &boltlib, BltId sceneId);

} // End of namespace Funhouse

#endif
