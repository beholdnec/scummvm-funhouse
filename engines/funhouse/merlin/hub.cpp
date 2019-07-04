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

#include "funhouse/merlin/hub.h"

#include "funhouse/merlin/merlin.h"

namespace Funhouse {

struct BltHub { // type 40
	static const uint32 kType = kBltHub;
	static const uint kSize = 0x10;
	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		sceneId = BltId(src.getUint32BEAt(0));
		// FIXME: unknown field at offset 4
		bgPlaneId = BltId(src.getUint32BEAt(6));
		// FIXME: unknown field at offset 0xA
		numItems = src.getUint8At(0xB);
		itemListId = BltId(src.getUint32BEAt(0xC));
	}

	BltId sceneId;
	BltId bgPlaneId;
	byte numItems;
	BltId itemListId;
};

struct BltHubItem { // type 41
	static const uint32 kType = kBltHubItem;
	static const uint kSize = 0x10;
	void load(Common::Span<const byte> src, Boltlib &boltlib) {
		// FIXME: unknown fields
		imageId = BltId(src.getUint32BEAt(4));
	}

	BltId imageId;
};

void HubCard::init(MerlinGame *game, Boltlib &boltlib, BltId resId) {
    _game = game;
	_graphics = _game->getGraphics();

	BltHub hubInfo;
	loadBltResource(hubInfo, boltlib, resId);
	_scene.load(_game->getEventLoop(), _graphics, boltlib, hubInfo.sceneId);
	_scene.setBackPlane(boltlib, hubInfo.bgPlaneId);

    _popup.init(_game, boltlib, _game->getPopupResId(MerlinGame::kHubPopup));

	BltResourceList hubItemsList;
	loadBltResourceArray(hubItemsList, boltlib, hubInfo.itemListId);
	_itemImages.alloc(hubInfo.numItems);
	for (uint i = 0; i < hubInfo.numItems; ++i) {
		BltHubItem hubItem;
		loadBltResource(hubItem, boltlib, hubItemsList[i].value);
		_itemImages[i].load(boltlib, hubItem.imageId);
	}
}

void HubCard::enter() {
	_scene.enter();

	// Draw item images to back plane
	// FIXME: Only draw items that are unlocked.
	for (uint i = 0; i < _itemImages.size(); ++i) {
		_itemImages[i].drawAt(_graphics->getPlaneSurface(kBack), 0, 0, true);
	}

    _graphics->markDirty();
}

BoltCmd HubCard::handleMsg(const BoltMsg &msg) {
    BoltCmd cmd = _popup.handleMsg(msg);
    if (cmd.type != BoltCmd::kPass) {
        return cmd;
    }

	if (msg.type == Scene::kClickButton) {
		return handleButtonClick(msg.num);
	}

	return _scene.handleMsg(msg);
}

BoltCmd HubCard::handleButtonClick(int num) {
	if (num == -1) {
		// If no button was clicked, complete stage and transition to next hub.
		return Card::kEnd;
	} else {
		BoltCmd cmd(Card::kEnterPuzzle);
		cmd.num = num;
		return cmd;
	}
}

} // End of namespace Funhouse
