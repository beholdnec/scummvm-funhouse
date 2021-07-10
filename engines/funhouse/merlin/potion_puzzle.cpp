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

struct BltPotionPuzzleInfo {
	static const uint32 kType = kBltPotionPuzzle;
	static const uint kSize = 0x46;
	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		difficultiesId = BltId(src.getUint32BEAt(0));
		bgImageId = BltId(src.getUint32BEAt(4));
		bgPaletteId = BltId(src.getUint32BEAt(8));
		numShelfPoints = src.getUint16BEAt(0x16);
		shelfPointsId = BltId(src.getUint32BEAt(0x18));
		// FIXME: U8Values resource specified at 0x1C has an unknown purpose.
		basinPointsId = BltId(src.getUint32BEAt(0x20));
		origin.x = src.getInt16BEAt(0x42);
		origin.y = src.getInt16BEAt(0x44);
	}

	BltId difficultiesId;
	BltId bgImageId;
	BltId bgPaletteId;
	uint16 numShelfPoints;
	BltId shelfPointsId;
	BltId basinPointsId;
	Common::Point origin;
};

struct BltPotionPuzzleSpritePointElement {
	static const uint32 kType = kBltPotionPuzzleSpritePoints;
	static const uint kSize = 0x4;
	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		pos.x = src.getInt16BEAt(0);
		pos.y = src.getInt16BEAt(2);
	}

	Common::Point pos;
};

typedef ScopedArray<BltPotionPuzzleSpritePointElement> BltPotionPuzzleSpritePoints;

struct BltPotionPuzzleDifficultyDef {
	static const uint32 kType = kBltPotionPuzzleDifficulty;
	static const uint kSize = 0xA;
	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		numIngredients = src.getUint16BEAt(0);
		ingredientImagesId = BltId(src.getUint32BEAt(2));
		comboTableListId = BltId(src.getUint32BEAt(6));
	}

	uint16 numIngredients;
	BltId ingredientImagesId;
	BltId comboTableListId;
};

struct BltPotionPuzzleComboTableListElement {
	static const uint32 kType = kBltPotionPuzzleComboTableList;
	static const uint kSize = 6;
	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		numCombos = src.getUint16BEAt(0);
		comboTableId = BltId(src.getUint32BEAt(2));
	}

	uint16 numCombos;
	BltId comboTableId;
};

typedef ScopedArray<BltPotionPuzzleComboTableListElement> BltPotionPuzzleComboTableList;

void PotionPuzzle::init(MerlinGame *game, Boltlib &boltlib, int challengeIdx) {
	_game = game;
	_mode.init(_game->getEngine());

	uint16 resId = 0;
	switch (challengeIdx) {
	case 6: resId = 0x940C; break;
	case 16: resId = 0x980C; break;
	case 29: resId = 0x9C0E; break;
	default: assert(false); break;
	}

	_popup.init(_game, boltlib, _game->getPopupResId(MerlinGame::kPotionPuzzlePopup));

	BltPotionPuzzleInfo puzzle;
	BltU16Values difficultyIds;
	BltPotionPuzzleDifficultyDef difficulty;
	BltResourceList ingredientImagesList;
	BltPotionPuzzleSpritePoints shelfPoints;
	BltPotionPuzzleSpritePoints bowlPoints;
	BltPotionPuzzleComboTableList comboTableList;

	loadBltResource(puzzle, boltlib, BltShortId(resId));
	_origin = puzzle.origin;
	_bgImage.load(boltlib, puzzle.bgImageId);
	_bgPalette.load(boltlib, puzzle.bgPaletteId);

	loadBltResourceArray(difficultyIds, boltlib, puzzle.difficultiesId);
	
	BltId difficultyId = BltShortId(difficultyIds[_game->getDifficulty(kLogicDifficulty)].value);
	loadBltResource(difficulty, boltlib, difficultyId);
	_numIngredients = difficulty.numIngredients;

	loadBltResourceArray(ingredientImagesList, boltlib, difficulty.ingredientImagesId);
	loadBltResourceArray(shelfPoints, boltlib, puzzle.shelfPointsId);
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
	
	_shelfSlotOccupied.alloc(puzzle.numShelfPoints);

	reset();
}

