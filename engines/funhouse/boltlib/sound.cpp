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

BltSound::BltSound() : _audioStream(nullptr), _mixer(nullptr)
{ }

BltSound::~BltSound() {
	if (_mixer) {
		_mixer->stopHandle(_audioHandle); // Mixer deletes the audio stream (FIXME: really?)
		_audioStream = nullptr;
	}
	else {
		delete _audioStream;
		_audioStream = nullptr;
	}
}

void BltSound::load(Boltlib &boltlib, BltId id) {
	_resource.reset(boltlib.loadResource(id, kBltSound));
	_audioStream = Audio::makeRawStream(&_resource[0], _resource.size(), 22050, Audio::FLAG_UNSIGNED, DisposeAfterUse::NO);
}

void BltSound::play(Audio::Mixer *mixer) {
	if (!_audioStream) {
		return;
	}

	_mixer = mixer;
	_audioStream->rewind();
	mixer->playStream(Audio::Mixer::kSFXSoundType, &_audioHandle, _audioStream, -1, Audio::Mixer::kMaxChannelVolume, 0, DisposeAfterUse::NO);
}

BltSoundList::BltSoundList() : _random("SoundRandomSource")
{ }

void BltSoundList::load(Boltlib &boltlib, BltId id) {
	BltU16Values soundIds;
	loadBltResourceArray(soundIds, boltlib, id);
	_sounds.alloc(soundIds.size());
	for (int i = 0; i < _sounds.size(); ++i) {
		_sounds[i].load(boltlib, BltShortId(soundIds[i].value));
	}
}

void BltSoundList::play(Audio::Mixer *mixer) {
	int num = _random.getRandomNumber(_sounds.size() - 1);
	_sounds[num].play(mixer);
}

} // End of namespace Funhouse
