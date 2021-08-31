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

#include "funhouse/merlin/save.h"
#include "funhouse/merlin/merlin.h"
#include "common/savefile.h"

namespace Funhouse {

const char *kSaveFileName = "merlin.sav";

void SaveManager::init(MerlinGame *game, int difficultyCount, int challengeCount, Common::Span<const VariationInfo> variationInfo) {
	_game = game;
	_difficultyCount = difficultyCount;
	_challengeCount = challengeCount;

	int varsPerProfile;
	int slotsPerProfile;
	ScopedArray<int> slotCountForVar;
	countVariationSlots(variationInfo, varsPerProfile, slotsPerProfile, slotCountForVar);
	_variationSlotCount = slotsPerProfile;

	// Initialize save data buffer
	for (int i = 0; i < kProfileCount; ++i) {
		_profileStatus[i] = 0;
		_profiles[i].scriptCursor = MerlinGame::kNewGameScriptCursor;
		_profiles[i].scriptReturnCursor = MerlinGame::kNewGameScriptCursor;
		_profiles[i].difficulties.resize(_difficultyCount);
		Common::fill(_profiles->difficulties.begin(), _profiles->difficulties.end(), -1);
		_profiles[i].challengeStatuses.resize(_challengeCount);
		Common::fill(_profiles->challengeStatuses.begin(), _profiles->challengeStatuses.end(), kNotWon);
		_profiles[i].variationSlots.resize(_variationSlotCount);
		Common::fill(_profiles->variationSlots.begin(), _profiles->variationSlots.end(), 0);
	}

	Common::InSaveFile *loadFile = game->getEngine()->getSaveFileManager()->openForLoading(kSaveFileName);
	if (!loadFile) {
		// Create new save file
		generateVariations(variationInfo);
		save();
	}
	else {
		Common::Serializer serializer = Common::Serializer(loadFile, nullptr);

		syncHeader(serializer);
		for (int i = 0; i < kProfileCount; ++i) {
			syncProfile(serializer, i);
		}
	}

}

bool SaveManager::getProfileStatus(int idx) const {
	return _profileStatus[idx];
}

ProfileData& SaveManager::getProfile(int idx) {
	assert(idx >= 0 && idx < kProfileCount);

	if (!_profileStatus[idx]) {
		// Reset profile
		_profiles[idx].scriptCursor = MerlinGame::kNewGameScriptCursor;
		_profiles[idx].scriptReturnCursor = MerlinGame::kNewGameScriptCursor;
		_profileStatus[idx] = 1;
	}

	return _profiles[idx];
}

void SaveManager::save() {
	Common::OutSaveFile *saveFile = _game->getEngine()->getSaveFileManager()->openForSaving(kSaveFileName);
	if (!saveFile) {
		return; // TODO: return error
	}

	debug(0, "Saving game to merlin.sav");

	Common::Serializer serializer = Common::Serializer(nullptr, saveFile);

	syncHeader(serializer);

	for (int profile = 0; profile < kProfileCount; ++profile) {
		_profiles[profile].scriptCursor = _game->_scriptCursor;
		_profiles[profile].scriptReturnCursor = _game->_scriptReturnCursor;

		syncProfile(serializer, profile);
	}

	saveFile->finalize();
}

void SaveManager::syncHeader(Common::Serializer& s) {
	for (int i = 0; i < kProfileCount; ++i) {
		s.syncAsByte(_profileStatus[i]);
	}
}

void SaveManager::syncProfile(Common::Serializer& s, int profileIdx) {
	ProfileData &profile = _profiles[profileIdx];

	// FIXME: Use bitpacking as in the original game. Or would that be overkill?
	s.syncAsUint16BE(profile.scriptCursor);
	s.syncAsUint16BE(profile.scriptReturnCursor);
	s.syncBytes(profile.difficulties.data(), profile.difficulties.size());
	s.syncBytes(profile.challengeStatuses.data(), profile.challengeStatuses.size());
	s.syncBytes(profile.variationSlots.data(), profile.variationSlots.size());
}

static const int kBitsPerSlot = 2;
static const int kValuesPerSlot = 1 << kBitsPerSlot; // Each slot is 2 bits

void SaveManager::countVariationSlots(Common::Span<const VariationInfo> variationInfo, int &varsPerProfile, int &slotsPerProfile, ScopedArray<int> &slotCountForVar) {
	varsPerProfile = 0; // A var tells which variation of a puzzle to load
	for (int i = 0; i < variationInfo.size(); ++i) {
		varsPerProfile += variationInfo[i].puzzleCount;
	}

	slotsPerProfile = 0; // Sometimes, a var can be spread across two slots
	slotCountForVar.alloc(varsPerProfile);
	int iout = 0;
	for (int i = 0; i < variationInfo.size(); ++i) {
		int j = 1;
		int slotsPerPuzzle = 0;
		while (j < variationInfo[i].variationCount) {
			++slotsPerPuzzle;
			j *= kValuesPerSlot;
		}

		debug(3, "slots per puzzle: %d", slotsPerPuzzle);
		slotsPerProfile += variationInfo[i].puzzleCount * slotsPerPuzzle;
		for (int k = 0; k < variationInfo[i].puzzleCount; ++k) {
			slotCountForVar[iout] = slotsPerPuzzle;
			++iout;
		}
	}

	debug(3, "slot count for each var:");
	for (int i = 0; i < slotCountForVar.size(); ++i) {
		debugN(3, "%d,", slotCountForVar[i]);
	}
	debug(3, "");
}

void SaveManager::generateVariations(Common::Span<const VariationInfo> variationInfo) {
	int varsPerProfile = 0;
	int slotsPerProfile = 0;
	ScopedArray<int> slotCountForVar;
	countVariationSlots(variationInfo, varsPerProfile, slotsPerProfile, slotCountForVar);

	ScopedArray<ScopedArray<int> > allVars;
	allVars.alloc(kProfileCount);
	for (int i = 0; i < kProfileCount; ++i) {
		allVars[i].alloc(varsPerProfile);
	}

	ScopedArray<int> varSet;
	varSet.alloc(kProfileCount);

	// Generate all variations
	// FIXME: variations don't seem to be very random...
	int iout = 0;
	for (int i = 0; i < variationInfo.size(); ++i) {
		for (int j = 0; j < variationInfo[i].puzzleCount; ++j) {
			makeShuffledSequence(variationInfo[i].variationCount, varSet.span());
			debugN(3, "sequence set %d, puzzle %d: ", i, j);
			for (int k = 0; k < kProfileCount; ++k) {
				debugN(3, "%d,", varSet[k]);
				allVars[k][iout] = varSet[k];
			}
			debug(3, "");
			++iout;
		}
	}

	// Assign variations to profiles
	for (int profile = 0; profile < kProfileCount; ++profile) {
		_profiles[profile].variationSlots.resize(slotsPerProfile);

		debugN(3, "vars for profile %d: ", profile);
		for (int j = 0; j < varsPerProfile; ++j) {
			debugN(3, "%d,", allVars[profile][j]);
		}
		debug(3, "");

		iout = 0;
		for (int j = 0; j < varsPerProfile; ++j) {
			int var = allVars[profile][j];
			for (int m = slotCountForVar[j] - 1; m >= 0; --m) {
				int slotValue = (var >> (kBitsPerSlot * m)) & (kValuesPerSlot - 1);
				_profiles[profile].variationSlots[iout] = slotValue;
				++iout;
			}
		}

		debugN(3, "slots for profile %d: ", profile);
		for (int j = 0; j < slotsPerProfile; ++j) {
			debugN(3, "%d,", _profiles[profile].variationSlots[j]);
		}
		debug(3, "");
	}
}

} // end of namespace Funhouse
