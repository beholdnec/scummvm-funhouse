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

#include "funhouse/merlin/merlin.h"

#include "common/events.h"
#include "common/system.h"
#include "gui/message.h"

#include "funhouse/bolt.h"
#include "funhouse/merlin/action_puzzle.h"
#include "funhouse/merlin/color_puzzle.h"
#include "funhouse/merlin/memory_puzzle.h"
#include "funhouse/merlin/potion_puzzle.h"
#include "funhouse/merlin/sliding_puzzle.h"
#include "funhouse/merlin/synch_puzzle.h"
#include "funhouse/merlin/tangram_puzzle.h"
#include "funhouse/merlin/word_puzzle.h"
#include "funhouse/merlin/hub.h"
#include "funhouse/merlin/main_menu.h"
#include "funhouse/merlin/file_menu.h"
#include "funhouse/merlin/difficulty_menu.h"

namespace Funhouse {

// Call pointer to member function.
// See <https://isocpp.org/wiki/faq/pointers-to-members>
#define CALL_MEMBER_FN(object, fn) ((object).*(fn))

struct BltPopupCatalog {
	static const uint32 kType = kBltPopupCatalog;
	static const uint32 kSize = 0x22;
	void load(Common::Span<const byte> src, Boltlib &bltFile) {
		popupId[0] = BltId(src.getUint32BEAt(0x12));
		popupId[1] = BltId(src.getUint32BEAt(0x18));
		popupId[2] = BltId(src.getUint32BEAt(0x1E));
	}

	BltId popupId[3];
};

static const uint16 kPopupCatalogId = 0x0A04;

void MerlinGame::init(OSystem *system, FunhouseEngine *engine, Audio::Mixer *mixer) {
	_system = system;
	_engine = engine;
	_cheatMode = false;
	_saveMan.init(this);
	for (int i = 0; i < kNumDifficultyCategories; ++i) {
		// FIXME: Set all difficulties to -1: not set
		// _difficulties[i] = -1;
		_difficulties[i] = 0;
	}

	_boltlib.load("BOLTLIB.BLT");

	_maPf.load("MA.PF");
	_helpPf.load("HELP.PF");
	_potionPf.load("POTION.PF");
	_challdirPf.load("CHALLDIR.PF");

	_movie.setTriggerCallback(MerlinGame::movieTrigger, this);

	// Load popup catalog
	BltPopupCatalog popupCatalog;
	loadBltResource(popupCatalog, _boltlib, BltShortId(kPopupCatalogId));
	for (int i = 0; i < kNumPopupTypes; ++i) {
		BltU16Values popupIds;
		loadBltResourceArray(popupIds, _boltlib, popupCatalog.popupId[i]);
		_popupResIds[i] = BltShortId(popupIds[0].value);
	}

	_scriptCursor = kInitialScriptCursor;
	_nextScriptCursor = kInitialScriptCursor;
	_prevScriptCursor = kInitialScriptCursor;

	// Load cursor
	initCursor();

	runScript();
}

BoltRsp MerlinGame::handleMsg(const BoltMsg &msg) {
	BoltRsp rsp = kDone;

	if (isInMovie()) {
		rsp = handleMsgInMovie(msg);
	} else {
		rsp = handleMsgInCard(msg);
	}

	return rsp;
}

BoltRsp MerlinGame::handleMsgInCard(const BoltMsg &msg) {
	BoltRsp rsp = kDone;

	if (_activeCard) {
		rsp = _activeCard->handleMsg(msg);
	}

	if (rsp == kDone) {
		// Handle card transitions
		if (_nextScriptCursor != _scriptCursor) {
			_prevScriptCursor = _scriptCursor;
			_scriptCursor = _nextScriptCursor;
			runScript();
		}
	}

	return rsp;
}

void MerlinGame::runScript() {
	const ScriptEntry& entry = kScript[_scriptCursor];
	CALL_MEMBER_FN(*this, entry.func)(&entry);
}

void MerlinGame::win() {
	// TODO: only call branchWin in puzzles
	branchWin();
}

OSystem* MerlinGame::getSystem() {
	return _system;
}

FunhouseEngine* MerlinGame::getEngine() {
	return _engine;
}

Graphics* MerlinGame::getGraphics() {
	return _engine->getGraphics();
}

bool MerlinGame::isInMovie() const {
	return _movie.isRunning();
}

void MerlinGame::startMAMovie(uint32 name) {
	startMovie(_maPf, name);
}

void MerlinGame::startPotionMovie(int num) {
	if (num < 0 || num >= kNumPotionMovies) {
		warning("Tried to play invalid potion movie %d", num);
		return;
	}

	startMovie(_potionPf, kPotionMovies[num]);
}

bool MerlinGame::doesProfileExist(int idx) const {
	return _saveMan.getProfileStatus(idx);
}

int MerlinGame::getProfile() const {
	return _saveMan.getProfileIdx();
}

void MerlinGame::selectProfile(int idx) {
	_saveMan.setProfileIdx(idx);
}

BltId MerlinGame::getPopupResId(PopupType type) {
	return _popupResIds[type];
}

void MerlinGame::initCursor() {
	static const uint16 kCursorImageId = 0x9D00;
	static const byte kCursorPalette[3 * 2] = { 0, 0, 0, 255, 255, 255 };

	if (!_cursorImage) {
		_cursorImage.load(_boltlib, BltShortId(kCursorImageId));
	}

	::Graphics::Surface surface;
	surface.create(_cursorImage.getWidth(), _cursorImage.getHeight(),
		::Graphics::PixelFormat::createFormatCLUT8());
	_cursorImage.draw(surface, false);
	_system->setMouseCursor(surface.getPixels(),
		_cursorImage.getWidth(), _cursorImage.getHeight(),
		-_cursorImage.getOffset().x, -_cursorImage.getOffset().y, 0);
	_system->setCursorPalette(kCursorPalette, 0, 2);
	_system->showMouse(true);
	surface.free();
}

class GenericMenuCard : public Card {
public:
	void init(MerlinGame *game, Boltlib &boltlib, BltId id) {
		_game = game;
		loadScene(_scene, game->getEngine(), boltlib, id);
	}

