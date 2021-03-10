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
	_graphics = _engine->getGraphics();
	_mixer = mixer;
	_eventLoop = _engine;
	_fileNum = -1;
    _cheatMode = false;
    for (int i = 0; i < kNumDifficultyCategories; ++i) {
        _difficulties[i] = -1;
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

	_currentHubNum = -1;
	_currentPuzzleNum = -1;

	// Load cursor
	initCursor();

	// Start sequence
	resetSequence();
}

BoltRsp MerlinGame::handleMsg(const BoltMsg &msg) {
	// Play movie over anything else
	if (_movie.isRunning()) {
		return handleMsgInMovie(msg);
	} else if (_currentCard) {
		return handleMsgInCard(msg);
	}

	assert(false); // Unreachable; there must be an active movie or card
	return BoltRsp::kDone;
}

OSystem* MerlinGame::getSystem() {
	return _system;
}

FunhouseEngine* MerlinGame::getEngine() {
	return _engine;
}

Graphics* MerlinGame::getGraphics() {
	return _graphics;
}

IBoltEventLoop* MerlinGame::getEventLoop() {
    return _eventLoop;
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

int MerlinGame::getFile() const {
	return _fileNum;
}

void MerlinGame::setFile(int num) {
	assert(num >= 0 && num < kNumFiles);
	_fileNum = num;
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

void MerlinGame::resetSequence() {
	_sequenceCursor = -1;
	advanceSequence();
}

void MerlinGame::advanceSequence() {
	// Advance sequence until movie or card becomes active
	_graphics->resetColorCycles(); // XXX: keeps cycles from sticking in wrong scenes; might break something?
	_graphics->setFade(1);
	do {
		++_sequenceCursor;
		if (_sequenceCursor >= kSequenceSize) {
			_sequenceCursor = 0;
		}
		enterSequenceEntry();
	} while (!_movie.isRunning() && !_currentCard);
}

// Call pointer to member function.
// See <https://isocpp.org/wiki/faq/pointers-to-members>
#define CALL_MEMBER_FN(object, fn) ((object).*(fn))

void MerlinGame::enterSequenceEntry() {
	_currentHub = nullptr;
	_currentPuzzle = nullptr;
	const Callback &callback = kSequence[_sequenceCursor];
	CALL_MEMBER_FN(*this, callback.func)(callback.param);
}

void MerlinGame::startMainMenu(BltId id) {
	_currentCard.reset();
	MainMenu* card = new MainMenu;
	card->init(this, _boltlib, id);
	setCurrentCard(card);
}

void MerlinGame::startFileMenu(BltId id) {
    _currentCard.reset();
    FileMenu* card = new FileMenu;
    card->init(this, _boltlib, id);
    setCurrentCard(card);
}

void MerlinGame::startDifficultyMenu(BltId id) {
    _currentCard.reset();
    DifficultyMenu* card = new DifficultyMenu;
    card->init(this, _boltlib, id);
    setCurrentCard(card);
}

void MerlinGame::exitOrReturn() {
    // TODO: Implement returning from hub to main menu, etc.
    enterSequenceEntry();
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
			warning("Unhandled button %d", msg.num);
            _game->getEngine()->setMsg(Card::kEnd);
			return BoltRsp::kDone;
		}

		return _scene.handleMsg(msg);
	}

private:
    MerlinGame* _game;
	Scene _scene;
};

void MerlinGame::startMenu(BltId id) {
	_currentCard.reset();
	GenericMenuCard* menuCard = new GenericMenuCard;
	menuCard->init(this, _boltlib, id);
	setCurrentCard(menuCard);
}

void MerlinGame::startMovie(PfFile &pfFile, uint32 name) {
	// Color cycles do NOT stop when a movie starts.
	_movie.stop();
	_movie.start(_graphics, _mixer, _eventLoop, pfFile, name);
}

