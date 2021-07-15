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

void HubCard::init(MerlinGame *game, Boltlib &boltlib, BltId resId) {
    _game = game;

	BltHub hubInfo;
	loadBltResource(hubInfo, boltlib, resId);
	loadScene(_scene, _game->getEngine(), boltlib, hubInfo.sceneId);
	_scene.loadBackPlane(boltlib, hubInfo.bgPlaneId);

	_game->setPopup(MerlinGame::kHubPopup);

	BltResourceList hubItemsList;
	loadBltResourceArray(hubItemsList, boltlib, hubInfo.itemListId);
	_items.alloc(hubInfo.numItems);
	_itemImages.alloc(hubInfo.numItems);
	for (uint i = 0; i < hubInfo.numItems; ++i) {
		loadBltResource(_items[i], boltlib, hubItemsList[i].value);
		_itemImages[i].load(boltlib, _items[i].imageId);
	}
}

void HubCard::enter() {
	if (!_game->isInMovie()) {
		// Find win movie to play
		for (int i = 0; i < _items.size(); i++) {
			if (_game->getChallengeStatus(_items[i].challengeIdx) == kPlayWinMovie) {
				_game->playWinMovie(_items[i].winMovie);
				return;
			}
		}
	}

	// Set button enablements
	for (int i = 0; i < _itemImages.size(); ++i) {
		ChallengeStatus status = _game->getChallengeStatus(_items[i].challengeIdx);
		_scene.getButton(i).setEnable(status != kWon);
	}

	_scene.enter();

	// Draw item images
	for (int i = 0; i < _itemImages.size(); ++i) {
		ChallengeStatus status = _game->getChallengeStatus(_items[i].challengeIdx);
		if (status == kWon) {
			_itemImages[i].drawAt(_game->getGraphics()->getPlaneSurface(kBack), 0, 0, true);
		}
		else if (status == kPlayWinMovie) {
			// If we got here, the movie has sent the Redraw trigger.
			// Mark the challenge as Won, but don't draw its image yet.
			_game->setChallengeStatus(_items[i].challengeIdx, kWon);
		}
	}

	_game->getGraphics()->markDirty();
}

BoltRsp HubCard::handleMsg(const BoltMsg &msg) {
    BoltRsp cmd = _game->handlePopup(msg);
    if (cmd != BoltRsp::kPass) {
        return cmd;
    }

	if (msg.type == Scene::kClickButton) {
		return handleButtonClick(msg.num);
	}

	return _scene.handleMsg(msg);
}

BoltRsp HubCard::handleButtonClick(int num) {
	if (num == -1) {
		// XXX: If no button was clicked, go to potion puzzle
        _game->branchScript(_itemImages.size());
		return BoltRsp::kDone;
	} else {
		_game->branchScript(num);
		return BoltRsp::kDone;
	}
}

} // End of namespace Funhouse
