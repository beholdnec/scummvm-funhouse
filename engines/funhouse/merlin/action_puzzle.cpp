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

#include "funhouse/merlin/action_puzzle.h"

namespace Funhouse {

ActionPuzzle::ActionPuzzle() : _random("ActionPuzzleRandomSource")
{ }

struct BltParticleDeaths { // type 45
	static const uint32 kType = kBltParticleDeaths;
	static const uint kSize = 0x12;
	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		numImages[0] = src.getUint16BEAt(0);
		imagesListId[0] = BltId(src.getUint32BEAt(2));
		numImages[1] = src.getUint16BEAt(6);
		imagesListId[1] = BltId(src.getUint32BEAt(8));
		numImages[2] = src.getUint16BEAt(0xC);
		imagesListId[2] = BltId(src.getUint32BEAt(0xE));
	}

	uint16 numImages[3];
	BltId imagesListId[3];
};

struct BltParticles { // type 46
	static const uint32 kType = kBltParticles;
	static const uint kSize = 2;
	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		numParticles = src.getUint16BEAt(0);
	}

	uint16 numParticles;
};

void ActionPuzzle::init(MerlinGame *game, Boltlib &boltlib, int challengeIdx) {
    _game = game;
	_mode.init(_game->getEngine());

	uint16 resId = 0;
	switch (challengeIdx) {
	case 2: resId = 0x4921; break;
	case 4: resId = 0x4D19; break;
	case 9: resId = 0x5113; break;
	case 14: resId = 0x551C; break;
	case 21: resId = 0x5918; break;
	case 27: resId = 0x5D17; break;
	default: assert(false); break;
	}

	_game->setPopup(MerlinGame::kPuzzlePopup);

	BltResourceList resourceList;
	loadBltResourceArray(resourceList, boltlib, BltShortId(resId));
	BltId difficultiesId = resourceList[0].value;
	BltId particlesId = resourceList[1].value;
	BltId bgImageId = resourceList[2].value;
	BltId backPaletteId = resourceList[3].value;
	BltId particleImagesId = resourceList[4].value;

	BltParticles particles;
	loadBltResource(particles, boltlib, particlesId);
	_particleImages.alloc(particles.numParticles);
	BltResourceList particleImagesList;
	loadBltResourceArray(particleImagesList, boltlib, particleImagesId);
	for (uint16 i = 0; i < particles.numParticles; ++i) {
		_particleImages[i].load(boltlib, particleImagesList[i].value);
	}

	_bgImage.load(boltlib, bgImageId);
	_backPalette.load(boltlib, backPaletteId);

	BltU16Values difficultiesList;
	loadBltResourceArray(difficultiesList, boltlib, difficultiesId);
	BltResourceList difficulty;
	loadBltResourceArray(difficulty, boltlib, BltShortId(difficultiesList[_game->getDifficulty(kActionDifficulty)].value));
	BltId forePaletteId = difficulty[1].value;
	BltId backColorCyclesId = difficulty[2].value;
	BltId foreColorCyclesId = difficulty[3].value;
	BltId pathListId = difficulty[4].value;
	BltId goalsId = difficulty[5].value;
	BltId goalImagesListId = difficulty[6].value;
	BltId particleDeathsId = difficulty[8].value;

	_forePalette.load(boltlib, forePaletteId);
	loadBltResource(_backColorCycles, boltlib, backColorCyclesId);
	loadBltResource(_foreColorCycles, boltlib, foreColorCyclesId);

	BltResourceList pathList;
	loadBltResourceArray(pathList, boltlib, pathListId);
	_paths.alloc(pathList.size());
	for (uint i = 0; i < pathList.size(); ++i) {
		BltS16Values pathValues;
		loadBltResourceArray(pathValues, boltlib, pathList[i].value);
		int16 numPoints = pathValues[0].value;
		if (numPoints != (int)(pathValues.size() - 1) / 2) {
			warning("Invalid particle path, specified wrong number of points");
		}
		else {
			_paths[i].alloc(numPoints);
			for (int16 j = 0; j < numPoints; ++j) {
				_paths[i][j].x = pathValues[1 + 2 * j].value;
				_paths[i][j].y = pathValues[1 + 2 * j + 1].value;
			}
		}
	}

	BltS16Values goalsValues;
	loadBltResourceArray(goalsValues, boltlib, goalsId);
	_goals.alloc(goalsValues.size() / 2);
	for (uint i = 0; i < _goals.size(); ++i) {
		_goals[i].x = goalsValues[2 * i].value;
		_goals[i].y = goalsValues[2 * i + 1].value;
	}

	BltResourceList goalImagesList;
	loadBltResourceArray(goalImagesList, boltlib, goalImagesListId);
	_goalImages.alloc(goalImagesList.size());
	for (uint i = 0; i < goalImagesList.size(); ++i) {
		_goalImages[i].load(boltlib, goalImagesList[i].value);
	}

	BltParticleDeaths particleDeaths;
	loadBltResource(particleDeaths, boltlib, particleDeathsId);
	for (int i = 0; i < kNumDeathSequences; ++i) {
		_deathSequences[i].alloc(particleDeaths.numImages[i]);
		BltResourceList imageList;
		loadBltResourceArray(imageList, boltlib, particleDeaths.imagesListId[i]);
		for (int j = 0; j < particleDeaths.numImages[i]; ++j) {
			_deathSequences[i][j].load(boltlib, imageList[j].value);
		}
	}
}

