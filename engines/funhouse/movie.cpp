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

#include "funhouse/movie.h"

#include "audio/audiostream.h"
#include "audio/decoders/raw.h"
#include "audio/mixer.h"
#include "common/debug.h"
#include "common/system.h"
#include "common/util.h"
#include "graphics/palette.h"
#include "graphics/surface.h"

#include "funhouse/bolt.h"
#include "funhouse/graphics.h"
#include "funhouse/pf_file.h"

namespace Funhouse {

Movie::Movie()
	: _audioStream(nullptr),
	_audioStarted(false),
	_triggerCallback(nullptr),
	_triggerCallbackParam(nullptr)
{ }

Movie::~Movie() {
	stopAudio();
}

void Movie::start(FunhouseEngine *engine, PfFile &pfFile, uint32 name) {
	debug(3, "loading movie %c%c%c%c ...",
		(name >> 24) & 0xff, (name >> 16) & 0xff, (name >> 8) & 0xff, name & 0xff);

	_engine = engine;
	_mode.init(_engine);

	stop();

	_file = pfFile.seekMovieAndGetFile(name);
	if (!_file) {
		warning("Movie not found");
		return;
	}

	_parserActive = true;
	_timelineActive = true;

	loadAudio();

	// Timeline should be the first packet
	startTimeline(fetchBuffer(_timelineQueue));

	_engine->setNextMsg(BoltMsg::kDrive);
}

void Movie::stop() {
	stopAudio();

	_parserActive = false;
	_timelineActive = false;

	_timelineBufAssembler.buf.reset();
	_audioBufAssembler.buf.reset();
	_videoBufAssembler.buf.reset();
	_auxVideoBufAssembler.buf.reset();

	_timeline.reset();
	for (int i = 0; i < 5; ++i) {
		_videoQueues[i].clear();
	}
	_cels.reset();
	_celsBackground.reset();
	_celCurCameraX = 0;
	_celNextCameraX = 0;
	_celCurCameraY = 0;
	_celNextCameraY = 0;
	_celScrollType = -1;

	_numColorCycles = 0;

	_fadeDirection = 0;
}

void Movie::stopAudio() {
	if (_audioStream) {
		if (_audioStarted) {
			_engine->_mixer->stopHandle(_audioHandle); // Mixer deletes audio stream
		}
		else {
			delete _audioStream;
		}

		_audioStream = nullptr;
	}

	_audioStarted = false;
}

bool Movie::isAudioRunning() const {
	return _audioStarted && _audioStream &&
		_engine->_mixer->isSoundHandleActive(_audioHandle);
}

bool Movie::isRunning() const {
	return _engine->getGraphics() && (_timelineActive || isAudioRunning());
}

BoltRsp Movie::handleMsg(const BoltMsg &msg) {
	_mode.react(msg);
	return kDone;
}

void Movie::setTriggerCallback(TriggerCallback callback, void *param) {
	_triggerCallback = callback;
	_triggerCallbackParam = param;
}

void Movie::playMode() {
	_mode.transition();
	_mode.onEnter([this]() {
		_frameTimer.start(_framePeriod, true);
	});
	_mode.onMsg([this](const BoltMsg &msg) {
		bool handled = false;
		switch (msg.type) {
		case BoltMsg::kSmoothAnimation:
			// Fades have smooth animation; they have a higher frame rate than movie cels.
			driveFade();
			handled = true;
			break;

		case BoltMsg::kAddTicks:
			if (_fadeDirection != 0) {
				_fadeTimer += msg.num;
			}
			handled = true;
			break;
		}

		if (handled && _fadeDirection != 0) {
			// Request smooth animation when fading
			_engine->requestSmoothAnimation();
		}
	});
	_mode.onTimer(&_frameTimer, [this]() {
		_frameTimer.ticks -= _framePeriod;

		driveAudio();
		driveFade(); // TODO: use accurate time
		stepTimeline();

		if (_fadeDirection != 0) {
			// Request smooth animation when fading
			_engine->requestSmoothAnimation();
		}
	});
}

void Movie::loadAudio() {
	if (!_audioStream) {
		_audioStream = Audio::makeQueuingAudioStream(22050, false);
		fillAudioQueue();
	}
}

void Movie::driveAudio() {
	fillAudioQueue();
}

void Movie::playAudio() {
	if (!_audioStarted) {
		// PF audio is premixed; speech and music volumes cannot be controlled
		// independently.
		// TODO: Make start of audio coincide exactly with the first frame of video.
		_engine->_mixer->playStream(Audio::Mixer::kPlainSoundType, &_audioHandle,
			_audioStream);
		_audioStarted = true;
	}
}

void Movie::fillAudioQueue() {
	static const int kNumSoundPacketsToQueue = 2;

	while (_parserActive && _audioStream &&
		_audioStream->numQueuedStreams() < kNumSoundPacketsToQueue) {

		readNextPacket();
	}
}

struct TimelineHeader {
	static const uint kSize = 0xC;
	TimelineHeader(Common::Span<const byte> src) {
		numCommands = src.getUint16BEAt(8);
		framePeriod = src.getUint16BEAt(0xA);
	}

