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
			displayColors(backPalette, 0, 1);
			const byte *backImage = getResolvedPtr(backPlane, 0x0);
			displayPic(backImage, 0, 0, 1);
		}
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
		resolveIt((uint32 *)(ptr + 0x1c));
		offset += 0x24;
		ptr += 0x24;
	}
}

} // End of namespace Merlin

} // End of namespace Bolt
