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

#ifndef FUNHOUSE_BOLTLIB_PALETTE_H
#define FUNHOUSE_BOLTLIB_PALETTE_H

#include "funhouse/boltlib/boltlib.h"
#include "common/ptr.h"

namespace Common {
	class Rational;
}

namespace Funhouse {

class Graphics;
	
struct BltColorCycleSlot {
	static const uint32 kType = kBltColorCycleSlot;
	static const uint kSize = 6;
	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		start = src.getUint16BEAt(0);
		end = src.getUint16BEAt(2);
		frames = src.getUint8At(4);
		plane = src.getUint8At(5);
	}

	uint16 start;
	uint16 end;
	byte frames;
	byte plane; // ???
};

struct BltColorCycles {
	static const uint32 kType = kBltColorCycles;
	static const uint kSize = 0x18;
	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		for (int i = 0; i < 4; ++i) {
			numSlots[i] = src.getUint16BEAt(i * 2); // Should be 1 or 0.
		}
		for (int i = 0; i < 4; ++i) {
			BltId slotId = BltId(src.getUint32BEAt(8 + i * 4));
			if (slotId.isValid()) {
				slots[i].reset(new BltColorCycleSlot);
				loadBltResource(*slots[i], boltlib, slotId);
			}
			else {
				slots[i].reset();
			}
		}
	}

	uint16 numSlots[4];
	Common::ScopedPtr<BltColorCycleSlot> slots[4];
};

void applyColorCycles(Graphics *graphics, int plane, const BltColorCycles *cycles);

struct BltPalette { // type 10
	static const uint32 kHeaderSize = 6;
	void load(Boltlib &boltlib, BltId id);
	BltResource data;
};

void applyPalette(Graphics *graphics, int plane, const BltPalette &palette);

struct BltPaletteModElement { // type 29
	static const uint32 kType = kBltPaletteMods;
	static const uint kSize = 6;
	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		first = src.getUint8At(0);
		num = src.getUint8At(1);
		BltId colorsId(src.getUint32BEAt(2));
		colors = boltlib.loadResource(colorsId, kBltColors);
	}

	byte first;
	byte num;
	BltResource colors;
};

typedef ScopedArray<BltPaletteModElement> BltPaletteMods;

void applyPaletteMod(Graphics *graphics, int plane, const BltPaletteMods &mod, int state);
void applyPaletteModBlended(Graphics *graphics, int plane, const BltPaletteMods &mod,
	int stateA, int stateB, Common::Rational t);

} // End of namespace Funhouse

#endif
