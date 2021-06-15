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
	_lastTicksTime = _eventTime;

	_graphics.init(_system, this);
	_game->init(_system, this, _mixer);
	
	while (!shouldQuit()) {
		BoltMsg msg = getNextMsg();
		_graphics.handleMsg(msg);
		if (msg.type == BoltMsg::kYield) {
			yield();
		}
		else {
			_game->handleMsg(msg);
		}
	}

	return Common::kNoError;
}

BoltMsg FunhouseEngine::getNextMsg()
{
	if (_eventsSinceYield >= kMaxEventsSinceYield) {
		// Basic sanity measure. This should never happen.
		warning("Too many events occurred since last yield! Yielding now");
		return BoltMsg::kYield;
	}

	++_eventsSinceYield;

	if (_nextMsg.type != BoltMsg::kYield) {
		BoltMsg msg = _nextMsg;
		_nextMsg = BoltMsg::kYield;
		return msg;
	}

	if (!_ticksSent) {
		int32 ticks = _eventTime - _lastTicksTime;
		_lastTicksTime = _eventTime;
		_ticksSent = true;
		BoltMsg msg(BoltMsg::kAddTicks);
		msg.num = ticks;
		return msg;
	}

	// Find next timer to handle
	int timerId = kTimerCount;
	for (int i = 0; i < kTimerCount; ++i) {
		if (_timers[i].armed && _timers[i].ticks >= _timers[i].elapse) {
			timerId = i;
			break;
		}
	}

	if (timerId != kTimerCount) {
		debug(4, "timer %d, %d elapsed", timerId, _timers[timerId].elapse);
		BoltMsg msg(BoltMsg::kTimer);
		msg.num = timerId;
		_timers[timerId].armed = false;
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
	else if (_smoothAnimationRequested && !_smoothAnimationSent) {
		_smoothAnimationRequested = false;
		_smoothAnimationSent = true;
		return BoltMsg::kSmoothAnimation;
	}
	else if (_hoverRequested) {
		_hoverRequested = false;
		BoltMsg msg(BoltMsg::kHover);
		msg.point = getEventManager()->getMousePos();
		return msg;
	}

	return BoltMsg::kYield;
}

void FunhouseEngine::yield() {
	_graphics.presentIfDirty();
	_eventTime = getTotalPlayTime();
	_eventsSinceYield = 0;
	_ticksSent = false;
	_smoothAnimationSent = false;
}

void FunhouseEngine::win() {
    _game->win();
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

void FunhouseEngine::startTimer(int id, int32 elapse) {
	debug(4, "start timer %d, %d", id, elapse);
	Timer newTimer;
	newTimer.armed = true;
	newTimer.ticks = 0;
	newTimer.elapse = elapse;
	_timers[id] = newTimer;
}

void FunhouseEngine::armTimer(int id, int32 elapse) {
	_timers[id].armed = true;
	_timers[id].elapse = elapse;
}

void FunhouseEngine::addTicks(int id, int32 ticks) {
	_timers[id].ticks += ticks;
}

void FunhouseEngine::removeTicks(int id, int32 ticks) {
	_timers[id].ticks -= ticks;
}

int32 FunhouseEngine::getTicks(int id) const {
	return _timers[id].ticks;
}

Graphics* FunhouseEngine::getGraphics() {
	return &_graphics;
}

} // End of namespace Funhouse