void PotionPuzzle::enter() {
	draw();
	if (!_game->isInMovie()) {
		evaluate();
	}
}

BoltRsp PotionPuzzle::handleMsg(const BoltMsg &msg) {
	_mode.react(msg);
	return kDone;
}

void PotionPuzzle::idle() {
	_mode.transition();
	_mode.onMsg([this](const BoltMsg& msg) {
		handleIdle(msg);
	});
}

BoltRsp PotionPuzzle::handleIdle(const BoltMsg &msg) {
	BoltRsp cmd;

	if ((cmd = _popup.handleMsg(msg)) != BoltRsp::kPass) {
		return cmd;
	}

	if (msg.type == BoltMsg::kClick) {
		return handleClick(msg.point);
	}

	return BoltRsp::kDone;
}

void PotionPuzzle::evaluate() {
	// Examine bowl to decide what action to take
	if (isValidIngredient(_bowlSlots[0]) && isValidIngredient(_bowlSlots[2])) {
		// Left and right bowl slots occupied; perform reaction
		performReaction();
		return;
	}
	
	if (isValidIngredient(_requestedIngredient)) {
		// Piece selected; move piece to bowl
		_shelfSlotOccupied[_requestedIngredient] = false;

		if (isValidIngredient(_bowlSlots[1])) {
			_bowlSlots[0] = _bowlSlots[1];
			_bowlSlots[1] = kNoIngredient;
			_bowlSlots[2] = _requestedIngredient;
		}
		else if (isValidIngredient(_bowlSlots[0])) {
			_bowlSlots[2] = _requestedIngredient;
		}
		else {
			_bowlSlots[0] = _requestedIngredient;
		}

		_requestedIngredient = kNoIngredient;

		draw();

		// TODO: Play "plunk" sound
		setTimeout(kPlacing2Time, [this]() { evaluate(); });
		_game->getEngine()->setNextMsg(BoltMsg::kDrive);
		return;
	}

	int numRemainingIngredients = getNumRemainingIngredients();
	bool bowlIsEmpty = !isValidIngredient(_bowlSlots[0]) && !isValidIngredient(_bowlSlots[1])
		&& !isValidIngredient(_bowlSlots[2]);
	if (numRemainingIngredients == 0 || (bowlIsEmpty && numRemainingIngredients == 1)) {
		// No more reactions are possible. Reset.
		reset();
		draw();
		// TODO: Play "reset" sound
		idle();
		_game->getEngine()->setNextMsg(BoltMsg::kDrive);
		return;
	}

	// No action taken
	idle();
}

BoltRsp PotionPuzzle::handleClick(Common::Point point) {
	// Eat the click event
	_game->getEngine()->setNextMsg(BoltMsg::kDrive);

	// Check if middle bowl piece was clicked. If it was clicked, undo the last action.
	if (isValidIngredient(_bowlSlots[1])) {
		const BltImage &image = _ingredientImages[_bowlSlots[1]];
		Common::Point imagePos = _bowlPoints[1] -
			Common::Point(image.getWidth() / 2, image.getHeight()) - _origin;
		// FIXME: should anchor point specified by image be ignored here?
		Common::Rect rect = image.getRect(imagePos);
		if (rect.contains(point)) {
			if (image.query(point.x - rect.left, point.y - rect.top) != 0) {
				return requestUndo();
			}
		}
	}

	// Determine which shelf piece was clicked.
	for (uint i = 0; i < _shelfPoints.size(); ++i) {
		if (_shelfSlotOccupied[i]) {
			const BltImage &image = _ingredientImages[i];
			Common::Point imagePos = _shelfPoints[i] -
				Common::Point(image.getWidth() / 2, image.getHeight()) - _origin;
			// FIXME: should anchor point specified by image be ignored here?
			Common::Rect rect = image.getRect(imagePos);
			if (rect.contains(point)) {
				if (image.query(point.x - rect.left, point.y - rect.top) != 0) {
					return requestIngredient(i);
				}
			}
		}
	}

	return BoltRsp::kDone;
}

