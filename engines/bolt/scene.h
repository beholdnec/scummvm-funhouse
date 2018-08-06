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

#include "bolt/bolt.h"
#include "bolt/boltlib/boltlib.h"
#include "bolt/boltlib/palette.h"

namespace Bolt {

class BoltEngine;

class Scene {
public:
	enum SceneMsg {
		kClickButton = BoltMsg::kMaxBoltMsg
	};

	void load(IBoltEventLoop *eventLoop, Graphics *graphics, Boltlib &boltlib, BltId sceneId);
	void enter();
	BoltCmd handleMsg(const BoltMsg &msg);

	void setBackPlane(Boltlib &boltlib, BltId id);


private:
	IBoltEventLoop *_eventLoop;
	Graphics *_graphics;

	struct BltPlane { // type 26
		static const uint32 kType = kBltPlane;
		static const uint kSize = 0x10;
		void load(const ConstSizedDataView<kSize> src, Boltlib &bltFile) {
			BltId imageId(src.readUint32BE(0));
			image.load(bltFile, imageId);
			BltId paletteId(src.readUint32BE(4));
			palette.load(bltFile, paletteId);
			BltId hotspotsId(src.readUint32BE(8));
			hotspots.load(bltFile, hotspotsId);
		}

		BltImage image;
		BltPalette palette;
		BltImage hotspots;
	};

	BltPlane _forePlane;
	BltPlane _backPlane;
	Common::ScopedPtr<BltColorCycles> _colorCycles;

	enum HotspotType {
		kRect = 1,
		// 2 is unused, but indicates a query of the visible display.
		kHotspotQuery = 3 // Query the hotspot image
	};

	enum GraphicsType {
		kPaletteMods = 1,
		kSprites = 2
	};

	struct Sprite {
		Common::Point pos;
		BltImage image;
	};

	typedef ScopedArray<Sprite> SpriteArray;

	SpriteArray _sprites;

	struct ButtonGraphics {
		GraphicsType graphicsType;

		// If graphicsType == kPaletteMods
		BltPaletteMods hoveredPaletteMods;
		BltPaletteMods idlePaletteMods;

		// If graphicsType == kSprites
		SpriteArray hoveredSprites;
		SpriteArray idleSprites;
	};

	typedef ScopedArray<ButtonGraphics> ButtonGraphicsArray;

	struct Button {
		HotspotType hotspotType;
		uint16 plane;
		// If hotspotType == kHotspotQuery, this value holds the range of color
		// indices in the hotspot image that correspond to this button.
		Rect hotspot;

		ButtonGraphicsArray graphics;
	};

	typedef ScopedArray<Button> ButtonArray;

	Common::Point _origin;
	ButtonArray _buttons;

	// Return number of button at a point, or -1 if there is no button.
	void loadSpriteArray(SpriteArray& spriteArray, Boltlib &boltlib, BltId id);
	int getButtonAtPoint(const Common::Point &pt);
	void drawButton(const Button &button, bool hovered);
};

} // End of namespace Bolt

#endif
