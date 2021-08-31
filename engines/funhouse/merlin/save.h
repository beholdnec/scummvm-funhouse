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

#ifndef FUNHOUSE_MERLIN_SAVE_H
#define FUNHOUSE_MERLIN_SAVE_H

#define FORBIDDEN_SYMBOL_ALLOW_ALL // fix #include <functional>

#include "common/array.h"
#include "common/serializer.h"
#include "common/span.h"
#include "funhouse/util.h"

namespace Common {
	class Serializer;
	class SaveFileManager;
}

namespace Funhouse
{

class MerlinGame;

static const int kProfileCount = 12;

enum ChallengeStatus {
	kNotWon = 0,
	kWon = 1,
	kPlayWinMovie = 3,
};

struct ProfileData
{
	int scriptCursor = 0;
	int scriptReturnCursor = 0;
	// Difficulty levels:
	// 0: beginner; 1: advanced; 2: expert; -1: not set
	Common::Array<byte> difficulties;
	Common::Array<byte> challengeStatuses;
	Common::Array<byte> variationSlots;
};

struct VariationInfo {
	int puzzleCount;
	int variationCount;
};

class SaveManager
{
public:
	void init(MerlinGame *game, int difficultyCount, int challengeCount, Common::Span<const VariationInfo> variationInfo);
	bool getProfileStatus(int idx) const;
	ProfileData &getProfile(int idx);
	void save();

private:
	void syncHeader(Common::Serializer &s);
	void syncProfile(Common::Serializer &s, int profile);
	void countVariationSlots(Common::Span<const VariationInfo> variationInfo, int &varsPerProfile, int &slotsPerProfile, ScopedArray<int> &slotCountForVar);
	void generateVariations(Common::Span<const VariationInfo> variationInfo);

	MerlinGame *_game = nullptr;
	int _difficultyCount = 0;
	int _challengeCount = 0;
	int _variationSlotCount = 0;

	byte _profileStatus[kProfileCount] = {};
	ProfileData _profiles[kProfileCount];
};

} // end of namespace Funhouse

#endif
