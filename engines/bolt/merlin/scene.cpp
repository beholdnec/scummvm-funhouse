/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "bolt/merlin/merlin.h"

namespace Bolt {

namespace Merlin {

struct Scene {
	const byte *bltScene;
};

Scene* MerlinEngine::loadScene(const byte* bltScene) {
	Scene *scene = (Scene*)_xp->allocMem(sizeof(Scene));

	scene->bltScene = bltScene;

	return scene;
}

void MerlinEngine::drawScene(const Scene* scene, byte flags) {
	if (flags & 0x20) {
		const byte *backPlane = getResolvedPtr(scene->bltScene, 0x4);
		const byte *backPalette = getResolvedPtr(backPlane, 0x4);
		if (backPalette) {
			_xp->fillDisplay(0, 0);
			// FIXME: displayColors appears to behave differently between Carnival and Merlin...
			displayColors(backPalette, 1, 0);
			const byte *backImage = getResolvedPtr(backPlane, 0x0);
			// FIXME: should be displayed on back page
			displayPic(backImage, 0, 0, 1);
			if (flags & 0x2) {
				drawSceneBackground(scene->bltScene, 0);
			}
			if (flags & 0x1) {
				drawSceneBackground(scene->bltScene, 1);
			}
		}
	}
}

void MerlinEngine::drawSceneBackground(const byte* bltScene, byte plane) {
	uint16 buttonCount = READ_UINT16(bltScene + 0x1a);
	for (uint16 i = 0; i < buttonCount; i++) {
		// TODO: look for type 2 buttons
	}

	const byte *bltPlane = (plane == 0) ? getResolvedPtr(bltScene, 0x0) : getResolvedPtr(bltScene, 0x4);
	if (!bltPlane) {
		_xp->fillDisplay(0, plane);
	} else {
		const byte *palette = getResolvedPtr(bltPlane, 0x4);
		if (palette) {
			displayColors(palette, plane, 0);
		}
		const byte *image = getResolvedPtr(bltPlane, 0x0);
		if (image) {
			displayPic(image, 0, 0, plane);
		} else {
			_xp->fillDisplay(0, plane);
		}
	}
}

void MerlinEngine::updateSceneButtons(Scene* scene, int x, int y) {
	uint16 buttonCount = READ_UINT16(scene->bltScene + 0x1a);
	for (int i = 0; i < buttonCount; i++) {
		const byte *bltButtons = getResolvedPtr(scene->bltScene, 0x1c);
		const byte *bltButtonGfx = getResolvedPtr(bltButtons, 0x14 * i + 0x10);
		if (bltButtonGfx) {
			uint16 plane = READ_UINT16(bltButtons + 0x14 * i + 0xa);
			drawSceneButton(bltButtonGfx, plane != 0 ? 1 : 0);
		}
	}
}

void MerlinEngine::drawSceneButton(const byte* bltButtonGfx, uint16 plane) {
	uint16 gfxType = READ_UINT16(bltButtonGfx + 0x0);
	if (gfxType == 1) {
		// Modify palette
		const byte *hovered = getResolvedPtr(bltButtonGfx, 0x6);
		if (hovered) {
			applyPaletteMod(hovered, plane << 7);
		}
	}
}

void MerlinEngine::applyPaletteMod(const byte* bltPaletteMod, byte dest) {
	byte start = bltPaletteMod[0x0];
	byte count = bltPaletteMod[0x1];
	const byte *rgb = getResolvedPtr(bltPaletteMod, 0x2);
	_xp->setPalette(count, dest + start, rgb);
}

void MerlinEngine::swapPlaneDesc() {
	byte *data = _boltCurrentMemberEntry->dataPtr;
	uint32 decompSize = _boltCurrentMemberEntry->decompSize;
	uint32 offset = 0;
	byte *ptr = data;

	while (offset < decompSize) {
		resolveIt((uint32 *)ptr);
		resolveIt((uint32 *)(ptr + 0x4));
		resolveIt((uint32 *)(ptr + 0x8));
		offset += 0x10;
		ptr += 0x10;
	}
}

void MerlinEngine::swapPaletteModDesc() {
	byte *data = _boltCurrentMemberEntry->dataPtr;
	uint32 decompSize = _boltCurrentMemberEntry->decompSize;
	uint32 offset = 0;
	byte *ptr = data;

	while (offset < decompSize) {
		resolveIt((uint32 *)(ptr + 0x2));
		offset += 0x6;
		ptr += 0x6;
	}
}

void MerlinEngine::swapButtonGfxDesc() {
	byte *data = _boltCurrentMemberEntry->dataPtr;
	uint32 decompSize = _boltCurrentMemberEntry->decompSize;
	uint32 offset = 0;
	byte *ptr = data;

	while (offset < decompSize) {
		WRITE_UINT16(ptr + 0x0, READ_BE_UINT16(ptr + 0x0));
		resolveIt((uint32 *)(ptr + 0x6));
		resolveIt((uint32 *)(ptr + 0xa));
		offset += 0xe;
		ptr += 0xe;
	}
}

void MerlinEngine::swapButtonDesc() {
	byte *data = _boltCurrentMemberEntry->dataPtr;
	uint32 decompSize = _boltCurrentMemberEntry->decompSize;
	uint32 offset = 0;
	byte *ptr = data;

	while (offset < decompSize) {
		WRITE_UINT16(ptr + 0x0, READ_BE_UINT16(ptr + 0x0));
		WRITE_UINT16(ptr + 0xa, READ_BE_UINT16(ptr + 0xa));
		WRITE_UINT16(ptr + 0xc, READ_BE_UINT16(ptr + 0xc));
		resolveIt((uint32 *)(ptr + 0x10));
		offset += 0x14;
		ptr += 0x14;
	}
}

void MerlinEngine::swapSceneDesc() {
	byte *data = _boltCurrentMemberEntry->dataPtr;
	uint32 decompSize = _boltCurrentMemberEntry->decompSize;
	uint32 offset = 0;
	byte *ptr = data;

	while (offset < decompSize) {
		resolveIt((uint32 *)ptr);
		resolveIt((uint32 *)(ptr + 0x4));
		resolveIt((uint32 *)(ptr + 0xa));
		resolveIt((uint32 *)(ptr + 0xe));
		resolveIt((uint32 *)(ptr + 0x12));
		resolveIt((uint32 *)(ptr + 0x16));
		WRITE_UINT16(ptr + 0x1a, READ_BE_UINT16(ptr + 0x1a));
		resolveIt((uint32 *)(ptr + 0x1c));
		offset += 0x24;
		ptr += 0x24;
	}
}

} // End of namespace Merlin

} // End of namespace Bolt
