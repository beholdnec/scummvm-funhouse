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

#include "funhouse/merlin/tangram_puzzle.h"

namespace Funhouse {

struct BltTangramPuzzleInfo {
	static const uint32 kType = kBltTangramPuzzleInfo;
	static const uint kSize = 0x2;
	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		variationSlot = src.getUint16BEAt(0x0);
	}

	uint16 variationSlot;
};

struct BltTangramPuzzleDifficultyInfo {
	static const uint32 kType = kBltTangramPuzzleDifficultyInfo;
	static const uint kSize = 0x6;
	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		numPieces = src.getUint8At(0);
		gridSpacing = src.getUint8At(1);
		offsetX = src.getInt8At(2);
		offsetY = src.getInt8At(3);
		// TODO: Unknown fields
	}

	uint8 numPieces;
	uint8 gridSpacing;
	int8 offsetX;
	int8 offsetY;
};

void TangramPuzzle::init(MerlinGame *game, Boltlib &boltlib, int challengeIdx) {
	_game = game;
	_pieceInHand = -1;

	uint16 resId = 0;
	switch (challengeIdx) {
	case 10: resId = 0x7115; break;
	case 13: resId = 0x6D15; break;
	case 20: resId = 0x7515; break;
	case 26: resId = 0x7915; break;
	default: assert(false); break;
	}

	_game->setPopup(MerlinGame::kPuzzlePopup);

	BltResourceList resourceList;
	loadBltResourceArray(resourceList, boltlib, BltShortId(resId));
	BltId difficultiesId = resourceList[0].value; // Ex: 7100
	BltId infoId         = resourceList[1].value; // Ex: 6D01
	BltId bgImageId      = resourceList[2].value;
	BltId paletteId      = resourceList[3].value;
	BltId colorCyclesId  = resourceList[4].value;

	BltTangramPuzzleInfo info;
	loadBltResource(info, boltlib, infoId);

	int difficultyLevel = _game->getDifficulty(kShapesDifficulty);
	int variation = (_game->getVariationSlot(info.variationSlot) + 1) % 4;
	debug(3, "Loading tangram puzzle difficulty %d, variation %d", difficultyLevel, variation);

	_bgImage.load(boltlib, bgImageId);
	_palette.load(boltlib, paletteId);
	loadBltResource(_colorCycles, boltlib, colorCyclesId);

	BltU16Values difficulties;
	loadBltResourceArray(difficulties, boltlib, difficultiesId);
	BltId difficultyId = BltShortId(difficulties[_game->getDifficulty(kShapesDifficulty)].value);
	// Ex: 6E5A, 6F42, 708A

	BltResourceList difficultyResources;
	loadBltResourceArray(difficultyResources, boltlib, difficultyId);
	BltId tangramDifficultyId     = difficultyResources[0].value; // Ex: 6E00
	BltId forePaletteId           = difficultyResources[1].value; // Ex: 6E01
	BltId placedImagesCatalogId   = difficultyResources[2].value; // Ex: 6E3A
	BltId unplacedImagesCatalogId = difficultyResources[3].value; // Ex: 6E3B
	BltId collisionsCatalogId     = difficultyResources[4].value; // Ex: 6E58
	BltId windowCollisionId       = difficultyResources[5].value; // Ex: 6E59

	BltTangramPuzzleDifficultyInfo difficultyInfo;
	loadBltResource(difficultyInfo, boltlib, tangramDifficultyId);
	_gridSpacing = difficultyInfo.gridSpacing;
	_offset = Common::Point(difficultyInfo.offsetX, difficultyInfo.offsetY);

	_forePalette.load(boltlib, forePaletteId);

	BltResourceList placedImagesCatalog;
	loadBltResourceArray(placedImagesCatalog, boltlib, placedImagesCatalogId);
	BltId placedImagesId = placedImagesCatalog[variation].value; // Ex: 6E32
	BltResourceList placedImagesList;
	loadBltResourceArray(placedImagesList, boltlib, placedImagesId);

	BltResourceList unplacedImagesCatalog;
	loadBltResourceArray(unplacedImagesCatalog, boltlib, unplacedImagesCatalogId);
	BltId unplacedImagesId = unplacedImagesCatalog[variation].value; // Ex: 6E36
	BltResourceList unplacedImagesList;
	loadBltResourceArray(unplacedImagesList, boltlib, unplacedImagesId);

	BltResourceList collisionsCatalog;
	loadBltResourceArray(collisionsCatalog, boltlib, collisionsCatalogId);
	BltId collisionsId = collisionsCatalog[variation].value; // Ex: 6E54
	BltResourceList collisionsList;
	loadBltResourceArray(collisionsList, boltlib, collisionsId);

	_pieceInfos.resize(difficultyInfo.numPieces);
	for (int i = 0; i < difficultyInfo.numPieces; ++i) {
		_pieceInfos[i].placedImage.load(boltlib, placedImagesList[i].value);
		_pieceInfos[i].unplacedImage.load(boltlib, unplacedImagesList[i].value);
		loadBltResourceArray(_pieceInfos[i].collision, boltlib, collisionsList[i].value);
	}

	loadBltResourceArray(_windowCollision, boltlib, windowCollisionId);

	_state = _game->getChallengeState(challengeIdx).cast<State>();
	if (!_state || _state->difficulty != difficultyLevel || _state->variation != variation) {
		_state.reset(new State());
		_game->setChallengeState(challengeIdx, _state);

		_state->difficulty = difficultyLevel;
		_state->variation = variation;
		_state->pieces.resize(difficultyInfo.numPieces);
	}
}

