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

#ifndef FUNHOUSE_MERLIN_HUB_H
#define FUNHOUSE_MERLIN_HUB_H

#include "funhouse/bolt.h"
#include "funhouse/scene.h"
#include "funhouse/merlin/popup_menu.h"

namespace Funhouse {

struct CardCmd;
class MerlinEngine;
class MerlinGame;
struct HubEntry;

struct BltHubItem { // type 41
	static const uint32 kType = kBltHubItem;
	static const uint kSize = 0x10;
	void load(Common::Span<const byte> src, Boltlib& boltlib) {
		challengeIdx = src.getInt8At(0x0);
		winMovie = src.getInt8At(0x3);
		imageId = BltId(src.getUint32BEAt(0x4));
	}

	int8 challengeIdx;
	int8 winMovie;
	BltId imageId;
};

class HubCard : public Card {
public:
	void init(MerlinGame *game, Boltlib &boltlib, BltId resId);
	void enter();
	BoltRsp handleMsg(const BoltMsg &msg);
protected:
	BoltRsp handleButtonClick(int num);

private:
	MerlinGame *_game;
	Scene _scene;
	ScopedArray<BltHubItem> _items;
	ScopedArray<BltImage> _itemImages;
};

} // End of namespace Funhouse

#endif
