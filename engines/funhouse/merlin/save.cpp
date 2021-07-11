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

namespace Funhouse {

void SaveManager::init(MerlinGame *game) {
    _game = game;
}

bool SaveManager::getProfileStatus(int idx) const {
    return _profileStatus[idx];
}

int SaveManager::getProfileIdx() const {
    return _profileIdx;
}

void SaveManager::setProfileIdx(int idx) {
    _profileIdx = idx;
}

ProfileData& SaveManager::getProfile() {
    assert(_profileIdx >= 0 && _profileIdx < kProfileCount);

    if (!_profileStatus[_profileIdx]) {
        // Create a new profile
        _profiles[_profileIdx] = {};
        _profiles[_profileIdx].scriptCursor = MerlinGame::kNewGameScriptCursor;
        _profiles[_profileIdx].scriptReturnCursor = MerlinGame::kNewGameScriptCursor;

        _profileStatus[_profileIdx] = true;
    }

    return _profiles[_profileIdx];
}

void SaveManager::save() {
    // TODO
    assert(_profileIdx >= 0 && _profileIdx < kProfileCount);

    getProfile().scriptCursor = _game->_scriptCursor;
    getProfile().scriptReturnCursor = _game->_scriptReturnCursor;

    warning("Save not implemented");
}

} // end of namespace Funhouse
