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

#include "funhouse/merlin/potion_puzzle.h"

namespace Funhouse {

struct BltPotionPuzzleDef {
	static const uint32 kType = kBltPotionPuzzle;
	static const uint kSize = 0x46;
	void load(const ConstSizedDataView<kSize> src, Boltlib &boltlib) {
		difficultiesId = BltId(src.readUint32BE(0));
		bgImageId = BltId(src.readUint32BE(4));
		bgPaletteId = BltId(src.readUint32BE(8));
		numShelfPoints = src.readUint16BE(0x16);
		shelfPointsId = BltId(src.readUint32BE(0x18));
		ingredientNumsId = BltId(src.readUint32BE(0x1C));
		basinPointsId = BltId(src.readUint32BE(0x20));
		origin.x = src.readInt16BE(0x42);
		origin.y = src.readInt16BE(0x44);
	}

	BltId difficultiesId;
	BltId bgImageId;
	BltId bgPaletteId;
	uint16 numShelfPoints;
	BltId shelfPointsId;
	BltId ingredientNumsId; // Maps shelf ingredient numbers to puzzle object numbers
	BltId basinPointsId;
	Common::Point origin;
};

struct BltPotionPuzzleSpritePointElement {
	static const uint32 kType = kBltPotionPuzzleSpritePoints;
	static const uint kSize = 0x4;
	void load(const ConstSizedDataView<kSize> src, Boltlib &boltlib) {
		pos.x = src.readInt16BE(0);
		pos.y = src.readInt16BE(2);
	}

	Common::Point pos;
};

typedef ScopedArray<BltPotionPuzzleSpritePointElement> BltPotionPuzzleSpritePoints;

struct BltPotionPuzzleDifficultyDef {
	static const uint32 kType = kBltPotionPuzzleDifficulty;
	static const uint kSize = 0xA;
	void load(const ConstSizedDataView<kSize> src, Boltlib &boltlib) {
		numIngredients = src.readUint16BE(0);
		ingredientImagesId = BltId(src.readUint32BE(2));
		comboTableListId = BltId(src.readUint32BE(6));
	}

	uint16 numIngredients;
	BltId ingredientImagesId;
	BltId comboTableListId;
};

struct BltPotionPuzzleComboTableListElement {
	static const uint32 kType = kBltPotionPuzzleComboTableList;
	static const uint kSize = 6;
	void load(const ConstSizedDataView<kSize> src, Boltlib &boltlib) {
		numCombos = src.readUint16BE(0);
		comboTableId = BltId(src.readUint32BE(2));
	}