	uint16 numCommands;
	uint16 framePeriod;
};

void Movie::startTimeline(ScopedBuffer::Movable buf) {
	_timeline.reset(buf);

	_curFrameNum = 0;

	TimelineHeader header(_timeline.span());
	_numTimelineCmds = header.numCommands;
	_framePeriod = header.framePeriod;
	_timelineCursor = TimelineHeader::kSize;
	_curTimelineCmdNum = 0;
	_lastTimelineCmdFrame = _curFrameNum;
	loadTimelineCommand();

	playMode();

	stepTimeline();
}

struct TimelineCommand {
	static const int kSize = 5;
	TimelineCommand(Common::Span<const byte> src) {
		delay = src.getUint16BEAt(0);
		opcode = src.getUint16BEAt(2);
		reps = src.getUint8At(4);
	}

	uint16 delay;
	uint16 opcode;
	uint8 reps;
};

namespace TimelineOpcodes {
	enum {
		kDrawFore = 1, // param size: 0
		kDrawBack = 2, // param size: 0
		kStartColorCycles = 12, // param size: 8
		kStopColorCycles = 13, // param size: 0
		kStartCelSequence = 15, // param size: 0
		kStepCelSequence = 16, // param size: 0
		kFade = 19, // param size: 4
		kName = 0x7FFF, // param size: 4
		kTriggerEvent1 = 0x8001, // param size: 0
		kTriggerEvent2 = 0x8002 // param size: 0
	};
}

void Movie::stepTimeline() {
	assert(_timeline);

	if (!_timelineActive) {
		return;
	}

	++_curFrameNum;

	// There may be one or more timeline commands with 0 delay. Run them all in this step.
	bool done = false;
	while (!done) {
		TimelineCommand cmd(_timeline.span().subspan(_timelineCursor));
		if (_timelineReps <= 0) {
			// Advance to next timeline command
			_timelineCursor += TimelineCommand::kSize + getTimelineCmdParamSize(cmd.opcode);
			++_curTimelineCmdNum;
			if (_curTimelineCmdNum >= _numTimelineCmds) {
				_timelineActive = false;
				done = true;
			}
			else {
				loadTimelineCommand();
			}
		}
		else if ((_curFrameNum - _lastTimelineCmdFrame) < cmd.delay) {
			// Delay occurs BEFORE command.
			done = true;
		}
		else {
			runTimelineCommand();
			_lastTimelineCmdFrame = _curFrameNum;
			--_timelineReps;
		}
	}

	_engine->getGraphics()->markDirty();
	playAudio();
}

void Movie::loadTimelineCommand() {
	assert(_timeline);

	TimelineCommand cmd(_timeline.span().subspan(_timelineCursor));
	_timelineReps = cmd.reps;
}

int Movie::getTimelineCmdParamSize(uint16 opcode) {
	switch (opcode) {
	case TimelineOpcodes::kDrawFore: return 0;
	case TimelineOpcodes::kDrawBack: return 0;
	case TimelineOpcodes::kStartColorCycles: return 8;
	case TimelineOpcodes::kStopColorCycles: return 0;
	case TimelineOpcodes::kStartCelSequence: return 0;
	case TimelineOpcodes::kStepCelSequence: return 0;
	case TimelineOpcodes::kFade: return 4;
	case TimelineOpcodes::kName: return 4;
	case TimelineOpcodes::kTriggerEvent1: return 0;
	case TimelineOpcodes::kTriggerEvent2: return 0;
	default:
		error("Unhandled timeline opcode 0x%X (unknown size)", opcode);
		return 0;
	}
}

void Movie::runTimelineCommand() {
	TimelineCommand cmd(_timeline.span().subspan(_timelineCursor));
	int paramsOffset = _timelineCursor + TimelineCommand::kSize;

	switch (cmd.opcode) {
	case TimelineOpcodes::kDrawFore: // param size: 0
	{
		// Fetch foreground from queue 0
		ScopedBuffer buf(fetchBuffer(_videoQueues[0]));
		applyQueue0or1Palette(kFore, buf);
		drawQueue0or1(kFore, buf, 0, 0);
		break;
	}
	case TimelineOpcodes::kDrawBack: // param size: 0
	{
		// Clear fore, fetch background from queue 1
		_engine->getGraphics()->clearPlane(kFore);
		ScopedBuffer buf(fetchBuffer(_videoQueues[1]));
		applyQueue0or1Palette(kBack, buf);
		drawQueue0or1(kBack, buf, 0, 0);
		break;
	}
	case TimelineOpcodes::kStartColorCycles: // param size: 8
	{
		uint16 startC = READ_BE_UINT16(&_timeline[paramsOffset + 0]);
		uint16 plane = READ_BE_UINT16(&_timeline[paramsOffset + 2]); // ?? Always 1 in Merlin
		uint16 num = READ_BE_UINT16(&_timeline[paramsOffset + 4]);
		int16 delay = READ_BE_INT16(&_timeline[paramsOffset + 6]); // Negative delay means cycle backwards
		debug(3, "start color cycles (%d, %d, %d, %d)",
			(int)startC, (int)plane, (int)num, (int)delay
			);
		if (plane != 1) {
			warning("Color cycling plane not 1 in movie");
		}
		if (_numColorCycles < 0 || _numColorCycles >= kMaxColorCycles) {
			warning("tried to start too many color cycles");
		}
		if (delay < 0) {
			// Cycle backwards
			_engine->getGraphics()->setColorCycle(_numColorCycles, kBack, startC + num - 1, startC, -delay);
		}
		else {
			// Cycle forwards
			_engine->getGraphics()->setColorCycle(_numColorCycles, kBack, startC, startC + num - 1, delay);
		}
		++_numColorCycles;
		break;
	}
	case TimelineOpcodes::kStopColorCycles: // stop palette cycling (param size: 0)
		debug(3, "stop color cycles");
		_engine->getGraphics()->resetColorCycles();
		_numColorCycles = 0;
		break;
	case TimelineOpcodes::kStartCelSequence: // param size: 0
		// Load, but don't do anything yet.
		loadCels(fetchBuffer(_videoQueues[4]));
		break;
	case TimelineOpcodes::kStepCelSequence: // param size: 0
		stepCels();
		break;
	case TimelineOpcodes::kFade: // fade (param size: 4)
	{
		uint16 duration = READ_BE_UINT16(&_timeline[paramsOffset + 0]);
		int16 direction = READ_BE_INT16(&_timeline[paramsOffset + 2]); // 1: fade in; -1: fade out
		debug(3, "fade (%d, %d)", (int)duration, (int)direction);
		startFade(duration, direction);
		break;
	}
	case TimelineOpcodes::kName: // set name (param size: 4, movie name)
		// Ignored
		break;
	case TimelineOpcodes::kTriggerEvent1: // trigger event (param size: 0, used in INTR)
	case TimelineOpcodes::kTriggerEvent2: // trigger event (param size: 0, enters hub card in win movies)
		if (_triggerCallback) {
			_triggerCallback(_triggerCallbackParam, cmd.opcode);
		}
		break;
	default:
		error("Unimplemented timeline command 0x%X", (int)cmd.opcode);
		break;
	}
}

struct CelsHeader {
	static const int kSize = 0x14;
	CelsHeader(Common::Span<const byte> src) {
		queueNum = src.getUint16BEAt(0);
		width = src.getUint16BEAt(2);
		height = src.getUint16BEAt(4);
		numFrames = src.getUint16BEAt(6);
		unk8 = src.getUint32BEAt(8);
		controlDataOffset = src.getUint32BEAt(0xC);
	}

