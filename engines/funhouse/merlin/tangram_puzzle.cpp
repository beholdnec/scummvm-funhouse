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

void TangramPuzzle::init(MerlinGame *game, Boltlib &boltlib, BltId resId) {
    _game = game;
	_graphics = _game->getGraphics();
    _pieceInHand = -1;

    _popup.init(_game->getEventLoop(), _graphics, boltlib,
        _game->getPopupResId(MerlinGame::kPuzzlePopup));

	BltResourceList resourceList;
	loadBltResourceArray(resourceList, boltlib, resId);
    BltId difficultiesId = resourceList[0].value;
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
    BltId forePaletteId = difficultyResources[1].value; // Ex: 6E01
    BltId images1Id     = difficultyResources[2].value; // Ex: 6E3A

    _forePalette.load(boltlib, forePaletteId);

    BltResourceList images1List;
    loadBltResourceArray(images1List, boltlib, images1Id);
    BltId images1_1Id = images1List[0].value; // Ex: 6E32

    BltResourceList images1_1List;
    loadBltResourceArray(images1_1List, boltlib, images1_1Id);

    _pieces.alloc(images1_1List.size()); // TODO: Use #-of-pieces value in difficulty info
    for (int i = 0; i < images1_1List.size(); ++i) {
        _pieces[i].image.load(boltlib, images1_1List[i].value);
    }
}

void TangramPuzzle::enter() {
	applyPalette(_graphics, kBack, _palette);
    applyPalette(_graphics, kFore, _forePalette);
	_bgImage.drawAt(_graphics->getPlaneSurface(kBack), 0, 0, false);
	applyColorCycles(_graphics, kBack, &_colorCycles);
    drawPieces();

	_graphics->markDirty();
}

BoltCmd TangramPuzzle::handleMsg(const BoltMsg &msg) {
    BoltCmd cmd = _popup.handleMsg(msg);
    if (cmd.type != BoltCmd::kPass) {
        return cmd;
    }

	if (msg.type == BoltMsg::kClick) {
		// TODO: implement puzzle.
        if (_pieceInHand != -1) {
            // Place piece
            _pieces[_pieceInHand].pos.x = _initialPiecePos.x + msg.point.x - _initialHandPos.x;
            _pieces[_pieceInHand].pos.y = _initialPiecePos.y + msg.point.y - _initialHandPos.y;
            _pieceInHand = -1;
            drawPieces();
        } else {
            _pieceInHand = getPieceAtPosition(msg.point);
            if (_pieceInHand != -1) {
                // Pick up piece
                _initialHandPos = msg.point;
                _initialPiecePos = _pieces[_pieceInHand].pos;
                debug(3, "Picked up piece %d", _pieceInHand);
            }
        }

		return BoltCmd::kDone;
	}

    if (msg.type == BoltMsg::kHover) {
        // Move piece. TODO: Pieces should snap to a coarse grid.
        if (_pieceInHand != -1) {
            _pieces[_pieceInHand].pos.x = _initialPiecePos.x + msg.point.x - _initialHandPos.x;
            _pieces[_pieceInHand].pos.y = _initialPiecePos.y + msg.point.y - _initialHandPos.y;
            drawPieces();
            return BoltCmd::kDone;
        }
    }

    if (msg.type == BoltMsg::kRightClick) {
        // Win instantly. TODO: remove.
        // The win condition is when all pieces have been placed such that their sprites are
        // anchored at position 0, 0.
        return Card::kWin;
    }

	return BoltCmd::kDone;
}

int TangramPuzzle::getPieceAtPosition(const Common::Point& pos) {
    int result = -1;

    // Loop through all pieces. Do not break early, since later pieces may
    // overlap earlier pieces.
    for (int i = 0; i < _pieces.size(); ++i) {
        const Piece& piece = _pieces[i];
        if (piece.image.query(
            pos.x - piece.pos.x - piece.image.getOffset().x,
            pos.y - piece.pos.y - piece.image.getOffset().y) != 0) {
            result = i;
        }
    }

    return result;
}

void TangramPuzzle::drawPieces() {
    _graphics->clearPlane(kFore);

    for (int i = 0; i < _pieces.size(); ++i) {
        if (i != _pieceInHand) {
            const Piece& piece = _pieces[i];
            piece.image.drawAt(_graphics->getPlaneSurface(kFore), piece.pos.x, piece.pos.y, true);
        }
    }

    if (_pieceInHand != -1) {
        const Piece& pieceInHand = _pieces[_pieceInHand];
        pieceInHand.image.drawAt(_graphics->getPlaneSurface(kFore), pieceInHand.pos.x, pieceInHand.pos.y, true);
    }

    _graphics->markDirty();
}

} // End of namespace Funhouse