	void enter() {
		_scene.enter();
	}

	BoltRsp handleMsg(const BoltMsg &msg) {
		if (msg.type == Scene::kClickButton) {
			_game->getEngine()->setNextMsg(BoltMsg::kDrive);
			_game->branchScript(msg.num);
			return BoltRsp::kDone;
		}

		return _scene.handleMsg(msg);
	}

private:
	MerlinGame* _game;
	Scene _scene;
};

void MerlinGame::startMenu(BltId id) {
	_activeCard.reset();
	GenericMenuCard* menuCard = new GenericMenuCard;
	menuCard->init(this, _boltlib, id);
	setActiveCard(menuCard);
}

void MerlinGame::startMovie(PfFile &pfFile, uint32 name) {
	// Color cycles do NOT stop when a movie starts.
	_movie.stop();
	_movie.start(_engine, pfFile, name);
}

void MerlinGame::movieTrigger(void *param, uint16 triggerType) {
	MerlinGame *self = reinterpret_cast<MerlinGame*>(param);
	if (triggerType == 0x8002) {
		// Enter next card; used during win movies to transition back to hub card
		if (self->_activeCard) {
			self->enterActiveCard(false);
		}
	}
	else {
		warning("unknown movie trigger 0x%.04X", (int)triggerType);
	}
}

BoltRsp MerlinGame::handleMsgInMovie(const BoltMsg &msg) {
	BoltRsp cmd = BoltRsp::kDone;
	if (msg.type == BoltMsg::kClick) {
		_movie.stop();
	} else {
		cmd = _movie.handleMsg(msg);
	}

	if (!_movie.isRunning()) {
		// If movie has stopped...
		_engine->getGraphics()->setFade(1);
		if (_activeCard) {
			enterActiveCard(true);
		}
	}

	return cmd;
}

int MerlinGame::getDifficulty(DifficultyCategory category) const {
	assert(category >= 0 && category < kNumDifficultyCategories);
	return _difficulties[category];
}

void MerlinGame::setDifficulty(DifficultyCategory category, int level) {
	assert(level >= 0 && level < 3);
	_difficulties[category] = level;
}

ChallengeStatus MerlinGame::getChallengeStatus(int idx) const {
	assert(idx >= 0 && idx < kChallengeCount);
	return _challengeStatuses[idx];
}

void MerlinGame::setChallengeStatus(int idx, ChallengeStatus status) {
	assert(idx >= 0 && idx < kChallengeCount);
	_challengeStatuses[idx] = status;
}

void MerlinGame::playWinMovie(int idx) {
	startMovie(_challdirPf, kWinMovies[idx]);
}

bool MerlinGame::getCheatMode() const {
	return _cheatMode;
}

void MerlinGame::setCheatMode(bool enable) {
	_cheatMode = enable;
}

void MerlinGame::redraw() {
	if (!isInMovie()) {
		assert(_activeCard);
		_activeCard->redraw();
	}
}

void MerlinGame::setActiveCard(Card *card) {
	_activeCard.reset(card);
	if (_activeCard) {
		enterActiveCard(true);
	}
}

void MerlinGame::enterActiveCard(bool cursorActive) {
	assert(_activeCard);
	_engine->getGraphics()->resetColorCycles();
	_activeCard->enter();
	if (cursorActive) {
		_engine->requestHover();
	}
}

void MerlinGame::branchScript(int idx, bool absolute) {
	if (absolute) {
		_nextScriptCursor = idx;
	} else {
		_nextScriptCursor = kScript[_scriptCursor].branchTable[idx];
	}

	if (_nextScriptCursor == -1) {
		branchLoadProfile();
	}

	_engine->setNextMsg(BoltMsg::kDrive);
}

void MerlinGame::branchReturn() {
	_nextScriptCursor = _scriptReturnCursor;
	_engine->setNextMsg(BoltMsg::kDrive);
}

void MerlinGame::branchWin() {
	int challengeIdx = kScript[_scriptCursor].param;
	_challengeStatuses[challengeIdx] = kPlayWinMovie;
	branchReturn();
}

void MerlinGame::branchLoadProfile() {
	const ProfileData &profile = _saveMan.getProfile();
	_nextScriptCursor = profile.scriptCursor;
	_scriptReturnCursor = profile.scriptReturnCursor;
	_engine->setNextMsg(BoltMsg::kDrive);
}

void MerlinGame::branchGamePieces() {
	_nextScriptCursor = kGamePiecesScriptCursor;
	_engine->setNextMsg(BoltMsg::kDrive);
}

void MerlinGame::branchDifficultyMenu() {
	_nextScriptCursor = kDifficultyScriptCursor;
	_engine->setNextMsg(BoltMsg::kDrive);
}

class MovieCard : public Card
{
public:
	MovieCard(MerlinGame *game) : _game(game) { }