void MerlinGame::movieTrigger(void *param, uint16 triggerType) {
	MerlinGame *self = reinterpret_cast<MerlinGame*>(param);
	if (triggerType == 0x8002) {
		// Enter next card; used during win movies to transition back to hub card
		if (self->_currentCard) {
			self->enterCurrentCard(false);
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
		_graphics->setFade(1);
		if (_currentCard) {
			enterCurrentCard(true);
		} else {
			advanceSequence();
		}
	}

	return cmd;
}

BoltRsp MerlinGame::handleMsgInCard(const BoltMsg &msg) {
	assert(_currentCard);

	BoltRsp cmd = _currentCard->handleMsg(msg);

    BoltMsg newMsg = _engine->getMsg();
	switch (newMsg.type) {
	case Card::kEnd:
		advanceSequence();
        _engine->setMsg(BoltMsg::kDrive);
		return BoltRsp::kDone;

    case Card::kReturn:
        exitOrReturn();
        _engine->setMsg(BoltMsg::kDrive);
        return BoltRsp::kDone;

	case Card::kWin:
		win();
        _engine->setMsg(BoltMsg::kDrive);
		return BoltRsp::kDone;

	case Card::kEnterPuzzle:
		if (newMsg.num < 0 || newMsg.num >= _currentHub->numPuzzles) {
			assert(false && "Tried to enter invalid puzzle number");
		}
		puzzle(newMsg.num, &_currentHub->puzzles[newMsg.num]);
        _engine->setMsg(BoltMsg::kDrive);
		return BoltRsp::kDone;
	}

	return cmd;
}

bool MerlinGame::isPuzzleSolved(int num) const {
	return _puzzlesSolved[num];
}

int MerlinGame::getDifficulty(DifficultyCategory category) const {
    assert(category >= 0 && category < kNumDifficultyCategories);
    return _difficulties[category];
}

void MerlinGame::setDifficulty(DifficultyCategory category, int level) {
    assert(level >= 0 && level < 3);
    _difficulties[category] = level;
}

bool MerlinGame::getCheatMode() const {
    return _cheatMode;
}

void MerlinGame::setCheatMode(bool enable) {
    _cheatMode = enable;
}

void MerlinGame::redraw() {
    if (!isInMovie()) {
        assert(_currentCard);
        _currentCard->redraw();
    }
}

void MerlinGame::win() {
    if (_currentPuzzle) {
		_puzzlesSolved[_currentPuzzleNum] = true;
        _currentCard.reset();
	    startMovie(_challdirPf, _currentPuzzle->winMovie);
	    enterSequenceEntry(); // Return to hub
    } else {
        warning("Win procedure is not possible here");
		advanceSequence();
    }
}

void MerlinGame::puzzle(int num, const PuzzleEntry *entry) {
	_currentPuzzleNum = num;
	_currentCard.reset();
	_currentPuzzle = entry;
	Card *card = _currentPuzzle->puzzle(this, _boltlib, BltShortId(_currentPuzzle->resId));
	setCurrentCard(card);
}

void MerlinGame::setCurrentCard(Card *card) {
	_currentCard.reset(card);
	if (!_movie.isRunning() && _currentCard) {
		// If there is no movie playing, enter new card now
		enterCurrentCard(true);
	}
}

void MerlinGame::enterCurrentCard(bool cursorActive) {
	assert(_currentCard);
	_graphics->resetColorCycles();
	_currentCard->enter();
	if (cursorActive) {
		BoltMsg hoverMsg(BoltMsg::kHover);
		hoverMsg.point = _system->getEventManager()->getMousePos();
        _engine->setMsg(BoltMsg::kYield); // This is required to prevent accidentally repeating events
		handleMsgInCard(hoverMsg);
	}
}

void MerlinGame::plotMovie(const void *param) {
	_currentCard.reset();
	uint32 name = *reinterpret_cast<const uint32*>(param);
	startMovie(_maPf, name);
}

void MerlinGame::mainMenu(const void *param) {
	static const uint16 kMainMenuId = 0x0118;
	startMainMenu(BltShortId(kMainMenuId));
}

void MerlinGame::fileMenu(const void *param) {
	static const uint16 kFileMenuId = 0x02A0;
	startFileMenu(BltShortId(kFileMenuId));
}

void MerlinGame::difficultyMenu(const void *param) {
	static const uint16 kDifficultyMenuId = 0x006B;
	startDifficultyMenu(BltShortId(kDifficultyMenuId));
}

void MerlinGame::hub(const void *param) {
	_currentCard.reset();

	int newHubNum = reinterpret_cast<intptr_t>(param);
	const HubEntry *entry = kHubEntries[newHubNum];

	if (newHubNum != _currentHubNum) {
		// Initialize new hub

		_currentHubNum = newHubNum;

		_puzzlesSolved.resize(entry->numPuzzles);
		for (int i = 0; i < entry->numPuzzles; ++i) {
			_puzzlesSolved[i] = false;
		}
	}

	_currentHub = entry;

	HubCard *card = new HubCard;
	card->init(this, _boltlib, BltShortId(entry->hubId));
	setCurrentCard(card);
}

void MerlinGame::freeplayHub(const void *param) {
	_currentCard.reset();
	uint16 sceneId = *reinterpret_cast<const uint16*>(param);
	GenericMenuCard *card = new GenericMenuCard;
	card->init(this, _boltlib, BltShortId(sceneId));
	setCurrentCard(card);
}

void MerlinGame::potionPuzzle(const void *param) {
	_currentCard.reset();
	uint16 id = *reinterpret_cast<const uint16*>(param);
	PotionPuzzle *card = new PotionPuzzle;
	card->init(this, _eventLoop, _boltlib, BltShortId(id));
	setCurrentCard(card);
}

int MerlinGame::scriptPlotMovie(const ScriptEntry* entry) {
	// TODO
	return 0;
}

int MerlinGame::scriptPostBumper(const ScriptEntry* entry) {
	// TODO
	return 0;
}

int MerlinGame::scriptMenu(const ScriptEntry* entry) {
	// TODO
	return 0;
}

int MerlinGame::scriptHub(const ScriptEntry* entry) {
	// TODO
	return 0;
}

int MerlinGame::scriptFreeplay(const ScriptEntry* entry) {
	// TODO
	return 0;
}

int MerlinGame::scriptActionPuzzle(const ScriptEntry* entry) {
	// TODO
	return 0;
}

int MerlinGame::scriptColorPuzzle(const ScriptEntry* entry) {
	// TODO
	return 0;
}

int MerlinGame::scriptMemoryPuzzle(const ScriptEntry* entry) {
	// TODO
	return 0;
}

int MerlinGame::scriptPotionPuzzle(const ScriptEntry* entry) {
	// TODO
	return 0;
}

int MerlinGame::scriptSlidingPuzzle(const ScriptEntry* entry) {
	// TODO
	return 0;
}

int MerlinGame::scriptSynchPuzzle(const ScriptEntry* entry) {
	// TODO
	return 0;
}

int MerlinGame::scriptTangramPuzzle(const ScriptEntry* entry) {
	// TODO
	return 0;
}

int MerlinGame::scriptWordPuzzle(const ScriptEntry* entry) {
	// TODO
	return 0;
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

template<class T>
static Card* makePuzzle(MerlinGame *game, Boltlib &boltlib, BltId resId) {
	T *card = new T;
	card->init(game, boltlib, resId);
	return card;
}

static const PuzzleEntry::PuzzleFunc makeActionPuzzle = makePuzzle<ActionPuzzle>;
static const PuzzleEntry::PuzzleFunc makeWordPuzzle = makePuzzle<WordPuzzle>;
static const PuzzleEntry::PuzzleFunc makeSlidingPuzzle = makePuzzle<SlidingPuzzle>;
static const PuzzleEntry::PuzzleFunc makeMemoryPuzzle = makePuzzle <MemoryPuzzle>;
static const PuzzleEntry::PuzzleFunc makeTangramPuzzle = makePuzzle<TangramPuzzle>;
static const PuzzleEntry::PuzzleFunc makeSynchPuzzle = makePuzzle<SynchPuzzle>;
static const PuzzleEntry::PuzzleFunc makeColorPuzzle = makePuzzle<ColorPuzzle>;

const HubEntry MerlinGame::kStage1 = { 0x0C0B, 6, MerlinGame::kStage1Puzzles };
const PuzzleEntry MerlinGame::kStage1Puzzles[6] = {
	{ makeActionPuzzle,  0x4921, MKTAG('S', 'E', 'E', 'D') }, // seeds
	{ makeWordPuzzle,    0x61E3, MKTAG('G', 'R', 'A', 'V') }, // grave
	{ makeSlidingPuzzle, 0x313F, MKTAG('O', 'A', 'K', 'L') }, // oak leaf
	{ makeMemoryPuzzle,  0x865E, MKTAG('P', 'O', 'N', 'D') }, // pond
	{ makeActionPuzzle,  0x4D19, MKTAG('L', 'E', 'A', 'V') }, // leaves
	{ makeSlidingPuzzle, 0x353F, MKTAG('R', 'A', 'V', 'N') }, // raven
};

const HubEntry MerlinGame::kStage2 = { 0x0D34, 9, MerlinGame::kStage2Puzzles };
const PuzzleEntry MerlinGame::kStage2Puzzles[9] = {
	{ makeSlidingPuzzle, 0x4140, MKTAG('R', 'T', 'T', 'L') }, // rattlesnake
	{ makeTangramPuzzle, 0x6D15, MKTAG('P', 'L', 'A', 'Q') }, // plaque
	{ makeActionPuzzle,  0x551C, MKTAG('S', 'N', 'O', 'W') }, // snow
	{ makeSynchPuzzle,   0x7D12, MKTAG('P', 'L', 'N', 'T') }, // planets
	{ makeWordPuzzle,    0x69E1, MKTAG('P', 'R', 'C', 'H') }, // parchment
	{ makeActionPuzzle,  0x5113, MKTAG('B', 'B', 'L', 'E') }, // bubbles
	{ makeSlidingPuzzle, 0x3D3F, MKTAG('S', 'K', 'L', 'T') }, // skeleton
	{ makeMemoryPuzzle,  0x8797, MKTAG('F', 'L', 'S', 'K') }, // flasks
	{ makeTangramPuzzle, 0x7115, MKTAG('M', 'I', 'R', 'R') }, // mirror
};

const HubEntry MerlinGame::kStage3 = { 0x0E4F, 12, MerlinGame::kStage3Puzzles };
const PuzzleEntry MerlinGame::kStage3Puzzles[12] = {
	{ makeColorPuzzle,   0x8C13, MKTAG('W', 'N', 'D', 'W') }, // window
	{ makeTangramPuzzle, 0x7515, MKTAG('O', 'C', 'T', 'A') }, // octagon
	{ makeSynchPuzzle,   0x8512, MKTAG('S', 'P', 'R', 'T') }, // spirits
	{ makeColorPuzzle,   0x9014, MKTAG('S', 'T', 'A', 'R') }, // star
	{ makeSynchPuzzle,   0x8114, MKTAG('D', 'O', 'O', 'R') }, // door
	{ makeActionPuzzle,  0x5918, MKTAG('G', 'E', 'M', 'S') }, // gems
	{ makeSlidingPuzzle, 0x393F, MKTAG('C', 'S', 'T', 'L') }, // crystal
	{ makeActionPuzzle,  0x5D17, MKTAG('D', 'E', 'M', 'N') }, // demons
	{ makeTangramPuzzle, 0x7915, MKTAG('T', 'I', 'L', 'E') }, // tile
	{ makeSlidingPuzzle, 0x453F, MKTAG('S', 'P', 'I', 'D') }, // spider
	{ makeWordPuzzle,    0x65E1, MKTAG('T', 'B', 'L', 'T') }, // tablet
	{ makeMemoryPuzzle,  0x887B, MKTAG('S', 'T', 'L', 'C') }, // stalactites & stalagmites
};

const HubEntry* const MerlinGame::kHubEntries[] = { &MerlinGame::kStage1, &MerlinGame::kStage2, &MerlinGame::kStage3 };

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

const MerlinGame::Callback
MerlinGame::kSequence[] = {
	// Pre-game menus
	{ &MerlinGame::plotMovie, &kPlotMovieBMPR },
	{ &MerlinGame::plotMovie, &kPlotMovieINTR },
	{ &MerlinGame::mainMenu, nullptr },
	{ &MerlinGame::fileMenu, nullptr },
	{ &MerlinGame::difficultyMenu, nullptr },

	// Stage 1: Forest
	{ &MerlinGame::plotMovie, &kPlotMoviePLOG },
	{ &MerlinGame::hub, reinterpret_cast<void*>(0) },

	// Stage 2: Laboratory
	{ &MerlinGame::plotMovie, &kPlotMovieLABT },
	{ &MerlinGame::hub, reinterpret_cast<void*>(1) },

	// Stage 3: Cave
	{ &MerlinGame::plotMovie, &kPlotMovieCAV1 },
	{ &MerlinGame::hub, reinterpret_cast<void*>(2) },

	// Finale movie is hidden until the game is fully implemented. 
	//{ &MerlinGame::plotMovie, &kPlotMovieFNLE },

	{ &MerlinGame::freeplayHub, &kFreeplayScene1 },
	{ &MerlinGame::potionPuzzle, &kPotionPuzzle1 },
	{ &MerlinGame::freeplayHub, &kFreeplayScene2 },
	{ &MerlinGame::potionPuzzle, &kPotionPuzzle2 },
	{ &MerlinGame::freeplayHub, &kFreeplayScene3 },
	{ &MerlinGame::potionPuzzle, &kPotionPuzzle3 },
};

const int MerlinGame::kSequenceSize =
	sizeof(MerlinGame::kSequence) /
	sizeof(MerlinGame::Callback);

const MerlinGame::ScriptEntry
MerlinGame::kScript[] = {
	/* 0 */ { &MerlinGame::scriptPlotMovie, 0, 0 },
	/* 1 */ { &MerlinGame::scriptPostBumper, 0, 0 },
	/* 2 */ { &MerlinGame::scriptPlotMovie, 0, 0 },
	/* 3 */ { &MerlinGame::scriptMenu, 0, 0 },
	/* 4 */ { &MerlinGame::scriptPlotMovie, 0, 0 },
	/* 5 */ { &MerlinGame::scriptPlotMovie, 0, 0 },
	/* 6 */ { &MerlinGame::scriptMenu, 0, 0 },
	/* 7 */ { &MerlinGame::scriptMenu, 0, 0 },
	/* 8 */ { &MerlinGame::scriptFreeplay, 0, 0 },
	/* 9 */ { &MerlinGame::scriptFreeplay, 0, 0 },
	/* 10 */ { &MerlinGame::scriptFreeplay, 0, 0 },
	/* 11 */ { &MerlinGame::scriptPlotMovie, 0, 0 },


	/* 12 */ { &MerlinGame::scriptPlotMovie, 0, 0 },
	/* 13 */ { &MerlinGame::scriptPlotMovie, 0, 0 },
	/* 14 */ { &MerlinGame::scriptPlotMovie, 0, 0 },
	/* 15 */ { &MerlinGame::scriptPlotMovie, 0, 0 },
	/* 16 */ { &MerlinGame::scriptPlotMovie, 0, 0 },
	/* 17 */ { &MerlinGame::scriptPlotMovie, 0, 0 },
	/* 18 */ { &MerlinGame::scriptPlotMovie, 0, 0 },
	/* 19 */ { &MerlinGame::scriptPlotMovie, 0, 0 },
	/* 20 */ { &MerlinGame::scriptHub, 0, 0 },
	/* 21 */ { &MerlinGame::scriptHub, 0, 0 },
	/* 22 */ { &MerlinGame::scriptHub, 0, 0 },

	/* 23 */ { &MerlinGame::scriptActionPuzzle, 0, 0 },
	/* 24 */ { &MerlinGame::scriptWordPuzzle, 0, 0 },
	/* 25 */ { &MerlinGame::scriptSlidingPuzzle, 0, 0 },
	/* 26 */ { &MerlinGame::scriptMemoryPuzzle, 0, 0 },
	/* 27 */ { &MerlinGame::scriptActionPuzzle, 0, 0 },
	/* 28 */ { &MerlinGame::scriptSlidingPuzzle, 0, 0 },
	/* 29 */ { &MerlinGame::scriptPotionPuzzle, 0, 0 },

	/* 30 */ { &MerlinGame::scriptSlidingPuzzle, 0, 0 },
	/* 31 */ { &MerlinGame::scriptActionPuzzle, 0, 0 },
	/* 32 */ { &MerlinGame::scriptSlidingPuzzle, 0, 0 },
	/* 33 */ { &MerlinGame::scriptActionPuzzle, 0, 0 },
	/* 34 */ { &MerlinGame::scriptWordPuzzle, 0, 0 },
	/* 35 */ { &MerlinGame::scriptSynchPuzzle, 0, 0 },
	/* 36 */ { &MerlinGame::scriptTangramPuzzle, 0, 0 },
	/* 37 */ { &MerlinGame::scriptMemoryPuzzle, 0, 0 },
	/* 38 */ { &MerlinGame::scriptTangramPuzzle, 0, 0 },
	/* 39 */ { &MerlinGame::scriptPotionPuzzle, 0, 0 },

	/* 40 */ { &MerlinGame::scriptSynchPuzzle, 0, 0 },
	/* 41 */ { &MerlinGame::scriptTangramPuzzle, 0, 0 },
	/* 42 */ { &MerlinGame::scriptColorPuzzle, 0, 0 },
	/* 43 */ { &MerlinGame::scriptColorPuzzle, 0, 0 },
	/* 44 */ { &MerlinGame::scriptSynchPuzzle, 0, 0 },
	/* 45 */ { &MerlinGame::scriptSlidingPuzzle, 0, 0 },
	/* 46 */ { &MerlinGame::scriptActionPuzzle, 0, 0 },
	/* 47 */ { &MerlinGame::scriptActionPuzzle, 0, 0 },
	/* 48 */ { &MerlinGame::scriptTangramPuzzle, 0, 0 },
	/* 49 */ { &MerlinGame::scriptSlidingPuzzle, 0, 0 },
	/* 50 */ { &MerlinGame::scriptWordPuzzle, 0, 0 },
	/* 51 */ { &MerlinGame::scriptMemoryPuzzle, 0, 0 },
	/* 52 */ { &MerlinGame::scriptPotionPuzzle, 0, 0 },

	/* 53 */ { &MerlinGame::scriptActionPuzzle, 0, 0 },
	/* 54 */ { &MerlinGame::scriptWordPuzzle, 0, 0 },
	/* 55 */ { &MerlinGame::scriptSlidingPuzzle, 0, 0 },
	/* 56 */ { &MerlinGame::scriptMemoryPuzzle, 0, 0 },
	/* 57 */ { &MerlinGame::scriptActionPuzzle, 0, 0 },
	/* 58 */ { &MerlinGame::scriptSlidingPuzzle, 0, 0 },
	/* 59 */ { &MerlinGame::scriptPotionPuzzle, 0, 0 },

	/* 60 */ { &MerlinGame::scriptSlidingPuzzle, 0, 0 },
	/* 61 */ { &MerlinGame::scriptActionPuzzle, 0, 0 },
	/* 62 */ { &MerlinGame::scriptSlidingPuzzle, 0, 0 },
	/* 63 */ { &MerlinGame::scriptActionPuzzle, 0, 0 },
	/* 64 */ { &MerlinGame::scriptWordPuzzle, 0, 0 },
	/* 65 */ { &MerlinGame::scriptSynchPuzzle, 0, 0 },
	/* 66 */ { &MerlinGame::scriptTangramPuzzle, 0, 0 },
	/* 67 */ { &MerlinGame::scriptMemoryPuzzle, 0, 0 },
	/* 68 */ { &MerlinGame::scriptTangramPuzzle, 0, 0 },
	/* 69 */ { &MerlinGame::scriptPotionPuzzle, 0, 0 },

	/* 70 */ { &MerlinGame::scriptSynchPuzzle, 0, 0 },
	/* 71 */ { &MerlinGame::scriptTangramPuzzle, 0, 0 },
	/* 72 */ { &MerlinGame::scriptColorPuzzle, 0, 0 },
	/* 73 */ { &MerlinGame::scriptColorPuzzle, 0, 0 },
	/* 74 */ { &MerlinGame::scriptSynchPuzzle, 0, 0 },
	/* 75 */ { &MerlinGame::scriptSlidingPuzzle, 0, 0 },
	/* 76 */ { &MerlinGame::scriptActionPuzzle, 0, 0 },
	/* 77 */ { &MerlinGame::scriptActionPuzzle, 0, 0 },
	/* 78 */ { &MerlinGame::scriptTangramPuzzle, 0, 0 },
	/* 79 */ { &MerlinGame::scriptSlidingPuzzle, 0, 0 },
	/* 80 */ { &MerlinGame::scriptWordPuzzle, 0, 0 },
	/* 81 */ { &MerlinGame::scriptMemoryPuzzle, 0, 0 },
	/* 82 */ { &MerlinGame::scriptPotionPuzzle, 0, 0 },

	/* 83 */ { &MerlinGame::scriptPlotMovie, 0, 0 },
	/* 84 */ { &MerlinGame::scriptPlotMovie, 0, 0 },
};

const int MerlinGame::kScriptLength = sizeof(MerlinGame::kScript) / sizeof(MerlinGame::ScriptEntry);

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