void TangramPuzzle::enter() {
	applyPalette(_game->getGraphics(), kBack, _palette);
	applyPalette(_game->getGraphics(), kFore, _forePalette);
	_bgImage.drawAt(_game->getGraphics()->getPlaneSurface(kBack), 0, 0, false);
	applyColorCycles(_game->getGraphics(), kBack, &_colorCycles);
	drawPieces();

	_game->getGraphics()->markDirty();
}

static int16 snap(int16 x, int spacing) {
	return x / spacing * spacing; // TODO: Refine snapping formula
}

static uint8 queryCollision(const BltU8Values& collision, int x, int y) {
	uint8 w = collision[0].value;
	uint8 h = collision[1].value;
	if (x < 0 || x >= w || y < 0 || y >= h) {
		return 5; // Empty
	}

	return collision[2 + y * w + x].value;
}

bool TangramPuzzle::pieceIsPlaceableAt(int pieceNum, int px, int py) {
	const PieceInfo& piece = _pieceInfos[pieceNum];
	uint8 width = piece.collision[0].value;
	uint8 height = piece.collision[1].value;
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			int a = queryCollision(piece.collision, x, y);
			int b = getCollisionAt(px + x, py + y);
			debug(3, "comparing collision a = %d, b = %d", a, b);
			// Collision values:
			//   0: Solid
			//   1: Upper left
			//   2: Upper right
			//   3: Lower left
			//   4: Lower right
			//   5: Empty
			if (a != 5 && b != 5 && (a + b) != 5) {
				return false;
			}
		}
	}

	return true;
}

BoltRsp TangramPuzzle::handleMsg(const BoltMsg &msg) {
	// FIXME: Is popup allowed while a piece is held?
	BoltRsp cmd = _game->handlePopup(msg);
	if (cmd != BoltRsp::kReject) {
		return cmd;
	}

	if (msg.type == BoltMsg::kClick) {
		// TODO: implement puzzle.
		if (_pieceInHand != -1) {
			// Place piece
			PieceState& p = _state->pieces[_pieceInHand];
			p.pos = msg.point - _grabPos;
			p.pos.x = snap(p.pos.x, _gridSpacing) + _offset.x;
			p.pos.y = snap(p.pos.y, _gridSpacing) + _offset.y;
			p.placed = pieceIsPlaceableAt(_pieceInHand,
				(p.pos.x - _offset.x) / _gridSpacing,
				(p.pos.y - _offset.y) / _gridSpacing);
			_pieceInHand = -1;
			drawPieces();

			if (checkWin()) {
				_game->branchWin();
				return BoltRsp::kContinue;
			}
		}
		else {
			_pieceInHand = getPieceAtPosition(msg.point);
			if (_pieceInHand != -1) {
				// Pick up piece
				// First, move the piece to be under the cursor
				// TODO: Restrict to screen
				const PieceInfo& pieceInfo = _pieceInfos[_pieceInHand];
				PieceState& pieceState = _state->pieces[_pieceInHand];
				_grabPos = Common::Point(pieceInfo.placedImage.getWidth() / 2, pieceInfo.placedImage.getHeight() / 2);
				pieceState.pos = msg.point - _grabPos;
				pieceState.pos.x = snap(pieceState.pos.x, _gridSpacing) + _offset.x;
				pieceState.pos.y = snap(pieceState.pos.y, _gridSpacing) + _offset.y;
				drawPieces();
				debug(3, "Picked up piece %d", _pieceInHand);
			}
		}

		return BoltRsp::kContinue;
	}

	if (msg.type == BoltMsg::kHover) {
		// Move piece
		if (_pieceInHand != -1) {
			// TODO: Restrict piece to a region inset from the screen.
			PieceState& p = _state->pieces[_pieceInHand];
			p.pos = msg.point - _grabPos;
			p.pos.x = snap(p.pos.x, _gridSpacing) + _offset.x;
			p.pos.y = snap(p.pos.y, _gridSpacing) + _offset.y;
			drawPieces();
			return BoltRsp::kContinue;
		}
	}

	return kPass;
}

