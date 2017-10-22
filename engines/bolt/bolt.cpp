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

#include "bolt/bolt.h"

#include "common/error.h"
#include "common/events.h"
#include "common/system.h"
#include "graphics/palette.h"
#include "engines/advancedDetector.h"

#include "bolt/merlin/merlin.h"

namespace Bolt {

BoltEngine::BoltEngine(OSystem *const syst, const ADGameDescription *const gd) :
	Engine(syst)
{
	if (Common::String("merlin").compareTo(gd->gameId) == 0) {
		_game.reset(new MerlinGame);
	} else {
		assert(false && "BoltEngine does not support this game.");
	}
}

bool BoltEngine::hasFeature(const EngineFeature f) const {
	return
		(f == kSupportsRTL);
}

Common::Error BoltEngine::run() {
	assert(_game);

	_eventTime = getTotalPlayTime();
	_graphics.init(_system, this);
	_game->init(_system, &_graphics, _mixer, this);
	
	while (!shouldQuit()) {
		_eventTime = getTotalPlayTime();

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
			_eventTime = nextTimer->start + nextTimer->delay;
			int id = nextTimer->id;
			_timers.erase(nextTimer);

			BoltMsg msg(BoltMsg::kTimer);
			msg.num = id;
			topLevelHandleMsg(msg);
		} else {
			Common::Event event;
			if (!_eventMan->pollEvent(event)) {
				event.type = Common::EVENT_INVALID;
			}

			if (event.type == Common::EVENT_MOUSEMOVE) {
				BoltMsg msg(BoltMsg::kHover);
				msg.point = event.mouse;
				topLevelHandleMsg(msg);
			} else if (event.type == Common::EVENT_LBUTTONDOWN) {
				BoltMsg msg(BoltMsg::kClick);
				msg.point = event.mouse;
				topLevelHandleMsg(msg);
			} else if (event.type == Common::EVENT_RBUTTONDOWN) {
				BoltMsg msg(BoltMsg::kRightClick);
				msg.point = event.mouse;
				topLevelHandleMsg(msg);
			} else if (_smoothAnimationRequested) {
				// FIXME: smooth animation events are handled rapidly and use 100% of the cpu.
				// Change this so smooth animation events are handled at a reasonable rate.
				// FIXME: Prevent smooth animation from starving out other events such as kDrive!
				_smoothAnimationRequested = false;
				topLevelHandleMsg(BoltMsg::kSmoothAnimation);
			} else {
				// Emit Drive event
				// TODO: Eliminate Drive events in favor of Timers, SmoothAnimation and AudioEnded.
				// Generally, events signify things that are reacted to instead of polled.
				topLevelHandleMsg(BoltMsg::kDrive);
			}
		}
	}

	return Common::kNoError;
}

uint32 BoltEngine::getEventTime() const {
	return _eventTime;
}

void BoltEngine::setMsg(const BoltMsg &msg) {
	_curMsg = msg;
}

void BoltEngine::requestSmoothAnimation() {
	_smoothAnimationRequested = true;
}

void BoltEngine::setTimer(uint32 delay, int id) {
	Timer newTimer;
	newTimer.start = _eventTime;
	newTimer.delay = delay;
	newTimer.id = id;
	_timers.push_back(newTimer);
}

void BoltEngine::topLevelHandleMsg(const BoltMsg &msg) {
	_curMsg = msg;

	_graphics.handleMsg(_curMsg);

	bool yield = false;
	while (!yield) {
		// Make a copy of the current message to prevent interference if the handler calls setMsg.
		BoltMsg msgCopy = _curMsg;
		BoltCmd cmd = _game->handleMsg(msgCopy);
		switch (cmd.type) {
		case BoltCmd::kDone:
			yield = true;
			break;
		case BoltCmd::kResend:
			break;
		default:
			assert(false && "Invalid BoltCmd"); // Unreachable
			yield = true;
			break;
		}
	}

	_graphics.presentIfDirty();
}

} // End of namespace Bolt
