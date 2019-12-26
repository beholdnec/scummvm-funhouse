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

#include "funhouse/boltlib/sound.h"
#include "funhouse/boltlib/boltlib.h"
#include "audio/audiostream.h"
#include "audio/decoders/raw.h"
#include "audio/mixer.h"

namespace Funhouse {

BltSound::BltSound() : _audioStream(nullptr)
{ }

// FIXME: Implement destructor; prevent memory leaks

void BltSound::load(Boltlib &boltlib, BltId id) {
	_resource.reset(boltlib.loadResource(id, kBltSound));
	_audioStream = Audio::makeRawStream(&_resource[0], _resource.size(), 22050, Audio::FLAG_UNSIGNED, DisposeAfterUse::NO);
}

void BltSound::play(Audio::Mixer *mixer) {
	if (!_audioStream) {
		return;
	}

	_audioStream->rewind();
	mixer->playStream(Audio::Mixer::kSFXSoundType, &_audioHandle, _audioStream, -1, Audio::Mixer::kMaxChannelVolume, 0, DisposeAfterUse::NO);
}

} // End of namespace Funhouse
