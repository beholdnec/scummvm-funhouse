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

#ifndef FUNHOUSE_BOLTLIB_SOUND_H
#define FUNHOUSE_BOLTLIB_SOUND_H

#include "funhouse/boltlib/boltlib.h"
#include "audio/mixer.h"
#include "common/random.h"

namespace Audio {
class SeekableAudioStream;
class Mixer;
}

namespace Funhouse {

class BltSound {
public:
	BltSound();
	~BltSound();
	void load(Boltlib &boltlib, BltId id);
	void play(Audio::Mixer *mixer);

private:
	Audio::SeekableAudioStream *_audioStream;
	Audio::SoundHandle _audioHandle;
	BltResource _resource;
	Audio::Mixer *_mixer;
};

class BltSoundList {
public:
	BltSoundList();
	void load(Boltlib &boltlib, BltId id);
	void play(Audio::Mixer *mixer);

private:
	ScopedArray<BltSound> _sounds;
	Common::RandomSource _random; // FIXME: use the game's random source
};

} // End of namespace Funhouse

#endif
