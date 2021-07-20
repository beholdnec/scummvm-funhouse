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

#ifndef FUNHOUSE_BOLTLIB_BOLTLIB_H
#define FUNHOUSE_BOLTLIB_BOLTLIB_H

#include "common/file.h"
#include "common/rect.h"

#include "funhouse/util.h"

namespace Funhouse {

enum BltType {
	kBltU8Values = 1,
	kBltS16Values = 2, // signed 16-bit values
	kBltU16Values = 3, // unsigned 16-bit values
	kBltResourceList = 6,
	kBltSound = 7,
	kBltImage = 8,
	kBltPalette = 10,
	kBltColorCycles = 11,
	kBltColorCycleSlot = 12,
	kBltRect = 24,
	kBltPlane = 26, // image, palette, hotspots
	kBltSpriteList = 27, // image, x, y
	kBltColors = 28, // just some colors, referenced by palette mods
	kBltPaletteMods = 29,
	kBltButtonGraphicsList = 30,
	kBltButtonList = 31,
	kBltScene = 32,
	kBltMainMenu = 33,
	kBltFileMenu = 34,
	kBltPopup = 37,
	kBltPopupCatalog = 38,
	kBltHub = 40,
	kBltHubItem = 41,
	kBltSlidingPuzzle = 44,
	kBltParticleDeaths = 45, // action puzzles
	kBltParticles                   = 46, // action puzzles
	kBltWordPuzzleInfo              = 48,
	kBltWordPuzzleVariantInfo       = 49,
	kBltTangramPuzzleDifficultyInfo = 51,
	kBltSynchPuzzleInfo             = 52,
	kBltSynchPuzzleTransition       = 53,
	kBltMemoryPuzzleInfos           = 54,
	kBltMemoryPuzzleItemFrameList   = 55,
	kBltMemoryPuzzleItemList        = 56,
	kBltColorPuzzleTransition       = 58,
	kBltPotionPuzzle                = 59,
	kBltPotionPuzzleSpritePoints    = 60, // potion puzzles
	kBltPotionPuzzleDifficulty      = 61, // potion puzzles
	kBltPotionPuzzleComboTableList  = 62,
	kBltPotionPuzzleComboTable      = 63,
};

struct BltShortId {
	static const uint16 kInvalid = 0xFFFF;

	BltShortId() : value(kInvalid) { }
	explicit BltShortId(uint16 v) : value(v) { }

	// ID is made of two 8-bit parts: <directory number> <resource number>.
	uint16 value;
};

struct BltId {
	static const uint32 kInvalid = 0xFFFFFFFFUL;

	BltId() : value(kInvalid) { }
	BltId(BltShortId shortId) : value(shortId.value << 16) { }
	explicit BltId(uint32 v) : value(v) { }
	bool isValid() const { return value != kInvalid; }

	// ID is made of two 16-bit parts: <short id> <offset>.
	// offset should always be zero.
	uint32 value;
};

typedef ScopedArray<byte> BltResource;

class Boltlib {
public:
	bool load(const Common::String &filename);

	BltResource loadResource(BltId id, uint32 expectedType);

private:
	// Warning: may clobber file cursor
	void ensureDirLoaded(byte dirNum);

	Common::File _file;

	struct DirectoryEntry {
		DirectoryEntry() { }
		DirectoryEntry(Common::File &file);

		uint32 numResources;
		uint32 compReadSize; // Number of bytes to read for decompression
		uint32 offset; // Offset of the resource table of this directory
		// (relative to beginning of file)
	};

	struct ResourceEntry {
		ResourceEntry() { }
		ResourceEntry(Common::File &file);

		byte compression; // 0: BOLT-LZ; 8: Raw
		uint32 type;
		uint32 size; // Uncompressed size
		uint32 offset; // Offset relative to beginning of file
	};

	struct Directory {
		DirectoryEntry entry;
		Common::Array<ResourceEntry> resEntries;
	};

	Common::Array<Directory> _dirs;
};

// Common template function for loading a simple BLT resource.
template<class T>
void loadBltResource(T &obj, Boltlib &boltlib, BltId id) {
	BltResource res(boltlib.loadResource(id, T::kType));
	// Reset obj to unloaded state
	obj.~T();
	new(&obj) T();
	if (res) {
		if (res.size() != T::kSize) {
			error("Invalid size for resource type %u: %u", (uint)T::kType, (uint)res.size());
		}
		obj.load(res.span(), boltlib);
	}
}

// Common template function for loading a simple BLT resource array.
template<class T>
void loadBltResourceArray(ScopedArray<T>& array, Boltlib &boltlib, BltId id) {
	BltResource res(boltlib.loadResource(id, T::kType));
	uint numItems = res.size() / T::kSize;
	array.alloc(numItems);
	for (uint i = 0; i < numItems; ++i) {
		array[i].load(res.span().subspan(i * T::kSize), boltlib);
	}
}

struct BltU8ValueElement { // type 1
	static const uint32 kType = kBltU8Values;
	static const uint kSize = 1;
	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		value = src.getUint8At(0);
	}

	byte value;
};

typedef ScopedArray<BltU8ValueElement> BltU8Values;

struct BltS16ValueElement { // type 2
	static const uint32 kType = kBltS16Values;
	static const uint kSize = 2;
	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		value = src.getInt16BEAt(0);
	}

	int16 value;
};

typedef ScopedArray<BltS16ValueElement> BltS16Values;

struct BltU16ValueElement { // type 3
	static const uint32 kType = kBltU16Values;
	static const uint kSize = 2;
	void load(Common::Span<const byte> src, Boltlib &bltFile) {
		value = src.getUint16BEAt(0);
	}

	uint16 value;
};

typedef ScopedArray<BltU16ValueElement> BltU16Values;

struct BltResourceListElement { // type 6
	static const uint32 kType = kBltResourceList;
	static const uint kSize = 4;
	void load(Common::Span<const byte> src, Boltlib &bltFile) {
		value = BltId(src.getUint32BEAt(0));
	}

	BltId value;
};

typedef ScopedArray<BltResourceListElement> BltResourceList;

} // End of namespace Funhouse

#endif
