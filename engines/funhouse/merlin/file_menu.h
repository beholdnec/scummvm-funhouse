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

#ifndef FUNHOUSE_MERLIN_FILE_MENU_H
#define FUNHOUSE_MERLIN_FILE_MENU_H

#include "funhouse/bolt.h"
#include "funhouse/scene.h"

namespace Funhouse {

class MerlinGame;

class FileMenu : public Card {
public:
    void init(MerlinGame *game, Graphics *graphics, IBoltEventLoop *eventLoop, Boltlib &boltlib, BltId resId);
    void enter();
    BoltCmd handleMsg(const BoltMsg &msg);
private:
    BoltCmd handleButtonClick(int num);

    MerlinGame *_game;
    Scene _scene;
};

} // End of namespace Funhouse

#endif
