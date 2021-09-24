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

#include "funhouse/boltlib/image.h"

#include "funhouse/graphics.h"

namespace Funhouse {

struct BltImageHeader {
	static const int kSize = 0x18;
	BltImageHeader(Common::Span<const byte> src) {
		compression = src.getUint8At(0);
		// FIXME: unknown fields at 1..6
		offset.x = src.getInt16BEAt(6);
		offset.y = src.getInt16BEAt(8);
		width = src.getUint16BEAt(0xA);
		height = src.getUint16BEAt(0xC);
		// FIXME: unknown fields at 0xE..0x18
	}

	byte compression;
	Common::Point offset;
	uint16 width;
	uint16 height;
};

void BltImage::load(Boltlib &bltFile, BltId id) {
	_res = bltFile.loadResource(id, kBltImage);
}

void BltImage::draw(::Graphics::Surface &surface, bool transparency) const {
	drawWithTopLeftAnchor(surface, 0, 0, transparency);
}

void BltImage::drawAt(::Graphics::Surface &surface, int x, int y,
					  bool transparency, DrawFlags flags) const {
	assert(!_res.empty());

	int topLeftX = x;
	int topLeftY = y;

	BltImageHeader header(spanOf(_res));
	if (!(flags & kNoOffset)) {
		topLeftX += header.offset.x;
		topLeftY += header.offset.y;
	}

	drawWithTopLeftAnchor(surface, topLeftX, topLeftY, transparency);
}

void BltImage::drawWithTopLeftAnchor(
	::Graphics::Surface &surface, int x, int y, bool transparency) const {

	assert(!_res.empty());

	BltImageHeader header(spanOf(_res));
	const byte *imageData = &_res[BltImageHeader::kSize];
	int imageDataSize = _res.size() - BltImageHeader::kSize;

	if (header.compression) {
		decodeRL7(surface, x, y, header.width, header.height,
				  imageData, imageDataSize, transparency);
	} else {
		decodeCLUT7(surface, x, y, header.width, header.height,
					imageData, imageDataSize, transparency);
	}
}

byte BltImage::query(int x, int y) const {
	BltImageHeader header(spanOf(_res));
	const byte *src = &_res[BltImageHeader::kSize];
	int srcLen = _res.size() - BltImageHeader::kSize;
	return header.compression ? queryRL7(x, y, src, srcLen, header.width, header.height) : queryCLUT7(x, y, src, srcLen, header.width, header.height);
}

Common::Rect BltImage::getRect(const Common::Point &pos, DrawFlags flags) const {
	BltImageHeader header(spanOf(_res));
	Common::Rect result(0, 0, header.width, header.height);
	if (!(flags & kNoOffset)) {
		result.translate(header.offset.x, header.offset.y);
	}
	result.translate(pos.x, pos.y);
	return result;
}

uint16 BltImage::getWidth() const {
	BltImageHeader header(spanOf(_res));
	return header.width;
}

uint16 BltImage::getHeight() const {
	BltImageHeader header(spanOf(_res));
	return header.height;
}

Common::Point BltImage::getOffset() const {
	BltImageHeader header(spanOf(_res));
	return header.offset;
}

} // End of namespace Funhouse
