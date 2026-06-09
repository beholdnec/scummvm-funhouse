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

	
#include "common/pack-start.h"	// START STRUCT PACKING

struct BltPlane {
	// Type 26
	BltPtr<byte> image;
	BltPtr<byte> palette;
} PACKED_STRUCT;

struct BltPaletteMod {
	// Type 29
	byte start;
	byte count;
	BltPtr<byte> colors;
} PACKED_STRUCT;

struct BltButtonGfx {
	// Type 30
	uint16 type;
	uint32 unk0x2;
	BltPtr<byte> hovered;
	BltPtr<byte> idle;
} PACKED_STRUCT;

struct BltButton {
	// Type 31
	uint16 type;
	uint16 left;
	uint16 right;
	uint16 top;
	uint16 bottom;
	uint16 plane;
	uint16 gfxCount;
	uint16 unk0xe;
	BltPtr<BltButtonGfx> gfx;
} PACKED_STRUCT;

struct BltScene {
	// Type 32
	BltPtr<BltPlane> forePlane;
	BltPtr<BltPlane> backPlane;
	uint32 unk0x8;
	uint32 unk0xc;
	uint32 unk0x10;
	uint32 unk0x14;
	uint16 unk0x18;
	uint16 buttonCount;
	BltPtr<BltButton> buttons;
} PACKED_STRUCT;

#include "common/pack-end.h"	// END STRUCT PACKING

struct Scene {
	const BltScene *bltScene;
};

Scene* MerlinEngine::loadScene(const byte* bltScene) {
	Scene *scene = (Scene*)_xp->allocMem(sizeof(Scene));

	scene->bltScene = (const BltScene*)bltScene;

	return scene;
}

void MerlinEngine::drawScene(const Scene* scene, byte flags) {
	if (flags & 0x20) {
		const BltPlane *backPlane = getResolved(scene->bltScene->backPlane);
		const byte *backPalette = getResolved(backPlane->palette);
		if (backPalette) {
			_xp->fillDisplay(0, 0);
			// FIXME: displayColors appears to behave differently between Carnival and Merlin...
			displayColors(backPalette, 1, 0);
			const byte *backImage = getResolved(backPlane->image);
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

void MerlinEngine::drawSceneBackground(const BltScene* bltScene, byte plane) {
	for (uint16 i = 0; i < bltScene->buttonCount; i++) {
		// TODO: look for type 2 buttons
	}

	const BltPlane *bltPlane = (plane == 0) ? getResolved(bltScene->forePlane) : getResolved(bltScene->backPlane);
	if (!bltPlane) {
		_xp->fillDisplay(0, plane);
	} else {
		const byte *palette = getResolved(bltPlane->palette);
		if (palette) {
			displayColors(palette, plane, 0);
		}
		const byte *image = getResolved(bltPlane->image);
		if (image) {
			displayPic(image, 0, 0, plane);
		} else {
			_xp->fillDisplay(0, plane);
		}
	}
}

void MerlinEngine::updateSceneButtons(Scene* scene, int x, int y) {
	for (int i = 0; i < scene->bltScene->buttonCount; i++) {
		const BltButton *bltButtons = getResolved(scene->bltScene->buttons);
		const BltButtonGfx *bltButtonGfx = getResolved(bltButtons[i].gfx);
		if (bltButtonGfx) {
			drawSceneButton(bltButtonGfx, bltButtons[i].plane != 0 ? 1 : 0);
		}
	}
}

void MerlinEngine::drawSceneButton(const BltButtonGfx* bltButtonGfx, uint16 plane) {
	uint16 gfxType = READ_UINT16(bltButtonGfx + 0x0);
	if (gfxType == 1) {
		// Modify palette
		// TODO: type checking?
		const BltPaletteMod *hovered = reinterpret_cast<const BltPaletteMod*>(getResolved(bltButtonGfx->hovered));
		if (hovered) {
			applyPaletteMod(hovered, plane << 7);
		}
	}
}

void MerlinEngine::applyPaletteMod(const BltPaletteMod* bltPaletteMod, byte dest) {
	const byte *rgb = getResolved(bltPaletteMod->colors);
	_xp->setPalette(bltPaletteMod->count, dest + bltPaletteMod->start, rgb);
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
	BltButton *ptr = reinterpret_cast<BltButton*>(data);

	while (offset < decompSize) {
		WRITE_UINT16(&ptr->type, READ_BE_UINT16(&ptr->type));
		WRITE_UINT16(&ptr->left, READ_BE_UINT16(&ptr->left));
		WRITE_UINT16(&ptr->right, READ_BE_UINT16(&ptr->right));
		WRITE_UINT16(&ptr->top, READ_BE_UINT16(&ptr->top));
		WRITE_UINT16(&ptr->bottom, READ_BE_UINT16(&ptr->bottom));
		WRITE_UINT16(&ptr->plane, READ_BE_UINT16(&ptr->plane));
		WRITE_UINT16(&ptr->gfxCount, READ_BE_UINT16(&ptr->gfxCount));
		resolveIt(&ptr->gfx.ptr);
		offset += sizeof(BltButton);
		ptr++;
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