	uint16 queueNum;
	uint16 width;
	uint16 height;
	uint16 numFrames;
	uint32 unk8;
	uint32 controlDataOffset;
};

void Movie::loadCels(ScopedBuffer::Movable buf) {
	_cels.reset(buf);

	CelsHeader header(_cels.span());
	assert(header.queueNum == 4);

	debug(4, "loading cels width %d, height %d, numFrames %d, unk8 %d",
		(int)header.width, (int)header.height, (int)header.numFrames, (int)header.unk8);

	// Do NOT reset background or camera here.
	_celsFrame = 0;
	_celControlCursor = header.controlDataOffset;
}

void Movie::stepCels() {
	if (_cels) {
		CelsHeader header(_cels.span());
		if (_celsFrame >= header.numFrames) {
			_celsFrame = header.numFrames - 1;
			warning("Ran past end of cel sequence");
		}

		//debug(3, "running control on cel %d of %d...", celsFrame+1, (int)header.numFrames);
		stepCelCommands();
		advanceScroll();

		if (_celNextCameraX != _celCurCameraX || _celNextCameraY != _celCurCameraY) {
			_celCurCameraX = _celNextCameraX;
			_celCurCameraY = _celNextCameraY;
			drawCelBackground();
		}

		drawCel(_cels, _celsFrame);

		++_celsFrame;
	}
	else {
		warning("No cels loaded");
	}
}

struct CelCommand {
	static const int kSize = 4;
	CelCommand(Common::Span<const byte> src) {
		celNumber = src.getUint8At(0);
		opcode = src.getUint8At(1);
		paramSize = src.getUint16BEAt(2);
	}

