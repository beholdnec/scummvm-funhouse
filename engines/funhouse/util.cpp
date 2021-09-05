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

#include "funhouse/util.h"

#include "common/random.h"

namespace Funhouse {

enum ShuffleStage {
	// Place included numbers.
	kPlaceInitial,
	// Place excluded numbers.
	kPlaceExcluded,
	// Place all numbers.
	kPlaceAll,
};

/// Rotate a value by n within a range [0, mod).
static int rotate(int initial, int adjust, int mod) {
	// This while loop is present in the original program and appears to work around
	// some uncertainty with how the mod operator `%` behaves in C with negative numbers.
	// TODO: optimize
	while (adjust < 0) {
		adjust += mod;
	}
	return (initial + adjust) % mod;
}

void makeShuffledSequence(int count, Common::Span<int> out, int deviance, int previous, Common::Span<bool> placed) {
	Common::RandomSource random_("ShuffleRandom");

	if (deviance == 0) {
		deviance = count / 2;
	}

	int outValue;
	if (previous == -1) {
		outValue = random_.getRandomNumber(count - 1);
	} else {
		outValue = previous;
	}

	ScopedArray<bool> localPlacedArray;
	if (!placed) {
		localPlacedArray.alloc(count);
		for (int i = 0; i < count; ++i) {
			localPlacedArray[i] = false;
		}

		placed = localPlacedArray.span();
	}

	ScopedArray<bool> initialUnplaced;
	initialUnplaced.alloc(count);

	int numbersRemaining = 0;
	for (int i = 0; i < count; ++i) {
		if (!placed[i]) {
			++numbersRemaining;
		}
		initialUnplaced[i] = !placed[i];
	}

	int initialPlacedCount = count - numbersRemaining;

	int stage = kPlaceInitial;

	for (int iout = 0; iout < out.size(); ++iout) {
		while (numbersRemaining == 0) {
			if (stage == kPlaceInitial) {
				// Place all numbers that were initially excluded
				stage = kPlaceExcluded;
				numbersRemaining = initialPlacedCount;
				for (int i = 0; i < count; ++i) {
					placed[i] = initialUnplaced[i];
				}
			} else {
				// Start over; Generate shuffled sequences until output is filled; Ignore exclusions
				stage = kPlaceAll;
				numbersRemaining = count;
				for (int i = 0; i < count; ++i) {
					placed[i] = false;
				}
			}
		}

		// Generate the next random number with specified maximum deviance from the previous number.
		int direction;
		do {
			direction = random_.getRandomNumber((deviance * 2 + 1) - 1) - deviance;
		} while(abs(direction) < 2); // FIXME: Is this correct? Check ghidra.
		// ^ This will loop infinitely if count is too small.

		outValue = rotate(outValue, direction, count);

		// If the new number is already placed, search until we find one that has not been placed yet.
		direction = direction < 0 ? -1 : 1;
		while (placed[outValue]) {
			outValue = rotate(outValue, direction, count);
		}

		out[iout] = outValue;
		--numbersRemaining;
		placed[outValue] = true;
	}
}

} // end of namespace Funhouse
