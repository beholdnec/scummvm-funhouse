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
		// TODO: Unknown fields
	}

	uint8 numPieces;
	uint8 gridSpacing;
};

void TangramPuzzle::init(MerlinGame *game, Boltlib &boltlib, BltId resId) {
    _game = game;
	_graphics = _game->getGraphics();
    _pieceInHand = -1;

    _popup.init(_game, boltlib, _game->getPopupResId(MerlinGame::kPuzzlePopup));

	BltResourceList resourceList;
	loadBltResourceArray(resourceList, boltlib, resId);
    BltId difficultiesId = resourceList[0].value; // Ex: 7100
	BltId bgImageId      = resourceList[2].value;
	BltId paletteId      = resourceList[3].value;
	BltId colorCyclesId  = resourceList[4].value;

	_bgImage.load(boltlib, bgImageId);
	_palette.load(boltlib, paletteId);
	loadBltResource(_colorCycles, boltlib, colorCyclesId);

    BltU16Values difficulties;
    loadBltResourceArray(difficulties, boltlib, difficultiesId);
    BltId difficultyId = BltShortId(difficulties[0].value); // TODO: Choose based on player settings
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
	applyPalette(_graphics, kBack, _palette);
    applyPalette(_graphics, kFore, _forePalette);
	_bgImage.drawAt(_graphics->getPlaneSurface(kBack), 0, 0, false);
	applyColorCycles(_graphics, kBack, &_colorCycles);
    drawPieces();

	_graphics->markDirty();
}

static int16 snap(int16 x, int spacing) {
	return x / spacing * spacing; // TODO: Refine snapping formula
}

BoltCmd TangramPuzzle::handleMsg(const BoltMsg &msg) {
	// FIXME: Is popup allowed while a piece is held?
    BoltCmd cmd = _popup.handleMsg(msg);
    if (cmd.type != BoltCmd::kPass) {
        return cmd;
    }

	if (msg.type == BoltMsg::kClick) {
		// TODO: implement puzzle.
        if (_pieceInHand != -1) {
            // Place piece
			Piece& p = _pieces[_pieceInHand];
			p.pos = msg.point - _grabPos;
			p.pos.x = snap(p.pos.x, _gridSpacing);
			p.pos.y = snap(p.pos.y, _gridSpacing);
            _pieceInHand = -1;
			// XXX: toggle between placed and unplaced.
			// TODO: Piece should be set to the placed state if it is dropped entirely within the window;
			//       otherwise, it should be sent back to its original position in the unplaced state.
			p.placed = !p.placed;
            drawPieces();
        } else {
            _pieceInHand = getPieceAtPosition(msg.point);
            if (_pieceInHand != -1) {
                // Pick up piece
				_grabPos = msg.point - _pieces[_pieceInHand].pos;
				drawPieces();
                debug(3, "Picked up piece %d", _pieceInHand);
            }
        }

		return BoltCmd::kDone;
	}

    if (msg.type == BoltMsg::kHover) {
        // Move piece
        if (_pieceInHand != -1) {
			// TODO: Restrict piece to a region inset from the screen.
			Piece& p = _pieces[_pieceInHand];
			p.pos = msg.point - _grabPos;
			p.pos.x = snap(p.pos.x, _gridSpacing);
			p.pos.y = snap(p.pos.y, _gridSpacing);
            drawPieces();
            return BoltCmd::kDone;
        }
    }

	return BoltCmd::kDone;
}

static uint8 queryCollision(const BltU8Values& collision, int x, int y) {
	uint8 w = collision[0].value;
	uint8 h = collision[1].value;
	if (x < 0 || x >= w || y < 0 || y >= h) {
		return 0;
	}

	return collision[2 + y * w + x].value;
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

void TangramPuzzle::drawPieces() {
	_graphics->clearPlane(kBack);
    _graphics->clearPlane(kFore);

	_bgImage.drawAt(_graphics->getPlaneSurface(kBack), 0, 0, false);

    for (int i = 0; i < _pieces.size(); ++i) {
        if (i != _pieceInHand) {
            const Piece& piece = _pieces[i];
			if (piece.placed) {
				Common::Point imagePos = piece.pos - piece.placedImage.getOffset();
				piece.placedImage.drawAt(_graphics->getPlaneSurface(kBack),
					imagePos.x, imagePos.y, true);
			} else {
				piece.unplacedImage.drawAt(_graphics->getPlaneSurface(kBack), 0, 0, true);
			}
        }
    }

    if (_pieceInHand != -1) {
        const Piece& pieceInHand = _pieces[_pieceInHand];
		// The piece in hand is drawn on the foreground plane; thus, it has
		// different colors than placed pieces, which are drawn on the background plane.
		Common::Point imagePos = pieceInHand.pos - pieceInHand.placedImage.getOffset();
        pieceInHand.placedImage.drawAt(_graphics->getPlaneSurface(kFore), imagePos.x, imagePos.y, true);
    }

    _graphics->markDirty();
}

} // End of namespace Funhouse
