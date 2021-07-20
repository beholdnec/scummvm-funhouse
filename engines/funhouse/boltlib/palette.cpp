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

#include "funhouse/boltlib/palette.h"

#include "funhouse/graphics.h"

namespace Funhouse {

void applyColorCycles(Graphics *graphics, int plane, const BltColorCycles *cycles) {
	graphics->resetColorCycles();
	if (cycles) {
		for (int i = 0; i < 4; ++i) {
			BltColorCycleSlot *slot = cycles->slots[i].get();
			if (cycles->numSlots[i] == 1 && slot) {
				if (slot->frames <= 0) {
					warning("Invalid color cycle frames");
				}
				else {
					if (slot->plane != 0) {
						warning("Color cycle plane was not 0");
					}
					graphics->setColorCycle(i, plane, slot->start, slot->end,
						slot->frames * 1000 / 60);
				}
			}
		}
	}
}

struct BltPaletteHeader {
	static const uint32 kSize = 6;
	BltPaletteHeader(Common::Span<const byte> src) {
		first = src.getUint16BEAt(2);
		last = src.getUint16BEAt(4);
	}

	uint16 first; // first color index (usually 0)
	uint16 last; // last color index (usually 127)
};

void BltPalette::load(Boltlib &boltlib, BltId id) {
	data = boltlib.loadResource(id, kBltPalette);
}

void applyPalette(Graphics *graphics, int plane, const BltPalette &palette) {
	if (palette.data) {
		BltPaletteHeader header(palette.data.span());

		int count = header.last - header.first + 1;
		if (count > 128) {
			count = 128;
		}
		else if (count < 0) {
			count = 0;
		}

		// FIXME: are the planes backwards?
		if (plane == 0) {
			graphics->setPlanePalette(kFore, &palette.data[BltPaletteHeader::kSize + header.first * 3], header.first, count);
		}
		else { // plane == 1
			graphics->setPlanePalette(kBack, &palette.data[BltPaletteHeader::kSize + header.first * 3], header.first, count);
		}
	}
}

void applyPaletteMod(Graphics *graphics, int plane, const BltPaletteMods &mod, int state) {
	graphics->setPlanePalette(plane, &mod[state].colors[0], mod[state].first, mod[state].num);
}

static int lerp(int a, int b, Common::Rational t) {
	return a + ((b - a) * t).toInt();
}

void applyPaletteModBlended(Graphics *graphics, int plane, const BltPaletteMods &mod,
	int stateA, int stateB, Common::Rational t) {
	if (mod[stateA].first != mod[stateB].first ||
		mod[stateA].num != mod[stateB].num) {
		warning("Mismatched ranges in palette morph");
		return;
	}

	int first = mod[stateA].first;
	int num = mod[stateA].num;
	if (first + num > 128) {
		warning("Invalid range in palette morph");
		return;
	}

	if (t <= 0) {
		applyPaletteMod(graphics, plane, mod, stateA);
	}
	else if (t >= 1) {
		applyPaletteMod(graphics, plane, mod, stateB);
	}
	else {
		byte morphed[128 * 3];
		for (int i = 0; i < num * 3; ++i) {
			byte a = mod[stateA].colors[i];
			byte b = mod[stateB].colors[i];
			morphed[i] = lerp(a, b, t);
		}

		graphics->setPlanePalette(plane, morphed, first, num);
	}
}

} // End of namespace Funhouse
