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

struct BltSpriteDesc {
	// Type 27
	int16 x;
	int16 y;
	BltPtr<byte> image;
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
	BltRect rect;
	uint16 plane;
	uint16 gfxCount;
	uint16 initialGfx;
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
	int16 originX;
	int16 originY;
} PACKED_STRUCT;

#include "common/pack-end.h"	// END STRUCT PACKING

struct Scene {
	const BltScene *bltScene;
	int hoveredX;
	int hoveredY;
	int hoveredButton;

	uint8 currGfx[300];
	uint8 buttonGfx[300];

	uint32 isIdle[300];
};

Scene* MerlinEngine::loadScene(BltScene* bltScene) {
	Scene *scene = (Scene*)_xp->allocMem(sizeof(Scene));

	resetButtonPlanes(); // FIXME: this shouldn't be here?

	scene->bltScene = bltScene;
	scene->hoveredX = -1;
	scene->hoveredY = -1;
	scene->hoveredButton = -1;

	_sceneOriginX = bltScene->originX;
	_sceneOriginY = bltScene->originY;

	for (int i = 0; i < bltScene->buttonCount; i++) {
		BltButton *button = &getResolved(bltScene->buttons)[i];
		scene->isIdle[i] = 1;
		scene->buttonGfx[i] = button->initialGfx;
		if (button->type == 1) {
			button->rect.left -= _sceneOriginX;
			button->rect.right -= _sceneOriginX;
			button->rect.top -= _sceneOriginY;
			button->rect.bottom -= _sceneOriginY;
		}
	}

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

	if (flags & 0x10) {
		// Draw buttons
		for (int i = 0; i < scene->bltScene->buttonCount; i++) {
			const BltButton *button = &getResolved(scene->bltScene->buttons)[i];
			if (button->gfxCount != 0) {
				// TODO: don't draw if button is disabled
				uint32 isIdle = scene->isIdle[i];
				if (button->plane == 0) {
					drawSceneButton(&getResolved(button->gfx)[scene->buttonGfx[i]], isIdle, _buttonPlane0);
				} else {
					drawSceneButton(&getResolved(button->gfx)[scene->buttonGfx[i]], isIdle, _buttonPlane1);
				}
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

void MerlinEngine::updateSceneButtons(Scene* scene, int x, int y, int8* currButton) {
	resetButtonPlanes();

	for (int i = 0; i < scene->bltScene->buttonCount; i++) {
		if (scene->buttonGfx[i] != scene->currGfx[i]) {
			scene->currGfx[i] = scene->buttonGfx[i];

			const BltButton *bltButtons = getResolved(scene->bltScene->buttons);
			const BltButtonGfx *bltButtonGfx = &getResolved(bltButtons[i].gfx)[scene->buttonGfx[i]];
			if (bltButtonGfx->hovered != bltButtonGfx->idle) {
				bool isIdle = (byte)scene->isIdle[i] != 0;
				if (bltButtons[i].plane == 0) {
					drawSceneButton(bltButtonGfx, isIdle, _buttonPlane0);
				} else {
					drawSceneButton(bltButtonGfx, isIdle, _buttonPlane1);
				}
			}
		}
	}

	int newHoveredButton = -1;
	if (scene->hoveredX == x && scene->hoveredY == y) {
		newHoveredButton = scene->hoveredButton;
	} else {
		scene->hoveredX = x;
		scene->hoveredY = y;
		for (int i = 0; i < scene->bltScene->buttonCount; i++) {
			if (newHoveredButton != -1) {
				break;
			}

			if (isPointInButton(&getResolved(scene->bltScene->buttons)[i], x, y)) {
				newHoveredButton = i;
			}
		}
	}

	int oldHoveredButton = scene->hoveredButton;
	if (newHoveredButton != oldHoveredButton) {
		// Unhover the old button
		if (oldHoveredButton != -1 && getResolved(scene->bltScene->buttons)[oldHoveredButton].gfxCount != 0) {
			scene->isIdle[oldHoveredButton] = 1;
		
			const BltButton *bltButtons = getResolved(scene->bltScene->buttons);
			const BltButtonGfx *bltButtonGfx = &getResolved(bltButtons[oldHoveredButton].gfx)[scene->buttonGfx[oldHoveredButton]];

			if (bltButtonGfx->hovered != bltButtonGfx->idle) {
				if (bltButtons[oldHoveredButton].plane == 0) {
					drawSceneButton(bltButtonGfx, true, _buttonPlane0);
				} else {
					drawSceneButton(bltButtonGfx, true, _buttonPlane1);
				}
			}
		}

		// Hover the new button
		if (newHoveredButton != -1 && getResolved(scene->bltScene->buttons)[newHoveredButton].gfxCount != 0) {
			scene->isIdle[newHoveredButton] = 0;
		
			const BltButton *bltButtons = getResolved(scene->bltScene->buttons);
			const BltButtonGfx *bltButtonGfx = &getResolved(bltButtons[newHoveredButton].gfx)[scene->buttonGfx[newHoveredButton]];

			if (bltButtonGfx->hovered != bltButtonGfx->idle) {
				if (bltButtons[newHoveredButton].plane == 0) {
					drawSceneButton(bltButtonGfx, false, _buttonPlane0);
				} else {
					drawSceneButton(bltButtonGfx, false, _buttonPlane1);
				}
			}
		}

		scene->hoveredButton = newHoveredButton;
	}

	if (currButton) {
		*currButton = newHoveredButton;
	}
}

void MerlinEngine::drawSceneButton(const BltButtonGfx* bltButtonGfx, bool idle, uint16 plane) {
	if (idle) {
		const byte *idleGfx = getResolved(bltButtonGfx->idle);
		if (idleGfx) {
			if (bltButtonGfx->type == 1) {
				const BltPaletteMod *idlePaletteMod = reinterpret_cast<const BltPaletteMod *>(idleGfx);
				applyPaletteMod(idlePaletteMod, plane << 7);
			} else {
				const BltSpriteDesc *idleSprite = reinterpret_cast<const BltSpriteDesc *>(idleGfx);
				displayPic(getResolved(idleSprite->image), idleSprite->x - _sceneOriginX, idleSprite->y - _sceneOriginY, plane);
			}
		}
	} else {
		const byte *hoveredGfx = getResolved(bltButtonGfx->hovered);
		if (hoveredGfx) {
			if (bltButtonGfx->type == 1) {
				const BltPaletteMod *hoveredPaletteMod = reinterpret_cast<const BltPaletteMod *>(hoveredGfx);
				applyPaletteMod(hoveredPaletteMod, plane << 7);
			} else {
				const BltSpriteDesc *hoveredSprite = reinterpret_cast<const BltSpriteDesc *>(hoveredGfx);
				displayPic(getResolved(hoveredSprite->image), hoveredSprite->x - _sceneOriginX, hoveredSprite->y - _sceneOriginY, plane);
			}
		}
	}
}

void MerlinEngine::applyPaletteMod(const BltPaletteMod* bltPaletteMod, byte dest) {
	const byte *rgb = getResolved(bltPaletteMod->colors);
	_xp->setPalette(bltPaletteMod->count, dest + bltPaletteMod->start, rgb);
}

void MerlinEngine::resetButtonPlanes() {
	_buttonPlane0 = 0;
	_buttonPlane1 = 1;
}

bool MerlinEngine::isPointInButton(const BltButton* bltButton, int x, int y) {
	if (bltButton->type == 1) {
		// Rectangle
		return bltButton->rect.contains(x, y);
	}

	return false; // TODO: other types
}

void MerlinEngine::setButtonGfx(Scene* scene, byte button, byte gfx) {
	debug("setting button %d gfx %d", (int)button, (int)gfx);
	if (scene->buttonGfx[button] != gfx) {
		scene->buttonGfx[button] = gfx;
	}
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

void MerlinEngine::swapSpriteDesc() {
	byte *data = _boltCurrentMemberEntry->dataPtr;
	uint32 decompSize = _boltCurrentMemberEntry->decompSize;
	uint32 offset = 0;
	BltSpriteDesc *ptr = reinterpret_cast<BltSpriteDesc*>(data);

	while (offset < decompSize) {
		WRITE_UINT16(&ptr->x, READ_BE_UINT16(&ptr->x));
		WRITE_UINT16(&ptr->y, READ_BE_UINT16(&ptr->y));
		resolveIt(&ptr->image.ptr);
		offset += sizeof(BltSpriteDesc);
		ptr++;
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
		ptr->rect.onLoad();
		WRITE_UINT16(&ptr->plane, READ_BE_UINT16(&ptr->plane));
		WRITE_UINT16(&ptr->gfxCount, READ_BE_UINT16(&ptr->gfxCount));
		WRITE_UINT16(&ptr->initialGfx, READ_BE_UINT16(&ptr->initialGfx));
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
		WRITE_UINT16(ptr + 0x20, READ_BE_UINT16(ptr + 0x20));
		WRITE_UINT16(ptr + 0x22, READ_BE_UINT16(ptr + 0x22));
		offset += 0x24;
		ptr += 0x24;
	}
}

} // End of namespace Merlin

} // End of namespace Bolt