	byte celNumber;
	byte opcode;
	uint16 paramSize; // This field is ignored in original program. Param size depends on opcode.
};

namespace CelOpcodes {
	enum {
		kLoadBack = 1,
		kLoadForePalette = 2,
		kScroll = 3,
		kStop = 0xFF
	};
}

void Movie::stepCelCommands() {
	assert(_cels);

	// FIXME: Cel control commands are different between Merlin and Crete.
	// Only Merlin commands are implemented.

	bool done = false;
	while (!done) {
		CelCommand cmd(_cels.span().subspan(_celControlCursor));
		int paramsOffset = _celControlCursor + CelCommand::kSize;

		// FIXME: There's still an off-by-one error causing a one frame desync.
		// Watch TOUR carefully, you can see the background switch a frame too early
		// in the scene following the Merlin portrait.
		// FIXME unless this happened in the original.
		if (_celsFrame >= cmd.celNumber) {
			if (cmd.opcode != CelOpcodes::kStop) {
				debug(3, "cel command at %d paramSize %d opcode %d", (int)cmd.celNumber,
					(int)cmd.paramSize, (int)cmd.opcode);
			}

			switch (cmd.opcode) {
			case CelOpcodes::kLoadBack:
			{
				debug(3, "cel command: load background");
				Common::Span<const byte> params = _cels.span().subspan(paramsOffset);

				_celsBackground.reset(fetchBuffer(_videoQueues[1]));

				_celCurCameraX = params.getInt16BEAt(0);
                _celCurCameraY = params.getInt16BEAt(2);
				_celNextCameraX = _celCurCameraX;
				_celNextCameraY = _celCurCameraY;

				applyQueue0or1Palette(kBack, _celsBackground);
				drawCelBackground();

				_celControlCursor += CelCommand::kSize + 4;
				break;
			}
			case CelOpcodes::kLoadForePalette:
			{
				Common::Span<const byte> params = _cels.span().subspan(paramsOffset);

				byte numColors = params.getUint8At(0);
				byte firstColor = params.getUint8At(1);
				debug(3, "cel command: load fore palette num %d first %d",
					(int)numColors, (int)firstColor);
				_engine->getGraphics()->setPlanePalette(kFore, &_cels[paramsOffset + 2], // TODO: utilize dataview here
					firstColor, numColors);

				_celControlCursor += CelCommand::kSize + 2 + numColors * 3;
				break;
			}
			case CelOpcodes::kScroll:
			{
				Common::Span<const byte> params = _cels.span().subspan(paramsOffset);

				uint16 duration = params.getUint16BEAt(0);
				byte speed = params.getUint8At(2);
				byte type = params.getUint8At(3);

				debug(3, "cel command: scroll duration %d, speed %d, type %d",
					(int)duration, (int)speed, (int)type);

				startScroll(duration, speed, type);

				_celControlCursor += CelCommand::kSize + 4;
				break;
			}
			case CelOpcodes::kStop:
				debug(4, "cel command: stop");
				done = true;
				break;
			default:
				error("Unknown cel command 0x%X", (int)cmd.opcode);
				break;
			}
		}
		else {
			done = true;
		}
	}
}

void Movie::startScroll(int duration, int speed, int type) {
	_celScrollDuration = duration;
	_celScrollSpeed = speed;
	_celScrollType = type;

	_celScrollProgress = 0;
	_celScrollStartCameraX = _celCurCameraX;
	_celScrollStartCameraY = _celCurCameraY;
}

void Movie::advanceScroll() {
	// FIXME: Scrolling is mostly correct, but CAV1 has an error where
	// scrolling pauses midway before reaching the bottom.
	if (_celScrollType != -1) {
		++_celScrollProgress;
		if (_celScrollProgress > _celScrollDuration) {
			_celScrollType = -1;
		}
		else {
			int vx = 0;
			int vy = 0;
			switch (_celScrollType) {
			case 0: // Left (unused)
				vx = -_celScrollSpeed;
				vy = 0;
				break;
			case 1: // Right (unused)
				vx = _celScrollSpeed;
				vy = 0;
				break;
			case 2: // Up (used only in finale)
				vx = 0;
				vy = -_celScrollSpeed;
				break;
			case 3: // Down
				vx = 0;
				vy = _celScrollSpeed;
				break;
			default:
				warning("Invalid scroll type %d", _celScrollType);
				break;
			}

			_celNextCameraX = _celScrollStartCameraX + vx * _celScrollProgress;
			_celNextCameraY = _celScrollStartCameraY + vy * _celScrollProgress;
		}
	}
}

void Movie::drawCelBackground() {
	if (_celsBackground) {
		drawQueue0or1(kBack, _celsBackground, -_celCurCameraX, -_celCurCameraY);
	}
}

void Movie::drawCel(const ScopedBuffer &src, uint16 frameNum) {
	// Queue 4 buffers define a sequence of foreground cels and background control commands.
	CelsHeader header(src.span());
	assert(header.queueNum == 4);
	assert(frameNum < header.numFrames);

    uint32 rl7Offset = src.span().getUint32BEAt(CelsHeader::kSize + frameNum * 8);
    uint32 rl7Size = src.span().getUint32BEAt(CelsHeader::kSize + frameNum * 8 + 4);

	decodeRL7(_engine->getGraphics()->getPlaneSurface(kFore), 0, 0, header.width, header.height,
		&src[rl7Offset], rl7Size, false);
}

enum PfPacketType {
	kPfTimeline = 0, // Timeline info (appears at beginning of movie)
	kPfAudio = 1, // Raw mono unsigned 8-bit PCM at 22050 Hz
	kPfVideo = 2, // Video
	kPfAuxVideo = 3, // Auxiliary video (used for large scrolling backgrounds)
	// NOTE: type 4 is related to sound. Original program flushes sound queue
	// when it encounters a type 4 packet.
	// NOTE: type 0xFE occurs near the end. It might signal the end of sound
	// packets.
	kPfFinal = 0xFF // End of packets
};

struct Movie::PacketHeader {
	PacketHeader(Common::File &file) {
		totalSize = file.readUint32BE();
		partialSize = file.readUint32BE();
		type = file.readByte();
		unk = file.readByte();
	}

