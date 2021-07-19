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
	BltId bgImageId      = resourceList[2].value;
	BltId paletteId      = resourceList[3].value;
	BltId colorCyclesId  = resourceList[4].value;

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

	int puzzleVariant = 0; // TODO: Choose a random puzzle variant 0..3.

	BltResourceList placedImagesCatalog;
	loadBltResourceArray(placedImagesCatalog, boltlib, placedImagesCatalogId);
	BltId placedImagesId = placedImagesCatalog[puzzleVariant].value; // Ex: 6E32
	BltResourceList placedImagesList;
	loadBltResourceArray(placedImagesList, boltlib, placedImagesId);

	BltResourceList unplacedImagesCatalog;
	loadBltResourceArray(unplacedImagesCatalog, boltlib, unplacedImagesCatalogId);
	BltId unplacedImagesId = unplacedImagesCatalog[puzzleVariant].value; // Ex: 6E36
	BltResourceList unplacedImagesList;
	loadBltResourceArray(unplacedImagesList, boltlib, unplacedImagesId);

	BltResourceList collisionsCatalog;
	loadBltResourceArray(collisionsCatalog, boltlib, collisionsCatalogId);
	BltId collisionsId = collisionsCatalog[puzzleVariant].value; // Ex: 6E54
	BltResourceList collisionsList;
	loadBltResourceArray(collisionsList, boltlib, collisionsId);

	_pieces.alloc(difficultyInfo.numPieces);
	for (int i = 0; i < difficultyInfo.numPieces; ++i) {
		_pieces[i].placedImage.load(boltlib, placedImagesList[i].value);
		_pieces[i].unplacedImage.load(boltlib, unplacedImagesList[i].value);
		loadBltResourceArray(_pieces[i].collision, boltlib, collisionsList[i].value);
	}

	loadBltResourceArray(_windowCollision, boltlib, windowCollisionId);
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
	const Piece& piece = _pieces[pieceNum];
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
	if (cmd != BoltRsp::kPass) {
		return cmd;
	}

	if (msg.type == BoltMsg::kPopupButtonClick) {
		return handlePopupButtonClick(msg.num);
	}

	if (msg.type == BoltMsg::kClick) {
		// TODO: implement puzzle.
		if (_pieceInHand != -1) {
			// Place piece
			Piece& p = _pieces[_pieceInHand];
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
				return BoltRsp::kDone;
			}
		} else {
			_pieceInHand = getPieceAtPosition(msg.point);
			if (_pieceInHand != -1) {
				// Pick up piece
				// First, move the piece to be under the cursor
				// TODO: Restrict to screen
				Piece& p = _pieces[_pieceInHand];
				_grabPos = Common::Point(p.placedImage.getWidth() / 2, p.placedImage.getHeight() / 2);
				p.pos = msg.point - _grabPos;
				p.pos.x = snap(p.pos.x, _gridSpacing) + _offset.x;
				p.pos.y = snap(p.pos.y, _gridSpacing) + _offset.y;
				drawPieces();
				debug(3, "Picked up piece %d", _pieceInHand);
			}
		}

		return BoltRsp::kDone;
	}

	if (msg.type == BoltMsg::kHover) {
		// Move piece
		if (_pieceInHand != -1) {
			// TODO: Restrict piece to a region inset from the screen.
			Piece& p = _pieces[_pieceInHand];
			p.pos = msg.point - _grabPos;
			p.pos.x = snap(p.pos.x, _gridSpacing) + _offset.x;
			p.pos.y = snap(p.pos.y, _gridSpacing) + _offset.y;
			drawPieces();
			return BoltRsp::kDone;
		}
	}

	return BoltRsp::kDone;
}

BoltRsp TangramPuzzle::handlePopupButtonClick(int num) {
	switch (num) {
	case 0: // Return
		_game->branchReturn();
		return BoltRsp::kDone;
	default:
		warning("Unhandled popup button %d", num);
		return BoltRsp::kDone;
	}
}

int TangramPuzzle::getPieceAtPosition(const Common::Point& pos) {
	int result = -1;

	// Loop through all pieces. Do not break early, since later pieces may
	// overlap earlier pieces. TODO: Prevent pieces from ever overlapping.
	for (int i = 0; i < _pieces.size(); ++i) {
		const Piece& piece = _pieces[i];
		if (piece.placed) {
			if (piece.placedImage.query(pos.x - piece.pos.x, pos.y - piece.pos.y) != 0) {
				result = i;
			}
		} else {
			if (piece.unplacedImage.query(pos.x - piece.unplacedImage.getOffset().x,
				pos.y - piece.unplacedImage.getOffset().y) != 0) {
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

	for (int i = 0; i < _pieces.size(); ++i) {
		if (i == _pieceInHand || !_pieces[i].placed) {
			continue;
		}

		const Piece& piece = _pieces[i];
		int px = (piece.pos.x - _offset.x) / _gridSpacing;
		int py = (piece.pos.y - _offset.y) / _gridSpacing;
		int pieceCollision = queryCollision(piece.collision, x - px, y - py);
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

	for (int i = 0; i < _pieces.size(); ++i) {
		if (i != _pieceInHand) {
			const Piece& piece = _pieces[i];
			if (piece.placed) {
				Common::Point imagePos = piece.pos - piece.placedImage.getOffset();
				piece.placedImage.drawAt(_game->getGraphics()->getPlaneSurface(kBack),
					imagePos.x, imagePos.y, true);
			} else {
				piece.unplacedImage.drawAt(_game->getGraphics()->getPlaneSurface(kBack), 0, 0, true);
			}
		}
	}

	if (_pieceInHand != -1) {
		const Piece& pieceInHand = _pieces[_pieceInHand];
		// The piece in hand is drawn on the foreground plane; thus, it has
		// different colors than placed pieces, which are drawn on the background plane.
		Common::Point imagePos = pieceInHand.pos - pieceInHand.placedImage.getOffset();
		pieceInHand.placedImage.drawAt(_game->getGraphics()->getPlaneSurface(kFore), imagePos.x, imagePos.y, true);
	}

	_game->getGraphics()->markDirty();
}

} // End of namespace Funhouse
