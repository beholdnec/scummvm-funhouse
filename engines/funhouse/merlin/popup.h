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

#ifndef FUNHOUSE_MERLIN_POPUP_H
#define FUNHOUSE_MERLIN_POPUP_H

#include "funhouse/bolt.h"
#include "funhouse/graphics.h"
#include "funhouse/boltlib/palette.h"
#include "funhouse/boltlib/sprites.h"

namespace Funhouse {

class Boltlib;
struct BltId;
class MerlinGame;

class Popup {
public:
    void init(MerlinGame *game, Boltlib &boltlib, BltId id);

    BoltCmd handleMsg(const BoltMsg &msg);

private:
    struct Button {
        Rect hotspot;
        BltSprites hovered;
        BltSprites unhovered;
    };
    typedef ScopedArray<Button> ButtonList;

    void activate();
    int getButtonAt(const Common::Point &pt) const;

    MerlinGame *_game;
    IBoltEventLoop *_eventLoop;
    Graphics *_graphics;

    bool _active;
    BltImage _bgImage;
    BltPalette _palette;
    ButtonList _buttons;
};

} // End of namespace Funhouse

#endif