void ActionPuzzle::enter() {
	_tickNum = 0;
	// TODO: Load progress from save data
	// (check original to see if action puzzles are saved)
	// (and what happens when you change difficulty mid-puzzle?)
	_goalNum = 0;

	// XXX: spawn particles on all paths
	// TODO: don't.
	for (uint i = 0; i < _paths.size(); ++i) {
		spawnParticle(i % _particleImages.size(), i);
	}

	redraw();

	playMode();
}

void ActionPuzzle::redraw() {
	applyPalette(_game->getGraphics(), kBack, _backPalette);
	applyColorCycles(_game->getGraphics(), kBack, &_backColorCycles);
	applyPalette(_game->getGraphics(), kFore, _forePalette);
	// TODO: fore color cycles

	drawBack();
	drawFore();

	_game->getGraphics()->markDirty();
}

BoltRsp ActionPuzzle::handleMsg(const BoltMsg &msg) {
	_mode.react(msg);
	return kDone;
}

void ActionPuzzle::playMode() {
	_mode.transition();
	_mode.onEnter([=]() {
		_timer.start(kTickPeriod, true);
	});
	_mode.onMsg([this](const BoltMsg &msg) {
		if (msg.type == BoltMsg::kPopupButtonClick) {
			handlePopupButtonClick(msg.num);
			return;
		}

		_game->handlePopup(msg);
		if (_game->getPopup().isActive()) {
			_timer.active = false;
			return;
		}
		else {
			_timer.active = true;
		}

		switch (msg.type) {
		case BoltMsg::kClick:
			handleClick(msg.point);
		}
	});
	_mode.onTimer(&_timer, [this]() {
		_timer.ticks -= kTickPeriod;

		tick();
		if (_goalNum >= _goals.size()) {
			win();
		}
	});
}

BoltRsp ActionPuzzle::handlePopupButtonClick(int num) {
	switch (num) {
	case 0: // Return
        _game->branchReturn();
		return kDone;
	case 1: // Difficulty
		_game->branchDifficultyMenu();
		return kDone;
	default:
		warning("Unhandled popup button %d", num);
		return kDone;
	}
}

const BltImage& ActionPuzzle::getParticleImage(const Particle &particle) {
	if (particle.deathNum > 0) {
		const ImageArray &deathSequence = _deathSequences[particle.deathNum - 1];
		return deathSequence[particle.deathProgress];
	}
	else {
		return _particleImages[particle.imageNum];
	}
}