	uint16 numCombos;
	BltId comboTableId;
};

typedef ScopedArray<BltPotionPuzzleComboTableListElement> BltPotionPuzzleComboTableList;

void PotionPuzzle::init(MerlinGame *game, IBoltEventLoop *eventLoop, Boltlib &boltlib, BltId resId) {
	_game = game;
	_eventLoop = eventLoop;
	_graphics = _game->getGraphics();

	BltPotionPuzzleDef puzzle;
	BltU16Values difficultyIds;
	BltPotionPuzzleDifficultyDef difficulty;
	BltResourceList ingredientImagesList;
	BltPotionPuzzleSpritePoints shelfPoints;
	BltPotionPuzzleSpritePoints bowlPoints;
	BltPotionPuzzleComboTableList comboTableList;

	loadBltResource(puzzle, boltlib, resId);
	_origin = puzzle.origin;
	_bgImage.load(boltlib, puzzle.bgImageId);
	_bgPalette.load(boltlib, puzzle.bgPaletteId);

	loadBltResourceArray(difficultyIds, boltlib, puzzle.difficultiesId);
	
	BltId difficultyId = BltShortId(difficultyIds[0].value); // TODO: Use player's chosen difficulty
	loadBltResource(difficulty, boltlib, difficultyId);
	_numIngredients = difficulty.numIngredients;

	loadBltResourceArray(ingredientImagesList, boltlib, difficulty.ingredientImagesId);
	loadBltResourceArray(shelfPoints, boltlib, puzzle.shelfPointsId);
	loadBltResourceArray(_ingredientNums, boltlib, puzzle.ingredientNumsId);
	loadBltResourceArray(bowlPoints, boltlib, puzzle.basinPointsId);
	loadBltResourceArray(comboTableList, boltlib, difficulty.comboTableListId); // TODO: which combo table should we choose?

	// XXX: Although there are multiple reaction tables, it is unclear how to select which one is
	//      used. In the original program, it has something to do with the value at 0x14 in the
	//      puzzle descriptor, combined with a value from the save data.
	loadBltResourceArray(_reactionTable, boltlib, comboTableList[0].comboTableId);

	_ingredientImages.alloc(difficulty.numIngredients);
	for (uint16 i = 0; i < difficulty.numIngredients; ++i) {
		_ingredientImages[i].load(boltlib, ingredientImagesList[i].value);
	}

	_shelfPoints.alloc(puzzle.numShelfPoints);
	for (uint16 i = 0; i < puzzle.numShelfPoints; ++i) {
		_shelfPoints[i] = shelfPoints[i].pos;
	}

	// TODO: clean up seemingly redundant / unnecessarily complex code
	if (bowlPoints.size() != kNumBowlPoints) {
		error("Invalid number of bowl points %d", bowlPoints.size());
	}
	for (int i = 0; i < kNumBowlPoints; ++i) {
		_bowlPoints[i] = bowlPoints[i].pos;
	}
	
	_mode = kWaitForPlayer;
	_timeoutActive = false;
	
	_shelfSlotOccupied.alloc(puzzle.numShelfPoints);
	for (int i = 0; i < puzzle.numShelfPoints; ++i) {
		_shelfSlotOccupied[i] = true;
	}

	for (int i = 0; i < kNumBowlSlots; ++i) {
		_bowlSlots[i] = kNoIngredient;
	}

	_requestedIngredient = kNoIngredient;
}

void PotionPuzzle::enter() {
	draw();
}

BoltCmd PotionPuzzle::handleMsg(const BoltMsg &msg) {
	switch (_mode) {
	case kWaitForPlayer:
		return driveWaitForPlayer(msg);
	case kTransition:
		return driveTransition(msg);
	default:
		assert(false && "Invalid potion puzzle mode");
		return BoltCmd::kDone;
	}
}

void PotionPuzzle::enterWaitForPlayerMode() {
	// TODO: show cursor
	_mode = kWaitForPlayer;
}

void PotionPuzzle::enterTransitionMode() {
	// TODO: hide cursor
	_mode = kTransition;
}

BoltCmd PotionPuzzle::driveWaitForPlayer(const BoltMsg &msg) {
	if (msg.type == BoltMsg::kClick) {
		return handleClick(msg.point);
	}

	if (msg.type == BoltMsg::kRightClick) {
		// Right-click to win instantly.
		// TODO: remove.
		return Card::kEnd;
	}

	// Event was not handled.
	return BoltCmd::kDone;
}

BoltCmd PotionPuzzle::driveTransition(const BoltMsg &msg) {
	// TODO: Eliminate Drive events. Transitions should be driven primarily by Timer and AudioEnded,
	// once those event types are implemented.
	if (msg.type != BoltMsg::kDrive) {
		return BoltCmd::kDone;
	}

	if (_timeoutActive) {
		return driveTimeout(msg);
	}

	// Examine state to decide what action to take

	if (isValidIngredient(_bowlSlots[0]) && isValidIngredient(_bowlSlots[1])) {
		// Left and right bowl slots occupied; perform reaction
		return performReaction();
	}
	
	if (isValidIngredient(_requestedIngredient)) {
		// Piece selected; move piece to bowl
		_shelfSlotOccupied[_requestedIngredient] = false;

		_bowlSlots[1] = _bowlSlots[0];
		_bowlSlots[0] = _requestedIngredient;

		_requestedIngredient = kNoIngredient;

		draw();

		// TODO: Play "plunk" sound
		setTimeout(kPlacing2Time);
		return BoltCmd::kResend;
	}

	// No action taken; change to WaitForPlayer mode
	enterWaitForPlayerMode();
	return BoltCmd::kResend;
}

BoltCmd PotionPuzzle::driveTimeout(const BoltMsg &msg) {
	const uint32 delta = _eventLoop->getEventTime() - _timeoutStart;
	if (delta >= _timeoutLength) {
		_timeoutActive = false;
		return BoltCmd::kResend;
	}

	return BoltCmd::kDone;
}

BoltCmd PotionPuzzle::handleClick(Common::Point point) {
	// Eat the click event
	_eventLoop->setMsg(BoltMsg::kDrive);

	// Check if middle bowl piece was clicked. If it was clicked, undo the last action.
	if (isValidIngredient(_bowlSlots[1])) {
		const BltImage &image = _ingredientImages[_bowlSlots[1]];
		Common::Point imagePos = _bowlPoints[1] -
			Common::Point(image.getWidth() / 2, image.getHeight()) - _origin;
		// FIXME: should anchor point specified by image be ignored here?
		Common::Rect rect = image.getRect(imagePos);
		if (rect.contains(point)) {
			if (image.query(point.x - rect.left, point.y - rect.top)) {
				return requestUndo();
			}
		}
	}

	// Determine which shelf piece was clicked.
	for (uint i = 0; i < _shelfPoints.size(); ++i) {
		if (_shelfSlotOccupied[i]) {
			// TODO: look up image in ingredient id -> ingredient image table?
			const BltImage &image = _ingredientImages[i];
			Common::Point imagePos = _shelfPoints[i] -
				Common::Point(image.getWidth() / 2, image.getHeight()) - _origin;
			// FIXME: should anchor point specified by image be ignored here?
			Common::Rect rect = image.getRect(imagePos);
			if (rect.contains(point)) {
				if (image.query(point.x - rect.left, point.y - rect.top)) {
					return requestIngredient(i);
				}
			}
		}
	}

	return BoltCmd::kDone;
}

int PotionPuzzle::getIngredientNum(int ingredient) const {
	// XXX: The _ingredientNums table seems to be ignored in the original.
	//      More investigation is needed.
	//if (ingredient >= 0 && ingredient < _ingredientNums.size()) {
	//	return _ingredientNums[ingredient].value;
	//}
	return ingredient;
}

BoltCmd PotionPuzzle::requestIngredient(int ingredient) {
	_requestedIngredient = ingredient;
	// TODO: play selection sound
	setTimeout(kPlacing1Time);
	enterTransitionMode();
	return BoltCmd::kResend;
}

BoltCmd PotionPuzzle::requestUndo() {
	// TODO
	warning("Undo not implemented");
	return BoltCmd::kDone;
}

BoltCmd PotionPuzzle::performReaction() {
	const int ingredientA = getIngredientNum(_bowlSlots[0]);
	const int ingredientB = getIngredientNum(_bowlSlots[1]);

	assert(isValidIngredient(ingredientA) && isValidIngredient(ingredientB) &&
		"Bowl slot 0 and 2 must be occupied");

	uint reactionNum = 0;
	const BltPotionPuzzleComboTableElement *reactionInfo = nullptr;

	// FIXME: how do reactions actually work? I don't know how to interpret the puzzle data.

	// Find reaction
	// NOTE: Sometimes, a different movie will play depending on which ingredients are on the left and
	//       right. The final object is the same, though.
	//       Example: In the stump puzzle (easy difficulty), when combining the rock and acorn, the SHMR
	//       movie plays if the acorn is on the left. Otherwise, the OOZE movie plays. Both reactions result
	//       in gravel.
	for (reactionNum = 0; reactionNum < _reactionTable.size(); ++reactionNum) {
		reactionInfo = &_reactionTable[reactionNum]; // TODO: rename comboTable to reactionTable
		debug(3, "checking reaction %d, %d, %d, %d, %d",
			(int)reactionInfo->a, (int)reactionInfo->b, (int)reactionInfo->c, (int)reactionInfo->d,
			(int)reactionInfo->movie);
		// -1 is "wildcard". If -1 is found in the reaction table, it matches any ingredient (?)
		// TODO: I believe c=-1, d=-1 signifies the Win condition. Maybe.
		if (ingredientA == reactionInfo->a && ingredientB == reactionInfo->b) {
			break;
		}
		if (reactionInfo->a == -1 && ingredientB == reactionInfo->b) {
			break;
		}
		//if (ingredientA == reactionInfo->b && ingredientB == reactionInfo->a) {
		//	break;
		//}
	}

	if (reactionNum >= _reactionTable.size()) {
		warning("No reaction found for ingredients %d, %d", ingredientA, ingredientB);
		// Empty the bowl. This should never happen.
		_bowlSlots[0] = kNoIngredient;
		_bowlSlots[1] = kNoIngredient;
		draw();
		return BoltCmd::kDone;
	}

	// Perform reaction
	// FIXME: what should we do here? What does reactionInfo->c mean, if anything?
	_bowlSlots[0] = reactionInfo->d;
	_bowlSlots[1] = kNoIngredient;
	// NOTE: The game doesn't redraw puzzle until midway through the movie. The movie
	//       sends a special redraw command.
	_game->startPotionMovie(reactionInfo->movie);

	return BoltCmd::kDone;
}

void PotionPuzzle::draw() {
	applyPalette(_graphics, kBack, _bgPalette);
	_bgImage.drawAt(_graphics->getPlaneSurface(kBack), 0, 0, false);

	if (!_game->isInMovie()) {
		// Clear the foreground, unless we're playing a movie.
		_graphics->clearPlane(kFore);
	}

	for (uint i = 0; i < _shelfPoints.size(); ++i) {
		// FIXME: can different ingredients be placed on the shelf? i.e. can the ingredient index be
		//        different from the shelf-slot index?
		if (_shelfSlotOccupied[i]) {
			// Draw ingredient on shelf, anchored at south point of image
			const BltImage &image = _ingredientImages[i];
			Common::Point pos = _shelfPoints[i] -
				Common::Point(image.getWidth() / 2, image.getHeight()) - _origin;
			// FIXME: is image-specified anchor point ignored here?
			image.drawAt(_graphics->getPlaneSurface(kBack), pos.x, pos.y, true);
		}
	}

	if (isValidIngredient(_bowlSlots[0]) && isValidIngredient(_bowlSlots[1])) {
		// Draw ingredients at bowl points 0 and 2
		{
			// Anchor left ingredient at southeast corner
			const BltImage &image = _ingredientImages[_bowlSlots[0]];
			Common::Point pos = _bowlPoints[0] -
				Common::Point(image.getWidth(), image.getHeight()) - _origin;
			_ingredientImages[_bowlSlots[0]].drawAt(_graphics->getPlaneSurface(kBack), pos.x, pos.y, true);
		}
		{

			// Anchor right ingredient at southwest corner
			const BltImage &image = _ingredientImages[_bowlSlots[1]];
			Common::Point pos = _bowlPoints[2] -
				Common::Point(0, image.getHeight()) - _origin;
			image.drawAt(_graphics->getPlaneSurface(kBack), pos.x, pos.y, true);
		}
	}
	else if (isValidIngredient(_bowlSlots[0])) {
		// Draw ingredient at bowl point 1, anchored at south point of image
		const BltImage &image = _ingredientImages[_bowlSlots[0]];
		Common::Point pos = _bowlPoints[1] -
			Common::Point(image.getWidth() / 2, image.getHeight()) - _origin;
		image.drawAt(_graphics->getPlaneSurface(kBack), pos.x, pos.y, true);
	}
	else {
		// Check for sanity
		assert(!isValidIngredient(_bowlSlots[0]) && !isValidIngredient(_bowlSlots[1]) &&
			"Invalid bowl state");
	}

	_graphics->markDirty();
}

bool PotionPuzzle::isValidIngredient(int ingredient) const {
	return ingredient >= 0 && ingredient < _numIngredients;
}

void PotionPuzzle::setTimeout(uint32 length) {
	_timeoutStart = _eventLoop->getEventTime();
	_timeoutLength = length;
	_timeoutActive = true;
}

} // End of namespace Funhouse