BoltRsp PotionPuzzle::requestIngredient(int ingredient) {
	_requestedIngredient = ingredient;
	// TODO: play selection sound
	debug(3, "requested ingredient %d", ingredient);
	setTimeout(kPlacing1Time, [this]() {
		evaluate();
	});
	_game->getEngine()->setNextMsg(BoltMsg::kDrive);
	return BoltRsp::kDone;
}

BoltRsp PotionPuzzle::requestUndo() {
	// TODO
	warning("Undo not implemented");
	// XXX: win.
	_game->branchScript(0);
	return BoltRsp::kDone;
}

BoltRsp PotionPuzzle::performReaction() {
	int ingredientA = _bowlSlots[0];
	int ingredientB = _bowlSlots[2];

	assert(isValidIngredient(ingredientA) && isValidIngredient(ingredientB) && !isValidIngredient(_bowlSlots[1])
		&& "Invalid bowl state in performReaction");

	// Find reaction

	int bestMatch = -1;
	byte uvarH = 0x2;
	byte uvarL = 0x3;
	const BltPotionPuzzleComboTableElement *reactionInfo = nullptr;
	for (uint i = 0; i < _reactionTable.size(); ++i) {
		reactionInfo = &_reactionTable[i];
		debug(3, "checking reaction %d, %d, %d, %d, %d",
			(int)reactionInfo->a, (int)reactionInfo->b, (int)reactionInfo->c, (int)reactionInfo->d,
			(int)reactionInfo->movie);

		if (_bowlSlots[0] == reactionInfo->a && _bowlSlots[2] == reactionInfo->b) {
			uvarL = 0;
			bestMatch = i;
		}
		else {
			if (_bowlSlots[0] == reactionInfo->b && _bowlSlots[2] == reactionInfo->a && 1 < uvarL) {
				uvarL = 0x1;
				bestMatch = i;
				continue;
			}
			if (((((_bowlSlots[0] == reactionInfo->a) && (reactionInfo->b == -1)) ||
				((_bowlSlots[0] == reactionInfo->b && (reactionInfo->a == -1)))) && (2 < uvarL)) ||
				((((_bowlSlots[2] == reactionInfo->a && (reactionInfo->b == -1)) ||
				((_bowlSlots[2] == reactionInfo->b && (reactionInfo->a == -1)))) && (2 < uvarL))))
			{
				uvarL = uvarH;
				bestMatch = i;
			}
		}

		if (uvarL == 0) {
			break;
		}
	}

	if (bestMatch < 0) {
		warning("No reaction found for ingredients %d, %d", ingredientA, ingredientB);
		// Empty the bowl. This should never happen.
		_bowlSlots[0] = kNoIngredient;
		_bowlSlots[1] = kNoIngredient;
		_bowlSlots[2] = kNoIngredient;
		draw();
		idle();
		return BoltRsp::kDone;
	}

	// Perform reaction
	reactionInfo = &_reactionTable[bestMatch];

	if (reactionInfo->c == -1) {
		// FIXME: Does the original program check if all ingredients are used?
		_game->branchScript(0);
		return BoltRsp::kDone;
	}
	else {
		if (reactionInfo->c != (int8)0xfd) { // I don't think this is ever false...
			ingredientA = reactionInfo->c;
		}
		if (reactionInfo->d != (int8)0xfd) { // I don't think this is ever false...
			ingredientB = reactionInfo->d;
		}
		if (ingredientA == (int8)0xfe && ingredientB == (int8)0xfe) {
			_bowlSlots[0] = kNoIngredient;
			_bowlSlots[1] = kNoIngredient;
			_bowlSlots[2] = kNoIngredient;
		} else {
			if (ingredientA == (int8)0xfe) {
				_bowlSlots[0] = kNoIngredient;
				_bowlSlots[1] = ingredientB;
				_bowlSlots[2] = kNoIngredient;
			} else {
				if (ingredientB == (int8)0xfe) {
					// FIXME: Here the original saves the ingredient in slot 1 for some reason?
					_bowlSlots[0] = kNoIngredient;
					_bowlSlots[1] = ingredientA;
					_bowlSlots[2] = kNoIngredient;
				} else {
					warning("Whoops! No reaction available?");
					_bowlSlots[0] = ingredientA;
					_bowlSlots[2] = ingredientB;
				}
			}
		}

		// Start the movie, but don't redraw the puzzle. The movie sends a
		// special command to redraw the puzzle.
		_game->startPotionMovie(reactionInfo->movie);
	}

	idle();
	return BoltRsp::kDone;
}

