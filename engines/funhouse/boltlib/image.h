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

#ifndef FUNHOUSE_BOLTLIB_IMAGE_H
#define FUNHOUSE_BOLTLIB_IMAGE_H

#include "funhouse/boltlib/boltlib.h"

namespace Funhouse {

enum DrawFlags {
	kNone = 0,
	kNoOffset = 0x1,
};

class BltImage { // type 8
public:
	operator bool() const {
		return !_res.empty();
	}

	void load(Boltlib &bltFile, BltId id);

	void draw(::Graphics::Surface &surface, bool transparency) const;
	void drawAt(::Graphics::Surface &surface, int x, int y, bool transparency, DrawFlags flags = kNone) const;
	byte query(int x, int y) const;

	Common::Rect getRect(const Common::Point &pos = Common::Point(0, 0), DrawFlags flags = kNone) const;
	uint16 getWidth() const;
	uint16 getHeight() const;
	Common::Point getOffset() const;

private:
	void drawWithTopLeftAnchor(::Graphics::Surface &surface, int x, int y, bool transparency) const;

	BltResource _res;
};

typedef Common::SharedPtr<BltImage> SharedImage;

} // End of namespace Funhouse

#endif
