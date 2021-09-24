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

#include "funhouse/boltlib/sprites.h"

namespace Funhouse {
	
struct BltSpriteElement { // type 27
	static const uint32 kType = kBltSpriteList;
	static const uint kSize = 0x8;
	void load(Common::Span<const byte> src, Boltlib &bltFile) {
		pos.x = src.getInt16BEAt(0x0);
		pos.y = src.getInt16BEAt(0x2);
		imageId = BltId(src.getUint32BEAt(0x4));
	}

	Common::Point pos;
	BltId imageId;
};

typedef Common::Array<BltSpriteElement> BltSpriteList;

SharedSpriteList loadBltSprites(Boltlib &boltlib, BltId id) {
	BltSpriteList spriteList;
	loadBltResourceArray(spriteList, boltlib, id);

	SharedSpriteList result(new Common::Array<SharedSprite>(spriteList.size()));
	for (uint i = 0; i < spriteList.size(); ++i) {
		(*result)[i].reset(new Sprite);
		(*result)[i]->pos = spriteList[i].pos;
		(*result)[i]->image.reset(new BltImage);
		(*result)[i]->image->load(boltlib, spriteList[i].imageId);
	}

	return result;
}

void drawSprites(::Graphics::Surface& surface, SharedSpriteList sprites, bool transparency, Common::Point origin) {
	for (int i = 0; i < sprites->size(); ++i) {
		if ((*sprites)[i] && (*sprites)[i]->image) {
			Common::Point pos = (*sprites)[i]->pos - origin;
			(*sprites)[i]->image->drawAt(surface, pos.x, pos.y, transparency);
		}
	}
}

} // End of namespace Funhouse