	uint32 totalSize;
	uint32 partialSize;
	uint8 type;
	uint8 unk;
};

void Movie::readNextPacket() {
	assert(_parserActive);

	// Read packet header
	PacketHeader header(*_file);
	// header.unk is always 0 in Merlin; original program seems to use it for
	// something related to multi-streaming.
	if (header.unk != 0) {
		warning("Unknown pf packet header unk %u", header.unk);
	}

	switch (header.type) {
	case kPfTimeline:
		if (readIntoBuffer(_timelineBufAssembler, header)) {
			_timelineQueue.push(_timelineBufAssembler.buf.release());
			_timelineBufAssembler.buf.reset();
		}
		break;
	case kPfAudio:
		if (readIntoBuffer(_audioBufAssembler, header)) {
			if (_audioStream) {
				// FIXME: Make this more efficient by reading directly into a
				// malloc'ed buffer.
				byte *sound = (byte*)malloc(_audioBufAssembler.totalSize);
				memcpy(sound, &_audioBufAssembler.buf[0], _audioBufAssembler.totalSize);

				_audioStream->queueBuffer(sound, _audioBufAssembler.totalSize,
					DisposeAfterUse::YES, Audio::FLAG_UNSIGNED);
				// sound will be freed by audio system
			}

			_audioBufAssembler.buf.reset();
		}
		break;
	case kPfVideo:
		if (readIntoBuffer(_videoBufAssembler, header)) {
			enqueueVideoBuffer(_videoBufAssembler.buf.release());
			_videoBufAssembler.buf.reset();
		}
		break;
	case kPfAuxVideo:
		if (readIntoBuffer(_auxVideoBufAssembler, header)) {
			// FIXME: Is any special handling required for auxiliary video? I
			// believe auxiliary video is nothing more than a second video
			// stream, allowing large video buffers (like background images)
			// to be loaded alongside regular video.
			enqueueVideoBuffer(_auxVideoBufAssembler.buf.release());
			_auxVideoBufAssembler.buf.reset();
		}
		break;
	case kPfFinal:
		// Final packet found, stop parser.
		// Movie will stop when timeline and audio are done.
		if (_audioStream) {
			_audioStream->finish();
			_audioStream = nullptr; // Audio stream will be freed by the mixer.
		}
		_parserActive = false;
		break;
	default:
		warning("Unknown PF packet type %u skipped", header.type);
		_file->seek(header.partialSize, SEEK_CUR);
		break;
	}
}

// Returns true if buffer is complete. Buffers may be split across multiple
// packets.
bool Movie::readIntoBuffer(BufferAssembler &assembler, const PacketHeader &header) {

	if (!assembler.buf) {
		// Begin buffer
		assembler.totalSize = header.totalSize;
		assembler.buf.alloc(header.totalSize);
		assembler.cursor = 0;
	}
	else if (header.totalSize != assembler.totalSize) {
		warning("Bad PF packet: total size field mismatch");
	}

	uint32 partialSize = header.partialSize;
	if ((assembler.cursor + partialSize) > assembler.totalSize) {
		warning("Bad PF packet: buffer overflow");
		partialSize = assembler.totalSize - assembler.cursor;
	}

	_file->read(&assembler.buf[assembler.cursor], partialSize);
	assembler.cursor += partialSize;

	return assembler.cursor >= assembler.totalSize;
}

Movie::ScopedBuffer::Movable Movie::fetchBuffer(ScopedBufferQueue &queue) {
	while (_parserActive && queue.empty()) {
		readNextPacket();
	}

	if (queue.empty()) {
		warning("Failed to fetch movie data buffer");
		return Movie::ScopedBuffer::Movable();
	}

	return queue.pop();
}

void Movie::startFade(uint16 duration, int16 direction) {
	_fadeTimer = 0;
	_fadeDuration = duration;
	if (direction == 1 || direction == -1) {
		_fadeDirection = direction;
	}
	else {
		warning("Invalid fade direction %d", (int)direction);
		_fadeDirection = 0;
	}
}

void Movie::driveFade() {
	if (_fadeDirection == 1) {
		// Fade in
		if (_fadeTimer >= _fadeDuration) {
			_engine->getGraphics()->setFade(1);
			_fadeDirection = 0;
		}
		else {
			_engine->getGraphics()->setFade(Common::Rational(_fadeTimer, _fadeDuration));
		}
	}
	else if (_fadeDirection == -1) {
		// Fade out
		if (_fadeTimer >= _fadeDuration) {
			_engine->getGraphics()->setFade(0);
			_fadeDirection = 0;
		}
		else {
			_engine->getGraphics()->setFade(Common::Rational(_fadeDuration - _fadeTimer, _fadeDuration));
		}
	}
	else if (_fadeDirection == 0) {
		// Do nothing
	}
	else {
		assert(false); // Unreachable; fade direction must be -1, 0, or 1.
	}
}

struct Queue01ImageHeader {
	const static int kSize = 0xD;
	Queue01ImageHeader(Common::Span<const byte> src) {
		queueNum = src.getUint16BEAt(0);
		width = src.getUint16BEAt(2);
		height = src.getUint16BEAt(4);
		// FIXME: Unknown fields
		unk[0] = src.getUint8At(6); // Always 128
		unk[1] = src.getUint8At(7); // Always 0
		unk[2] = src.getUint8At(8); // Always 0
		unk[3] = src.getUint8At(9); // Always 0
		unk[4] = src.getUint8At(0xA); // Something
		unk[5] = src.getUint8At(0xB); // Something (probably data size)
		compression = src.getUint8At(0xC);
	}

