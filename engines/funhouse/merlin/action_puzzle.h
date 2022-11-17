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

#ifndef FUNHOUSE_MERLIN_ACTION_PUZZLE_H
#define FUNHOUSE_MERLIN_ACTION_PUZZLE_H

#include "funhouse/bolt.h"
#include "funhouse/merlin/merlin.h"
#include "funhouse/merlin/popup_menu.h"
#include "funhouse/boltlib/palette.h"
#include "funhouse/boltlib/boltlib.h"
#include "common/random.h"

namespace Funhouse {

struct BltActionDifficultyInfo { // type 47
	static const uint32 kType = kBltActionDifficultyInfo;
	static const uint kSize = 0xe;
	void load(Common::Span<const byte> src, Boltlib& boltlib) {
		particlesForGoal = src.getUint8At(0x0);
		particlesForLoss = src.getUint8At(0x1);
	}

	byte particlesForGoal;
	byte particlesForLoss;
};

class ActionPuzzle : public Card {
public:
	ActionPuzzle();
	void init(MerlinGame *game, Boltlib &boltlib, int challengeIdx);
	void enter() override;
	void redraw() override;
	BoltRsp handleMsg(const BoltMsg &msg) override;
	void handleReset() override;

protected:
	struct State : public ChallengeState
	{
		virtual ~State() { }

		int difficulty;
		int variation;
		int hitParticles = 0;
		int lostParticles = 0;
		int goalsEarned = 0;
	};

	struct Particle {
		int imageNum;
		int pathNum;
		int deathNum; // 0: not dying; 1+: in death sequence
		int deathProgress;
		int progress;
	};

	typedef Common::List<Particle> ParticleList;

	const BltImage& getParticleImage(const Particle &particle);
	Common::Point getParticlePos(const Particle &particle);
	BoltRsp handleClick(const Common::Point &pt);
	bool isParticleAtPoint(const Particle &particle, const Common::Point &pt);
	void launchNewParticle();
	void spawnParticle(int sprite, int path);
	void drawBack();
	void drawFore();
	void reset();
	void tick();
	BoltRsp win();

	MerlinGame *_game;
	Common::SharedPtr<State> _state;
	Timer _timer;
	BltImage _bgImage;
	BltPalette _backPalette;
	BltPalette _forePalette;
	BltColorCycles _backColorCycles;
	BltColorCycles _foreColorCycles;
	Common::Array<BltImage> _particleImages;
	typedef Common::Array<Common::Point> PointArray;
	Common::Array<PointArray> _paths;
	PointArray _goals;

	typedef Common::Array<BltImage> ImageArray;
	ImageArray _goalImages;
	static const int kNumDeathSequences = 3;
	ImageArray _deathSequences[kNumDeathSequences];
	int _tickPeriod;


	BltActionDifficultyInfo _difficultyInfo;
	ParticleList _particles;

	Common::RandomSource _random;
	uint _tickNum;
	Common::Array<int> _spriteSequence;
	int _spriteIdx = 0;
	Common::Array<int> _pathSequence;
	int _pathIdx = 0;
};

} // End of namespace Funhouse

#endif
