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

#include "funhouse/bolt.h"

#include "common/error.h"
#include "common/events.h"
#include "common/system.h"
#include "graphics/palette.h"
#include "engines/advancedDetector.h"

#include "funhouse/console.h"
#include "funhouse/merlin/merlin.h"

namespace Funhouse {

FunhouseEngine::FunhouseEngine(OSystem *const syst, const ADGameDescription *const gd) :
	Engine(syst)
{
	if (Common::String("merlin").compareTo(gd->gameId) == 0) {
		_game.reset(new MerlinGame);
	} else {
		assert(false && "FunhouseEngine does not support this game.");
	}
}

bool FunhouseEngine::hasFeature(const EngineFeature f) const {
	return
		(f == kSupportsRTL);
}

Common::Error FunhouseEngine::run() {
	assert(_game);

    _console.reset(new FunhouseConsole(this));

	_eventTime = getTotalPlayTime();
	_graphics.init(_system, this);
	_game->init(_system, this, _mixer);
	
	while (!shouldQuit()) {
		BoltMsg msg = getNextMsg();
		_graphics.handleMsg(msg);
		if (msg.type != BoltMsg::kYield) {
			_game->handleMsg(msg);
		}
		else {
			_graphics.presentIfDirty();
			_eventTime = getTotalPlayTime();
		}
	}

	return Common::kNoError;
}

BoltMsg FunhouseEngine::getNextMsg()
{
	if (_nextMsg.type != BoltMsg::kYield) {
		BoltMsg msg = _nextMsg;
		_nextMsg = BoltMsg::kYield;
		return msg;
	}

	// Find next timer to handle
	Common::List<Timer>::iterator nextTimer = _timers.end();
	uint32 nextTimerDelta = 0xFFFFFFFF;
	for (Common::List<Timer>::iterator it = _timers.begin(); it != _timers.end(); ++it) {
		uint32 delta = _eventTime - it->start;
		if (delta >= it->delay && delta < nextTimerDelta) {
			nextTimer = it;
			nextTimerDelta = delta;
		}
	}

	if (nextTimer != _timers.end()) {
		// TODO: don't modify _eventTime. If a timer event arrives late then so be it.
		_eventTime = nextTimer->start + nextTimer->delay;
		int id = nextTimer->id;
		_timers.erase(nextTimer);

		BoltMsg msg(BoltMsg::kTimer);
		msg.num = id;
		return msg;
	}

	Common::Event event;
	if (!_eventMan->pollEvent(event)) {
		event.type = Common::EVENT_INVALID;
	}

	if (event.type == Common::EVENT_KEYDOWN &&
		event.kbd.keycode == Common::KEYCODE_d &&
		(event.kbd.flags & Common::KBD_CTRL)) {
		_console->attach();
		_console->onFrame();
	}
	else if (event.type == Common::EVENT_MOUSEMOVE) {
		BoltMsg msg(BoltMsg::kHover);
		msg.point = event.mouse;
		return msg;
	}
	else if (event.type == Common::EVENT_LBUTTONDOWN) {
		BoltMsg msg(BoltMsg::kClick);
		msg.point = event.mouse;
		return msg;
	}
	else if (event.type == Common::EVENT_RBUTTONDOWN) {
		BoltMsg msg(BoltMsg::kRightClick);
		msg.point = event.mouse;
		return msg;
	}
	else if (_smoothAnimationRequested) {
		// FIXME: smooth animation events are handled rapidly and use 100% of the cpu.
		// Change this so smooth animation events are handled at a reasonable rate.
		_smoothAnimationRequested = false;
		return BoltMsg::kSmoothAnimation;
	}
	else if (_hoverRequested) {
		_hoverRequested = false;
		BoltMsg msg(BoltMsg::kHover);
		msg.point = event.mouse;
		return msg;
	}

	return BoltMsg::kYield;
}

void FunhouseEngine::win() {
    _game->win();
}

uint32 FunhouseEngine::getEventTime() const {
	return _eventTime;
}

void FunhouseEngine::setNextMsg(const BoltMsg &msg) {
	_nextMsg = msg;
}

void FunhouseEngine::requestSmoothAnimation() {
	_smoothAnimationRequested = true;
}

void FunhouseEngine::requestHover() {
	_hoverRequested = true;
}

void FunhouseEngine::setTimer(uint32 delay, int id) {
	Timer newTimer;
	newTimer.start = _eventTime;
	newTimer.delay = delay;
	newTimer.id = id;
	_timers.push_back(newTimer);
}

Graphics* FunhouseEngine::getGraphics() {
	return &_graphics;
}

} // End of namespace Funhouse
