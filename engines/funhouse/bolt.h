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

#ifndef FUNHOUSE_BOLT_H
#define FUNHOUSE_BOLT_H

#include "common/rect.h"

#include "engines/engine.h"

#include "funhouse/boltlib/boltlib.h"
#include "funhouse/graphics.h"
#include "funhouse/pf_file.h"
#include "funhouse/util.h"

struct ADGameDescription;

namespace Common {
struct Event;
}

namespace Funhouse {

class FunhouseConsole;

// A Bolt::Rect differs from a Common::Rect in the following ways:
// - Attributes are stored in left, right, top, bottom order
// - All edges are inclusive, i.e. right and bottom are included in the
//   rectangle
struct Rect {
	static const uint kSize = 8;
	Rect() : left(0), right(0), top(0), bottom(0) { }
	Rect(int16 l, int16 t, int16 r, int16 b)
		: left(l), right(r), top(t), bottom(b) { }
	Rect(Common::Span<const byte> src) {
		left = src.getInt16BEAt(0);
		right = src.getInt16BEAt(2);
		top = src.getInt16BEAt(4);
		bottom = src.getInt16BEAt(6);
	}

	operator Common::Rect() const {
		return Common::Rect(left, top, right + 1, bottom + 1);
	}

	bool contains(const Common::Point &p) const {
		return p.x >= left && p.x <= right && p.y >= top && p.y <= bottom;
	}

	void translate(int16 x, int16 y) {
		left += x;
		right += x;
		top += y;
		bottom += y;
	}

	int16 left;
	int16 right;
	int16 top;
	int16 bottom;
};

// The BOLT engine has an event-loop based architecture. Unlike other engines in ScummVM, where the
// game ticks at a constant rate and input is polled at each tick, BOLT is designed to handle
// messages at the rate they are received. This allows BOLT to have a high frame rate and short
// response times.
//
// FIXME:
// Unfortunately, ScummVM was not designed with this kind of event loop in mind, so the game
// typically pegs the CPU and wastes power. In the future, ScummVM should have the ability to save
// power when no messages are being received.

// Messages that the engine sends to the game.
struct BoltMsg {
	enum Type {
		// System messages (>= 0)
		kYield = 0, // Present a new frame and gather more input
		kDrive, // Run message handlers with no particular event
		kHover,
		kClick,
		kRightClick,
		kTimer,
		kAudioEnded, // TODO: implement
		kSmoothAnimation,
		kPopupButtonClick,
        kSceneMsgs = 100,
	};

	BoltMsg(int type_ = kYield) : type(type_) { }

	int type;
	// The exact time at which a timer elapsed. Only meaningful for kTimer messages.
	// Use getEventTime() to get the time at which the event was sent, which may be
	// slightly later than the timer.
	uint32 timerTime = 0;
	int num = 0;
	Common::Point point;
};

// Responses to a message.
enum BoltRsp {
    kDone, // Message was handled.
    kPass, // Message was not handled and should be passed to the next handler.
};

class Card {
public:
	virtual ~Card() { }
	virtual void enter() = 0;
    virtual void redraw() {
        enter();
    }
	virtual BoltRsp handleMsg(const BoltMsg &msg) = 0;
};

enum TimerId {
	kMovieTimer,
	kCardTimer
};

class FunhouseEngine;

class FunhouseGame {
public:
	virtual ~FunhouseGame() { }
	virtual void init(OSystem *system, FunhouseEngine *engine, Audio::Mixer *mixer) = 0;
	virtual BoltRsp handleMsg(const BoltMsg &msg) = 0;
    virtual void win() = 0;
};

class FunhouseEngine : public Engine {
public:
	FunhouseEngine(OSystem *syst, const ADGameDescription *gd);

    void win();

	// From Engine
	virtual bool hasFeature(EngineFeature f) const;

	uint32 getEventTime() const;
	uint32 getNowTime() const; // NOTE: getEventTime should be used in most cases.
	void setNextMsg(const BoltMsg &msg);
	void requestSmoothAnimation();
	void requestHover();
	void setTimer(uint32 start, int32 delay, int id);

	Graphics* getGraphics();

protected:
	// From Engine
	virtual Common::Error run();

private:
	BoltMsg getNextMsg();
	void yield();
	
    Common::ScopedPtr<FunhouseConsole> _console;
	Graphics _graphics;

    Common::ScopedPtr<FunhouseGame> _game;

	BoltMsg _nextMsg;
	uint32 _eventTime;

	static const int kMaxEventsSinceYield = 1024;
	int _eventsSinceYield = 0;

	struct Timer {
		uint32 start;
		int32 delay;
		int id;
	};
	Common::List<Timer> _timers;

	// True if a kSmoothAnimation message has been requested.
	bool _smoothAnimationRequested = false;
	// True if a kSmoothAnimation message has been sent this frame.
	// The engine allows one kSmoothAnimation message per frame.
	bool _smoothAnimationSent = false;
	// True if a kHover message has been requested this frame.
	// This forces a kHover message to be sent even if the mouse has not moved.
	bool _hoverRequested = false;
};

} // End of namespace Funhouse

#endif