void TangramPuzzle::handleReset() {
	_pieceInHand = -1;
	for (uint i = 0; i < _state->pieces.size(); ++i) {
		_state->pieces[i].placed = false;
	}
	drawPieces();
}

int TangramPuzzle::getPieceAtPosition(const Common::Point& pos) {
	int result = -1;

	// Loop through all pieces. Do not break early, since later pieces may
	// overlap earlier pieces. TODO: Prevent pieces from ever overlapping.
	for (int i = 0; i < _pieceInfos.size(); ++i) {
		const PieceInfo& pieceInfo = _pieceInfos[i];
		const PieceState& pieceState = _state->pieces[i];
		if (pieceState.placed) {
			if (pieceInfo.placedImage.query(pos.x - pieceState.pos.x, pos.y - pieceState.pos.y) != 0) {
				result = i;
			}
		} else {
			if (pieceInfo.unplacedImage.query(pos.x - pieceInfo.unplacedImage.getOffset().x,
				pos.y - pieceInfo.unplacedImage.getOffset().y) != 0) {
				result = i;
			}
		}
	}

	return result;
}

int TangramPuzzle::getCollisionAt(int x, int y) {
	int result = queryCollision(_windowCollision, x, y);
	if (result == 0) {
		return result;
	}

	for (int i = 0; i < _state->pieces.size(); ++i) {
		if (i == _pieceInHand || !_state->pieces[i].placed) {
			continue;
		}

		const PieceInfo& pieceInfo = _pieceInfos[i];
		const PieceState& pieceState = _state->pieces[i];
		int px = (pieceState.pos.x - _offset.x) / _gridSpacing;
		int py = (pieceState.pos.y - _offset.y) / _gridSpacing;
		int pieceCollision = queryCollision(pieceInfo.collision, x - px, y - py);
		if (result == 5) {
			result = pieceCollision;
		} else if (pieceCollision != 5) {
			result = 0;
		}
		
		if (result == 0) {
			break;
		}
	}

	return result;
}

bool TangramPuzzle::checkWin() {
	int w = _windowCollision[0].value;
	int h = _windowCollision[1].value;
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			if (getCollisionAt(x, y) != 0) {
				return false;
			}
		}
	}

	return true;
}

void TangramPuzzle::drawPieces() {
	_game->getGraphics()->clearPlane(kBack);
	_game->getGraphics()->clearPlane(kFore);

	_bgImage.drawAt(_game->getGraphics()->getPlaneSurface(kBack), 0, 0, false);

	for (int i = 0; i < _pieceInfos.size(); ++i) {
		if (i != _pieceInHand) {
			const PieceState& pieceState = _state->pieces[i];
			const PieceInfo& pieceInfo = _pieceInfos[i];
			if (pieceState.placed) {
				Common::Point imagePos = pieceState.pos - pieceInfo.placedImage.getOffset();
				pieceInfo.placedImage.drawAt(_game->getGraphics()->getPlaneSurface(kBack),
					imagePos.x, imagePos.y, true);
			} else {
				pieceInfo.unplacedImage.drawAt(_game->getGraphics()->getPlaneSurface(kBack), 0, 0, true);
			}
		}
	}

	if (_pieceInHand != -1) {
		const PieceState& pieceState = _state->pieces[_pieceInHand];
		const PieceInfo& pieceInHand = _pieceInfos[_pieceInHand];
		// The piece in hand is drawn on the foreground plane; thus, it has
		// different colors than placed pieces, which are drawn on the background plane.
		Common::Point imagePos = pieceState.pos - pieceInHand.placedImage.getOffset();
		pieceInHand.placedImage.drawAt(_game->getGraphics()->getPlaneSurface(kFore), imagePos.x, imagePos.y, true);
	}

	_game->getGraphics()->markDirty();
}

} // End of namespace Funhouse