void PotionPuzzle::reset() {
	for (uint i = 0; i < _shelfSlotOccupied.size(); ++i) {
		_shelfSlotOccupied[i] = true;
	}

	for (int i = 0; i < kNumBowlSlots; ++i) {
		_bowlSlots[i] = kNoIngredient;
	}

	_requestedIngredient = kNoIngredient;
}

void PotionPuzzle::draw() {
	applyPalette(_game->getGraphics(), kBack, _bgPalette);
	_bgImage.drawAt(_game->getGraphics()->getPlaneSurface(kBack), 0, 0, false);

	if (!_game->isInMovie()) {
		// Clear the foreground, unless we're playing a movie.
		_game->getGraphics()->clearPlane(kFore);
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
			image.drawAt(_game->getGraphics()->getPlaneSurface(kBack), pos.x, pos.y, true);
		}
	}

	if (isValidIngredient(_bowlSlots[0])) {
		// Anchor left ingredient at lower right corner
		const BltImage &image = _ingredientImages[_bowlSlots[0]];
		Common::Point pos = _bowlPoints[0] -
			Common::Point(image.getWidth(), image.getHeight()) - _origin;
		image.drawAt(_game->getGraphics()->getPlaneSurface(kBack), pos.x, pos.y, true);
	}

	if (isValidIngredient(_bowlSlots[1])) {
		// Anchor middle ingredient at lower middle point
		const BltImage &image = _ingredientImages[_bowlSlots[1]];
		Common::Point pos = _bowlPoints[1] -
			Common::Point(image.getWidth() / 2, image.getHeight()) - _origin;
		image.drawAt(_game->getGraphics()->getPlaneSurface(kBack), pos.x, pos.y, true);
	}

	if (isValidIngredient(_bowlSlots[2])) {
		// Anchor right ingredient at lower left corner
		const BltImage &image = _ingredientImages[_bowlSlots[2]];
		Common::Point pos = _bowlPoints[2] -
			Common::Point(0, image.getHeight()) - _origin;
		image.drawAt(_game->getGraphics()->getPlaneSurface(kBack), pos.x, pos.y, true);
	}

	_game->getGraphics()->markDirty();
}

bool PotionPuzzle::isValidIngredient(int ingredient) const {
	return ingredient >= 0 && ingredient < _numIngredients;
}

int PotionPuzzle::getNumRemainingIngredients() const {
	int num = 0;
	for (uint i = 0; i < _shelfSlotOccupied.size(); ++i) {
		if (_shelfSlotOccupied[i]) {
			++num;
		}
	}
	return num;
}

void PotionPuzzle::setTimeout(int32 delay, std::function<void()> then) {
	_mode.transition();
	_mode.onEnter([this, delay]() {
		_mode.startTimer(0, delay, true);
	});
	_mode.onTimer(0, [=]() {
		then();
	});
}

} // End of namespace Funhouse