Common::Point ActionPuzzle::getParticlePos(const Particle &particle) {
	return _paths[particle.pathNum][particle.progress];
}

BoltRsp ActionPuzzle::handleClick(const Common::Point &pt) {
	for (ParticleList::iterator it = _particles.begin(); it != _particles.end(); ++it) {
		if (isParticleAtPoint(*it, pt)) {
			// Kill particle
			it->deathNum = _random.getRandomNumberRng(1, 3);
		}
	}

	return BoltRsp::kDone;
}

bool ActionPuzzle::isParticleAtPoint(const Particle &particle, const Common::Point &pt) {
	// Only consider particles that are not dying
	if (particle.deathNum == 0) {
		Common::Rect rect = getParticleImage(particle).getRect(getParticlePos(particle));
		return rect.contains(pt);
	}

	return false;
}

void ActionPuzzle::spawnParticle(int imageNum, int pathNum) {
	Particle particle;
	particle.imageNum = imageNum;
	particle.pathNum = pathNum;
	particle.deathNum = 0;
	particle.deathProgress = 0;
	particle.progress = 0;
	_particles.push_back(particle);
}

void ActionPuzzle::drawBack() {
	_bgImage.drawAt(_game->getGraphics()->getPlaneSurface(kBack), 0, 0, false);
	for (uint i = 0; i < _goals.size(); ++i) {
		if (i < _goalNum) {
			const Common::Point &pt = _goals[i];
			// TODO: there may be multiple sets of goals
			// (player has to complete one set and then the next)
			_goalImages[0].drawAt(_game->getGraphics()->getPlaneSurface(kBack), pt.x, pt.y, true);
		}
	}
}

void ActionPuzzle::drawFore() {
	_game->getGraphics()->clearPlane(kFore);

	for (ParticleList::const_iterator it = _particles.begin(); it != _particles.end(); ++it) {
		const Particle &p = *it;
		const BltImage &image = getParticleImage(p);
		Common::Point pt = getParticlePos(p);
		// FIXME: positions of particles in death sequence are wrong
		image.drawAt(_game->getGraphics()->getPlaneSurface(kFore), pt.x, pt.y, true);
	}
}

void ActionPuzzle::tick() {
	++_tickNum;

	for (ParticleList::iterator it = _particles.begin(); it != _particles.end();) {
		Particle &p = *it;
		++p.progress;

		if (p.deathNum > 0) {
			++p.deathProgress;
		}

		bool despawn = (uint)p.progress >= _paths[p.pathNum].size();
		if (p.deathNum > 0) {
			const ImageArray &deathSequence = _deathSequences[p.deathNum - 1];
			despawn |= (uint)p.deathProgress >= deathSequence.size();
		}

		if (despawn) {
			it = _particles.erase(it);
		}
		else {
			++it;
		}
	}

	// Spawn a new particle every 20 ticks
	static const int kNewParticleTicks = 20;
	if (_tickNum % kNewParticleTicks == 0) {
		spawnParticle(_random.getRandomNumber(_particleImages.size()-1),
			_random.getRandomNumber(_paths.size()-1));
	}

	// Award new goal every 100 ticks
	// TODO: implement game
	static const int kGoalTicks = 100;
	if (_tickNum % kGoalTicks == 0) {
		if (_goalNum < _goals.size()) {
			++_goalNum;
			drawBack();
		}
	}

	drawFore();
	_game->getGraphics()->markDirty();
}

BoltRsp ActionPuzzle::win() {
	// Redraw background before starting win movie
	_bgImage.drawAt(_game->getGraphics()->getPlaneSurface(kBack), 0, 0, false);
	_game->getGraphics()->clearPlane(kFore);
    _game->branchWin();
	return BoltRsp::kDone;
}

} // End of namespace Funhouse