	void enter() override {
	}

	BoltRsp handleMsg(const BoltMsg &msg) override {
		// Movie is finished. Go to the next script line.
		_game->branchScript(0);
		return kDone;
	}

private:
	MerlinGame *_game;
};

void MerlinGame::scriptPlotMovie(const ScriptEntry* entry) {
	_activeCard.reset();

	MovieCard* card = new MovieCard(this);
	setActiveCard(card);

	uint32 movie = entry->param;
	startMAMovie(movie);
}

void MerlinGame::scriptPostBumper(const ScriptEntry* entry) {
	// TODO: what does this script command actually do?
	branchScript(0);
}

void MerlinGame::scriptMenu(const ScriptEntry* entry) {
	_activeCard.reset();

	switch (entry->param) {
	case 0: {
		MainMenu *card = new MainMenu();
		card->init(this, _boltlib, BltShortId(0x0118));
		setActiveCard(card);
		break;
	}
	case 1: {
		FileMenu *card = new FileMenu();
		card->init(this, _boltlib, BltShortId(0x02A0));
		setActiveCard(card);
		break;
	}
	case 2: {
		DifficultyMenu *card = new DifficultyMenu();
		card->init(this, _boltlib, BltShortId(0x006B));
		setActiveCard(card);
		break;
	}
	default:
		assert(false);
	}
}

void MerlinGame::scriptHub(const ScriptEntry* entry) {
	_scriptReturnCursor = _scriptCursor;
	_saveMan.save();

	_activeCard.reset();

	uint16 sceneId = entry->param;
	HubCard *card = new HubCard;
	card->init(this, _boltlib, BltShortId(sceneId));
	setActiveCard(card);
}

void MerlinGame::scriptFreeplay(const ScriptEntry* entry) {
	_scriptReturnCursor = _scriptCursor;

	_activeCard.reset();

	uint16 sceneId = entry->param;
	GenericMenuCard* card = new GenericMenuCard;
	card->init(this, _boltlib, BltShortId(sceneId));
	setActiveCard(card);
}

template<class T>
void MerlinGame::scriptPuzzle(const ScriptEntry* entry) {
	// FIXME: don't save in freeplay mode
	_saveMan.save();

	_activeCard.reset();

	int challengeIdx = entry->param;
	T* card = new T();
	card->init(this, _boltlib, challengeIdx);
	setActiveCard(card);
}

// Hardcoded values from MERLIN.EXE:
//
// Action puzzles:
//   SeedsDD    4921
//   LeavesDD   4D19
//   BubblesDD  5113
//   SnowflakDD 551C
//   GemsDD     5918
//   DemonsDD   5D17
//
// Word puzzles:
//   GraveDD  61E3
//   ParchDD  69E1
//   TabletDD 65E1
//
// Tangram puzzles:
//   MirrorDD  7115
//   PlaqueDD  6D15
//   OctagonDD 7515
//   TileDD    7915
//
// Sliding puzzles:
//   RavenDD  353F
//   LeafDD   313F
//   SnakeDD  4140
//   SkeltnDD 3D3F
//   SpiderDD 453F
//   QuartzDD 393F
//
// Synchronization puzzles:
//   PlanetDD 7D12
//   DoorDD   8114
//   SphereDD 8512
//
// Color puzzles:
//   WindowDD 8C13
//   StarDD   9014
//
// Potion puzzles:
//   ForestDD 940C
//   LabratDD 980C
//   CavernDD 9C0E
//
// Memory puzzles:
//   PondDD   865E
//   FlasksDD 8797
//   StalacDD 887B
//
// Potion movies:
//   'ELEC', 'EXPL', 'FLAM', 'FLSH', 'MIST', 'OOZE', 'SHMR',
//   'SWRL', 'WIND', 'BOIL', 'BUBL', 'BSPK', 'FBRS', 'FCLD',
//   'FFLS', 'FSWR', 'LAVA', 'LFIR', 'LSMK', 'SBLS', 'SCLM',
//   'SFLS', 'SPRE', 'WSTM', 'WSWL', 'BUGS', 'CRYS', 'DNCR',
//   'FISH', 'GLAC', 'GOLM', 'EYEB', 'MOLE', 'MOTH', 'MUDB',
//   'ROCK', 'SHTR', 'SLUG', 'SNAK', 'SPKB', 'SPKM', 'SPDR',
//   'SQID', 'CLOD', 'SWIR', 'VOLC', 'WORM',
//
// TODO: there are more: cursor, menus, etc.

static const uint32 kPlotMovieBMPR = MKTAG('B', 'M', 'P', 'R');
static const uint32 kPlotMovieINTR = MKTAG('I', 'N', 'T', 'R');
static const uint32 kPlotMoviePLOG = MKTAG('P', 'L', 'O', 'G');
static const uint32 kPlotMovieLABT = MKTAG('L', 'A', 'B', 'T');
static const uint32 kPlotMovieCAV1 = MKTAG('C', 'A', 'V', '1');
static const uint32 kPlotMovieFNLE = MKTAG('F', 'N', 'L', 'E');

static const uint16 kFreeplayScenes = 0x0600; // TODO: 0600 contains ID's for freeplay hubs
static const uint16 kFreeplayScene1 = 0x0337; // so stop hardcoding these
static const uint16 kFreeplayScene2 = 0x0446;
static const uint16 kFreeplayScene3 = 0x0555;

static const uint16 kPotionPuzzle1 = 0x940C;
static const uint16 kPotionPuzzle2 = 0x980C;
static const uint16 kPotionPuzzle3 = 0x9C0E;

const int MerlinGame::kInitialScriptCursor = 0;
const int MerlinGame::kGamePiecesScriptCursor = 6;
const int MerlinGame::kDifficultyScriptCursor = 7;
const int MerlinGame::kNewGameScriptCursor = 11;

const MerlinGame::ScriptEntry
MerlinGame::kScript[] = {
	/* 0 */  { &MerlinGame::scriptPlotMovie,  MKTAG('B','M','P','R'), 0, {1, 1} }, // branch index 0
	/* 1 */  { &MerlinGame::scriptPostBumper, 0, 0, {2} }, // branch index 2
	/* 2 */  { &MerlinGame::scriptPlotMovie,  MKTAG('I','N','T','R'), 0, {3, 3} }, // branch index 3
	/* 3 */  { &MerlinGame::scriptMenu,       0, 0, {6, 4, 83, 5} }, // branch index 5
	/* 4 */  { &MerlinGame::scriptPlotMovie,  0, 0, {3, 3} }, // branch index 9
	/* 5 */  { &MerlinGame::scriptPlotMovie,  0, 0, {3, 3} }, // branch index 11
	/* 6 */  { &MerlinGame::scriptMenu,       1, 0, {3, -1, 7} }, // branch index 13
	/* 7 */  { &MerlinGame::scriptMenu,       2, 0, {3, 6, -1} }, // branch index 16
	/* 8 */  { &MerlinGame::scriptFreeplay,   0x0337, 0, {53, 54, 55, 56, 57, 58, 59, 10, 9 } }, // branch index 19
	/* 9 */  { &MerlinGame::scriptFreeplay,   0x0446, 0, {60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 8, 10} }, // branch index 28
	/* 10 */ { &MerlinGame::scriptFreeplay,   0x0555, 0, {70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 9, 8}  }, // branch index 40

	/* 11 */ { &MerlinGame::scriptPlotMovie, MKTAG('P','L','O','G'), 0, {20, 20} }, // branch index 55
	/* 12 */ { &MerlinGame::scriptPlotMovie, MKTAG('L','A','B','T'), 0, {21, 21} }, // branch index 57
	/* 13 */ { &MerlinGame::scriptPlotMovie, 0, 0, {8, 8} }, // branch index 59 
	/* 14 */ { &MerlinGame::scriptPlotMovie, MKTAG('C','A','V','1'), 0, {22, 22} }, // branch index 61 (FIXME: CAV2 is played under some conditions)
	/* 15 */ { &MerlinGame::scriptPlotMovie, 0, 0, {22, 22} }, // branch index 63
	/* 16 */ { &MerlinGame::scriptPlotMovie, 0, 0, {9, 9} }, // branch index 65
	/* 17 */ { &MerlinGame::scriptPlotMovie, 0, 0, {9, 9} }, // branch index 67
	/* 18 */ { &MerlinGame::scriptPlotMovie, /*MKTAG('F','N','L','E')*/ 0, 0, {4, 4} }, // branch index 69 (XXX: Finale movie is hidden until the game is fully implemented)
	/* 19 */ { &MerlinGame::scriptPlotMovie, 0, 0, {10, 10} }, // branch index 71
	/* 20 */ { &MerlinGame::scriptHub,       0x0C0B, 0, {23, 24, 25, 26, 27, 28, 29} }, // branch index 73
	/* 21 */ { &MerlinGame::scriptHub,       0x0D34, 0, {30, 31, 32, 33, 34, 35, 36, 37, 38, 39} }, // branch index 80
	/* 22 */ { &MerlinGame::scriptHub,       0x0E4F, 0, {40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52} }, // branch index 90
	
	// Hub 1
	/* 23 */ { &MerlinGame::scriptPuzzle<ActionPuzzle>,  2, 0, {20} }, // branch index 103
	/* 24 */ { &MerlinGame::scriptPuzzle<WordPuzzle>,    0, 0, {20} }, // branch index 104
	/* 25 */ { &MerlinGame::scriptPuzzle<SlidingPuzzle>, 1, 0, {20} }, // branch index 105
	/* 26 */ { &MerlinGame::scriptPuzzle<MemoryPuzzle>,  3, 0, {20} }, // branch index 106
	/* 27 */ { &MerlinGame::scriptPuzzle<ActionPuzzle>,  4, 0, {20} }, // branch index 107
	/* 28 */ { &MerlinGame::scriptPuzzle<SlidingPuzzle>, 5, 0, {20} }, // branch index 108
	/* 29 */ { &MerlinGame::scriptPuzzle<PotionPuzzle>,  6, 0, {12} }, // branch index 109

	// Hub 2
	/* 30 */ { &MerlinGame::scriptPuzzle<SlidingPuzzle>, 8, 0, {21} }, // branch index 110
	/* 31 */ { &MerlinGame::scriptPuzzle<ActionPuzzle>,  14, 0, {21} }, // branch index 111
	/* 32 */ { &MerlinGame::scriptPuzzle<SlidingPuzzle>, 15, 0, {21} }, // branch index 112
	/* 33 */ { &MerlinGame::scriptPuzzle<ActionPuzzle>,  9, 0, {21} }, // branch index 113
	/* 34 */ { &MerlinGame::scriptPuzzle<WordPuzzle>,    7, 0, {21} }, // branch index 114
	/* 35 */ { &MerlinGame::scriptPuzzle<SynchPuzzle>,   12, 0, {21} }, // branch index 115
	/* 36 */ { &MerlinGame::scriptPuzzle<TangramPuzzle>, 13, 0, {21} }, // branch index 116
	/* 37 */ { &MerlinGame::scriptPuzzle<MemoryPuzzle>,  11, 0, {21} }, // branch index 117
	/* 38 */ { &MerlinGame::scriptPuzzle<TangramPuzzle>, 10, 0, {21} }, // branch index 118
	/* 39 */ { &MerlinGame::scriptPuzzle<PotionPuzzle>,  16, 0, {14, 15} }, // branch index 119
	
	// Hub 3
	/* 40 */ { &MerlinGame::scriptPuzzle<SynchPuzzle>,   19, 0, {22} }, // branch index 121
	/* 41 */ { &MerlinGame::scriptPuzzle<TangramPuzzle>, 0x14, 0, {22} }, // branch index 122
	/* 42 */ { &MerlinGame::scriptPuzzle<ColorPuzzle>,   0x12, 0, {22} }, // branch index 123
	/* 43 */ { &MerlinGame::scriptPuzzle<ColorPuzzle>,   0x18, 0, {22} }, // branch index 124
	/* 44 */ { &MerlinGame::scriptPuzzle<SynchPuzzle>,   0x19, 0, {22} }, // branch index 125
	/* 45 */ { &MerlinGame::scriptPuzzle<SlidingPuzzle>, 0x16, 0, {22} }, // branch index 126
	/* 46 */ { &MerlinGame::scriptPuzzle<ActionPuzzle>,  0x15, 0, {22} }, // branch index 127
	/* 47 */ { &MerlinGame::scriptPuzzle<ActionPuzzle>,  0x1b, 0, {22} }, // branch index 128
	/* 48 */ { &MerlinGame::scriptPuzzle<TangramPuzzle>, 0x1a, 0, {22} }, // branch index 129
	/* 49 */ { &MerlinGame::scriptPuzzle<SlidingPuzzle>, 0x1c, 0, {22} }, // branch index 130
	/* 50 */ { &MerlinGame::scriptPuzzle<WordPuzzle>,    17, 0, {22} }, // branch index 131
	/* 51 */ { &MerlinGame::scriptPuzzle<MemoryPuzzle>,  0x17, 0, {22} }, // branch index 132
	/* 52 */ { &MerlinGame::scriptPuzzle<PotionPuzzle>,  0x1d, 0, {18} }, // branch index 133

	// Freeplay Hub 1
	/* 53 */ { &MerlinGame::scriptPuzzle<ActionPuzzle>,  2, 0, {8} }, // branch index 134
	/* 54 */ { &MerlinGame::scriptPuzzle<WordPuzzle>,    0, 0, {8} }, // branch index 135
	/* 55 */ { &MerlinGame::scriptPuzzle<SlidingPuzzle>, 1, 0, {8} }, // branch index 136
	/* 56 */ { &MerlinGame::scriptPuzzle<MemoryPuzzle>,  3, 0, {8} }, // branch index 137
	/* 57 */ { &MerlinGame::scriptPuzzle<ActionPuzzle>,  4, 0, {8} }, // branch index 138
	/* 58 */ { &MerlinGame::scriptPuzzle<SlidingPuzzle>, 5, 0, {8} }, // branch index 139
	/* 59 */ { &MerlinGame::scriptPuzzle<PotionPuzzle>,  6, 0, {13} }, // branch index 140

	// Freeplay Hub 2
	/* 60 */ { &MerlinGame::scriptPuzzle<SlidingPuzzle>, 8, 0, {9} }, // branch index 141
	/* 61 */ { &MerlinGame::scriptPuzzle<ActionPuzzle>,  14, 0, {9} }, // branch index 142
	/* 62 */ { &MerlinGame::scriptPuzzle<SlidingPuzzle>, 15, 0, {9} }, // branch index 143
	/* 63 */ { &MerlinGame::scriptPuzzle<ActionPuzzle>,  9, 0, {9} }, // branch index 144
	/* 64 */ { &MerlinGame::scriptPuzzle<WordPuzzle>,    7, 0, {9} }, // branch index 145
	/* 65 */ { &MerlinGame::scriptPuzzle<SynchPuzzle>,   12, 0, {9} }, // branch index 146
	/* 66 */ { &MerlinGame::scriptPuzzle<TangramPuzzle>, 13, 0, {9} }, // branch index 147
	/* 67 */ { &MerlinGame::scriptPuzzle<MemoryPuzzle>,  11, 0, {9} }, // branch index 148
	/* 68 */ { &MerlinGame::scriptPuzzle<TangramPuzzle>, 10, 0, {9} }, // branch index 149
	/* 69 */ { &MerlinGame::scriptPuzzle<PotionPuzzle>,  16, 0, {16, 17} }, // branch index 150

	// Freeplay Hub 3
	/* 70 */ { &MerlinGame::scriptPuzzle<SynchPuzzle>,   19, 0, {10} }, // branch index 152
	/* 71 */ { &MerlinGame::scriptPuzzle<TangramPuzzle>, 20, 0, {10} }, // branch index 153
	/* 72 */ { &MerlinGame::scriptPuzzle<ColorPuzzle>,   18, 0, {10} }, // branch index 154
	/* 73 */ { &MerlinGame::scriptPuzzle<ColorPuzzle>,   24, 0, {10} }, // branch index 155
	/* 74 */ { &MerlinGame::scriptPuzzle<SynchPuzzle>,   25, 0, {10} }, // branch index 156
	/* 75 */ { &MerlinGame::scriptPuzzle<SlidingPuzzle>, 22, 0, {10} }, // branch index 157
	/* 76 */ { &MerlinGame::scriptPuzzle<ActionPuzzle>,  21, 0, {10} }, // branch index 158
	/* 77 */ { &MerlinGame::scriptPuzzle<ActionPuzzle>,  0x1b, 0, {10} }, // branch index 159
	/* 78 */ { &MerlinGame::scriptPuzzle<TangramPuzzle>, 0x1a, 0, {10} }, // branch index 160
	/* 79 */ { &MerlinGame::scriptPuzzle<SlidingPuzzle>, 0x1c, 0, {10} }, // branch index 161
	/* 80 */ { &MerlinGame::scriptPuzzle<WordPuzzle>,    17, 0, {10} }, // branch index 162
	/* 81 */ { &MerlinGame::scriptPuzzle<MemoryPuzzle>,  23, 0, {10} }, // branch index 163
	/* 82 */ { &MerlinGame::scriptPuzzle<PotionPuzzle>,  0x1d, 0, {19} }, // branch index 164

	/* 83 */ { &MerlinGame::scriptPlotMovie, 0, 0, {85, 84} }, // branch index 165
	/* 84 */ { &MerlinGame::scriptPlotMovie, 0, 0, {85, 85} }, // branch index 167
	/* 85 */ { nullptr, 0, 0, {} },
};

/*
 * Original branch table:
		gMainScriptBranchTable                          XREF[1]:     merlinMain:000138c4 (*)   
		00013590 01  00  00       int[169]
				 00  01  00 
				 00  00  02 
		   00013590 [0]                      1h,            1h,            2h,            3h
		   000135a0 [4]                      3h,            6h,            4h,           53h
		   000135b0 [8]                      5h,            3h,            3h,            3h
		   000135c0 [12]                     3h,            3h,     FFFFFFFFh,            7h
		   000135d0 [16]                     3h,            6h,     FFFFFFFFh,           35h
		   000135e0 [20]                    36h,           37h,           38h,           39h
		   000135f0 [24]                    3Ah,           3Bh,            Ah,            9h
		   00013600 [28]                    3Ch,           3Dh,           3Eh,           3Fh
		   00013610 [32]                    40h,           41h,           42h,           43h
		   00013620 [36]                    44h,           45h,            8h,            Ah
		   00013630 [40]                    46h,           47h,           48h,           49h
		   00013640 [44]                    4Ah,           4Bh,           4Ch,           4Dh
		   00013650 [48]                    4Eh,           4Fh,           50h,           51h
		   00013660 [52]                    52h,            9h,            8h,           14h
		   00013670 [56]                    14h,           15h,           15h,            8h
		   00013680 [60]                     8h,           16h,           16h,           16h
		   00013690 [64]                    16h,            9h,            9h,            9h
		   000136a0 [68]                     9h,            4h,            4h,            Ah
		   000136b0 [72]                     Ah,           17h,           18h,           19h
		   000136c0 [76]                    1Ah,           1Bh,           1Ch,           1Dh
		   000136d0 [80]                    1Eh,           1Fh,           20h,           21h
		   000136e0 [84]                    22h,           23h,           24h,           25h
		   000136f0 [88]                    26h,           27h,           28h,           29h
		   00013700 [92]                    2Ah,           2Bh,           2Ch,           2Dh
		   00013710 [96]                    2Eh,           2Fh,           30h,           31h
		   00013720 [100]                   32h,           33h,           34h,           14h
		   00013730 [104]                   14h,           14h,           14h,           14h
		   00013740 [108]                   14h,            Ch,           15h,           15h
		   00013750 [112]                   15h,           15h,           15h,           15h
		   00013760 [116]                   15h,           15h,           15h,            Eh
		   00013770 [120]                    Fh,           16h,           16h,           16h
		   00013780 [124]                   16h,           16h,           16h,           16h
		   00013790 [128]                   16h,           16h,           16h,           16h
		   000137a0 [132]                   16h,           12h,            8h,            8h
		   000137b0 [136]                    8h,            8h,            8h,            8h
		   000137c0 [140]                    Dh,            9h,            9h,            9h
		   000137d0 [144]                    9h,            9h,            9h,            9h
		   000137e0 [148]                    9h,            9h,           10h,           11h
		   000137f0 [152]                    Ah,            Ah,            Ah,            Ah
		   00013800 [156]                    Ah,            Ah,            Ah,            Ah
		   00013810 [160]                    Ah,            Ah,            Ah,            Ah
		   00013820 [164]                   13h,           55h,           54h,           55h
		   00013830 [168]                   55h

*/

const uint32 MerlinGame::kWinMovies[] = {
	0,
	MKTAG('G','R','A','V'), MKTAG('L','E','A','V'), MKTAG('O','A','K','L'),
	MKTAG('P','O','N','D'), MKTAG('R','A','V','N'), MKTAG('S','E','E','D'),
	MKTAG('B','B','L','E'), MKTAG('F','L','S','K'), MKTAG('M','I','R','R'),
	MKTAG('P','R','C','H'), MKTAG('P','L','N','T'), MKTAG('P','L','A','Q'),
	MKTAG('S','K','L','T'), MKTAG('R','T','T','L'), MKTAG('S','N','O','W'),
	MKTAG('C','S','T','L'), MKTAG('D','E','M','N'), MKTAG('D','O','O','R'),
	MKTAG('G','E','M','S'), MKTAG('O','C','T','A'), MKTAG('S','P','I','D'),
	MKTAG('S','P','R','T'), MKTAG('S','T','L','C'), MKTAG('S','T','A','R'),
	MKTAG('T','B','L','T'), MKTAG('T','I','L','E'), MKTAG('W','N','D','W'),
};

const uint32 MerlinGame::kPotionMovies[] = {
	MKTAG('E','L','E','C'), MKTAG('E','X','P','L'), MKTAG('F','L','A','M'),
	MKTAG('F','L','S','H'), MKTAG('M','I','S','T'), MKTAG('O','O','Z','E'),
	MKTAG('S','H','M','R'), MKTAG('S','W','R','L'), MKTAG('W','I','N','D'),
	MKTAG('B','O','I','L'), MKTAG('B','U','B','L'), MKTAG('B','S','P','K'),
	MKTAG('F','B','R','S'), MKTAG('F','C','L','D'), MKTAG('F','F','L','S'),
	MKTAG('F','S','W','R'), MKTAG('L','A','V','A'), MKTAG('L','F','I','R'),
	MKTAG('L','S','M','K'), MKTAG('S','B','L','S'), MKTAG('S','C','L','M'),
	MKTAG('S','F','L','S'), MKTAG('S','P','R','E'), MKTAG('W','S','T','M'),
	MKTAG('W','S','W','L'), MKTAG('B','U','G','S'), MKTAG('C','R','Y','S'),
	MKTAG('D','N','C','R'), MKTAG('F','I','S','H'), MKTAG('G','L','A','C'),
	MKTAG('G','O','L','M'), MKTAG('E','Y','E','B'), MKTAG('M','O','L','E'),
	MKTAG('M','O','T','H'), MKTAG('M','U','D','B'), MKTAG('R','O','C','K'),
	MKTAG('S','H','T','R'), MKTAG('S','L','U','G'), MKTAG('S','N','A','K'),
	MKTAG('S','P','K','B'), MKTAG('S','P','K','M'), MKTAG('S','P','D','R'),
	MKTAG('S','Q','I','D'), MKTAG('C','L','O','D'), MKTAG('S','W','I','R'),
	MKTAG('V','O','L','C'), MKTAG('W','O','R','M'),
};

const int MerlinGame::kNumPotionMovies =
	sizeof(MerlinGame::kPotionMovies) / sizeof(uint32);

} // End of namespace Funhouse
