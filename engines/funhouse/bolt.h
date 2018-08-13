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

// A Bolt::Rect differs from a Common::Rect in the following ways:
// - Attributes are stored in left, right, top, bottom order
// - All edges are inclusive, i.e. right and bottom are included in the
//   rectangle
struct Rect {
	static const uint kSize = 8;
	Rect() : left(0), right(0), top(0), bottom(0) { }
	Rect(int16 l, int16 t, int16 r, int16 b)
		: left(l), right(r), top(t), bottom(b) { }
	Rect(const ConstSizedDataView<kSize> src) {
		left = src.readInt16BE(0);
		right = src.readInt16BE(2);
		top = src.readInt16BE(4);
		bottom = src.readInt16BE(6);
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
		kNone = 0,
		kHover,
		kClick,
		kRightClick,
		kTimer,
		kAudioEnded, // TODO: implement
		kSmoothAnimation,
		kDrive, // Drive events are fired constantly. TODO: eliminate Drive
		kMaxBoltMsg = 100
	};

	BoltMsg(int type_ = kNone) : type(type_), num(0) { }

	int type;
	int num;
	Common::Point point;
};

// Commands that the game returns to the engine after handling a message.
struct BoltCmd {
	enum Type {
		// System commands
		kDone = 0, // Message has been handled.
		kResend,   // Message should be resent. The game can use setMsg to change the message.
		kMaxBoltCmd = 100
	};

	int type;
	int num;

	BoltCmd(int type_ = kDone) : type(type_), num(0) { }
};

class Card {
public:
	// Card-specific commands for use in BoltCmd
	enum CardCmd {
		// Card commands
		kEnd = BoltCmd::kMaxBoltCmd,
		kWin,
		kEnterPuzzle,
		kMaxCardCmd = BoltCmd::kMaxBoltCmd + 100
	};

	virtual ~Card() { }
	virtual void enter() = 0;
	virtual BoltCmd handleMsg(const BoltMsg &msg) = 0;
};

enum TimerId {
	kMovieTimer,
	kCardTimer
};

class IBoltEventLoop {
public:
	virtual ~IBoltEventLoop() { }
	virtual uint32 getEventTime() const = 0;
	virtual void setMsg(const BoltMsg &msg) = 0;
	virtual void requestSmoothAnimation() = 0;
	virtual void setTimer(uint32 delay, int id) = 0;
};

class FunhouseGame {
public:
	virtual ~FunhouseGame() { }
	virtual void init(OSystem *system, Graphics *graphics, Audio::Mixer *mixer, IBoltEventLoop *eventLoop) = 0;
	virtual BoltCmd handleMsg(const BoltMsg &msg) = 0;
};

class FunhouseEngine : public Engine, public IBoltEventLoop {
public:
	FunhouseEngine(OSystem *syst, const ADGameDescription *gd);

	// From Engine
	virtual bool hasFeature(EngineFeature f) const;

	// From IBoltEventLoop (for internal game use)
	virtual uint32 getEventTime() const;
	virtual void setMsg(const BoltMsg &msg);
	virtual void requestSmoothAnimation();
	virtual void setTimer(uint32 delay, int id);

protected:
	// From Engine
	virtual Common::Error run();

private:
	void topLevelHandleMsg(const BoltMsg &msg);
	
	Graphics _graphics;
	BoltMsg _curMsg;
	uint32 _eventTime;
	Common::ScopedPtr<FunhouseGame> _game;

	struct Timer {
		uint32 start;
		uint32 delay;
		int id;
	};
	Common::List<Timer> _timers;

	bool _smoothAnimationRequested;
};

} // End of namespace Funhouse

#endif
