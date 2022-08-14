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

// FIXME: Add WaitEventTimeout method to event manager
#include <SDL.h>

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
		(f == kSupportsReturnToLauncher);
}

Common::Error FunhouseEngine::run() {
	assert(_game);

	_console.reset(new FunhouseConsole(this));

	_eventTime = getTotalPlayTime();
	_lastTicksTime = _eventTime;

	_graphics.init(_system, this);
	_game->init(_system, this, _mixer);
	
	while (!shouldQuit() && !_quitRequested) {
		BoltMsg msg = getNextMsg();
		debug(4, "handling msg %d", msg.type);
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

void FunhouseEngine::waitForMsg() {
	if (_nextMsg.type != BoltMsg::kYield)
		return;

	if (!_ticksSent)
		return;

	if (_nextEvent.type != Common::EVENT_INVALID)
		return;

	if (_eventMan->pollEvent(_nextEvent))
		return;

	if (_smoothAnimationRequested && !_smoothAnimationSent)
		return;

	if (_hoverRequested)
		return;

	int32 waitTicks = _wakeupTicks;
	if (_wakeupTicks != INT32_MAX) // Trim ticks that have elapsed during this frame
		waitTicks -= (getTotalPlayTime() - _eventTime);

	if (waitTicks > 0 && !_smoothAnimationRequested) {
		debug(3, "waiting for event with timeout %d ...", waitTicks);
		SDL_WaitEventTimeout(NULL, waitTicks);
	}
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
		_wakeupTicks = INT32_MAX;
		int32 ticks = _eventTime - _lastTicksTime;
		_lastTicksTime = _eventTime;
		_ticksSent = true;
		BoltMsg msg(BoltMsg::kAddTicks);
		msg.num = ticks;
		debug(4, "adding %d ticks...", ticks);
		return msg;
	}

	if (!_probeWakeupTimeSent)
	{
		debug(3, "probing wakeup time...");
		_probeWakeupTimeSent = true;
		BoltMsg msg(BoltMsg::kProbeWakeupTime);
		return msg;
	}

	_probeWakeupTimeSent = false;

	Common::Event event = _nextEvent;
	_nextEvent = Common::Event();

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
	debug(4, "yielding...");
	_graphics.presentIfDirty();
	waitForMsg();
	_eventTime = getTotalPlayTime();
	_eventsSinceYield = 0;
	_ticksSent = false;
	_probeWakeupTimeSent = false;
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

void FunhouseEngine::requestWakeup(int32 ticks) {
	debug(4, "requesting wakeup in %d ticks", ticks);
	_wakeupTicks = MIN(ticks, _wakeupTicks);
}

void FunhouseEngine::requestQuit() {
	_quitRequested = true;
}

Graphics* FunhouseEngine::getGraphics() {
	return &_graphics;
}

void ModeContext::init(FunhouseEngine* engine) {
	_engine = engine;
}

void ModeContext::react(const BoltMsg& msg) {
	bool done = false;
	bool msgSent = false;

	while (!done) {
		done = true;

		if (_nextMode) {
			done = false;

			if (_mode) {
				_mode->leave();
			}

			_mode = _nextMode;
			_nextMode = nullptr;

			if (_mode) {
				_mode->enter();
			}
		}
		else if (!msgSent) {
			done = false;
			_mode->react(msg);
			msgSent = true;
		}
	}

	// TODO: send timer probe message
}

Mode* ModeContext::getMode() {
	return _mode;
}

void ModeContext::setNextMode(Mode* nextMode) {
	_nextMode = nextMode;
}

void DynamicMode::onEnter(std::function<void()> fn) {
	_enterFn = fn;
}

void DynamicMode::onMsg(std::function<void(const BoltMsg& msg)> fn) {
	_msgFn = fn;
}

void DynamicMode::enter() {
	if (_enterFn) {
		_enterFn();
	}
}

void DynamicMode::leave() {
	// Unused
}

void DynamicMode::react(const BoltMsg& msg) {
	if (_msgFn) {
		_msgFn(msg);
	}
}

void FunhouseEngine::runTimer(const BoltMsg& msg, Timer& timer)
{
	if (msg.type == BoltMsg::kAddTicks)
	{
		timer.ticks += msg.num;
		_probeWakeupTimeSent = false;
	}
}

bool FunhouseEngine::queryTimer(const BoltMsg& msg, const Timer& timer)
{
	if (timer.ticks >= timer.elapse)
	{
		return true;
	}
	else
	{
		if (msg.type == BoltMsg::kProbeWakeupTime)
		{
			requestWakeup(timer.elapse - timer.ticks);
		}
		return false;
	}
}

void Timer::start(int32 elapse_) {
	ticks = 0;
	elapse = elapse_;
}

} // End of namespace Funhouse
