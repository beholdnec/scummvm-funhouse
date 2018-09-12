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
	void load(const ConstSizedDataView<kSize> src, Boltlib &bltFile) {
		pos.x = src.readInt16BE(0);
		pos.y = src.readInt16BE(2);
		imageId = BltId(src.readUint32BE(4));
	}

	Common::Point pos;
	BltId imageId;
};

typedef ScopedArray<BltSpriteElement> BltSpriteList;

void BltSprites::load(Boltlib &boltlib, BltId id) {
    BltSpriteList spriteList;
    loadBltResourceArray(spriteList, boltlib, id);

    _sprites.alloc(spriteList.size());
    _imageSet.alloc(spriteList.size());
    for (uint i = 0; i < _sprites.size(); ++i) {
        _sprites[i].pos = spriteList[i].pos;
        _imageSet[i].load(boltlib, spriteList[i].imageId);
        _sprites[i].image = &_imageSet[i];
    }
}

const Sprite& BltSprites::getSprite(uint i) const {
    return _sprites[i];
}

uint BltSprites::getNumSprites() const {
    return _sprites.size();
}

void BltSprites::setSpriteImage(uint i, BltImage *image) {
    _sprites[i].image = image;
}

} // End of namespace Funhouse
