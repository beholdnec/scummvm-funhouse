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

	void load(IBoltEventLoop *eventLoop, Graphics *graphics, Boltlib &boltlib, BltId sceneId);
	void enter();
    void redrawSprites();
	BoltCmd handleMsg(const BoltMsg &msg);

	void setBackPlane(Boltlib &boltlib, BltId id);
	const Common::Point& getOrigin() const { return _origin; }
    BltSprites& getSprites() { return _sprites; }

private:
	IBoltEventLoop *_eventLoop;
	Graphics *_graphics;

    struct Plane {
        BltImage image;
        BltPalette palette;
        BltImage hotspots;
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

	typedef ScopedArray<ButtonGraphics> ButtonGraphicsArray;

	struct Button {
		HotspotType hotspotType;
		uint16 plane;
        // If hotspotType == kRect: this field holds the rectangular area of the button.
        // If hotspotType == kDisplayQuery: this field holds the min and max color indices of the button in the visible plane.
        // If hotspotType == kHotspotQuery: this field holds the min and max color indices of the button in the hotspot image.
		Rect hotspot;

		ButtonGraphicsArray graphics;
	};

	typedef ScopedArray<Button> ButtonArray;

    // Return the number of button at a given point, or return -1 if there is no button.
    int getButtonAtPoint(const Common::Point &pt);
    void drawButton(const Button &button, bool hovered);

    Plane _forePlane;
    Plane _backPlane;
    Common::ScopedPtr<BltColorCycles> _colorCycles;
    BltSprites _sprites;
	Common::Point _origin;
	ButtonArray _buttons;
};

} // End of namespace Funhouse

#endif