	uint16 queueNum;
	uint16 width;
	uint16 height;
	byte unk[6];
	byte compression;
};

void Movie::applyQueue0or1Palette(int plane, const ScopedBuffer &src) {
	Queue01ImageHeader header(src.span());
	assert(header.queueNum == 0 || header.queueNum == 1);

	_engine->getGraphics()->setPlanePalette(plane, &src[Queue01ImageHeader::kSize], 0, 128);
}

void Movie::drawQueue0or1(int plane, const ScopedBuffer &src, int x, int y) {
	// Queue 0 buffers define background frames for scene changes.
	// Queue 1 buffers define background frames for use with cel sequences.
	Queue01ImageHeader header(src.span());
	assert(header.queueNum == 0 || header.queueNum == 1);

	const byte *imageSrc = &src[Queue01ImageHeader::kSize + 128 * 3];
	int imageDataLen = src.size() - 128 * 3 - Queue01ImageHeader::kSize;

	if (header.compression) {
		decodeRL7(_engine->getGraphics()->getPlaneSurface(plane), x, y, header.width, header.height,
			imageSrc, imageDataLen, false);
	}
	else {
		decodeCLUT7(_engine->getGraphics()->getPlaneSurface(plane), x, y, header.width, header.height,
			imageSrc, imageDataLen, false);
	}
}

void Movie::enqueueVideoBuffer(ScopedBuffer::Movable buf) {
	ScopedBuffer myBuf(buf);
    uint16 queueNum = myBuf.span().getUint16BEAt(0);
	if (queueNum < 5) {
		_videoQueues[queueNum].push(myBuf.release());
	}
	else {
		warning("Unknown PF image stream queue num %u", queueNum);
	}
}

} // End of namespace Funhouse
