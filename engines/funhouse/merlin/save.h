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

namespace Funhouse
{

class MerlinGame;

static const int kProfileCount = 12;

struct ProfileData
{
	int scriptCursor = 0;
	int scriptReturnCursor = 0;
	Common::Array<int> difficulties;
	Common::Array<int> challengeStatuses;
	Common::Array<int> puzzleVariants;
};

class SaveManager
{
public:
	void init(MerlinGame *game);
	bool getProfileStatus(int idx) const;
	int getProfileIdx() const;
	void setProfileIdx(int idx);
	ProfileData &getProfile();
	void save();

private:
	MerlinGame *_game = nullptr;
	int _profileIdx = -1; // -1: No profile loaded
	bool _profileStatus[kProfileCount] = {};
	ProfileData _profiles[kProfileCount];
};

} // end of namespace Funhouse

#endif
