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

#include "funhouse/merlin/synch_puzzle.h"

namespace Funhouse {

void SynchPuzzle::init(Graphics *graphics, IBoltEventLoop *eventLoop, Boltlib &boltlib, BltId resId) {
    _graphics = graphics;

	BltResourceList resourceList;
	loadBltResourceArray(resourceList, boltlib, resId);

    BltId difficultiesId = resourceList[0].value;
    BltId sceneId = resourceList[4].value;

    BltU16Values difficultiesList;
    loadBltResourceArray(difficultiesList, boltlib, difficultiesId);

    // TODO: Select the difficulty that the player chose
    BltId difficultyId = BltShortId(difficultiesList[0].value);
    BltResourceList difficulty;
    loadBltResourceArray(difficulty, boltlib, difficultyId);

    BltId itemListId = difficulty[1].value;

    BltResourceList itemList;
    loadBltResourceArray(itemList, boltlib, itemListId);

    _items.alloc(itemList.size());
    for (uint i = 0; i < _items.size(); ++i) {
        _items[i].state = 0;
        _items[i].sprites.load(boltlib, itemList[i].value);
    }

	_scene.load(eventLoop, graphics, boltlib, sceneId);
}

void SynchPuzzle::enter() {
	_scene.enter();

    // XXX: Draw all items in all states for testing
    //for (uint i = 0; i < _items.size(); ++i) {
    //    const Item &item = _items[i];
    //    for (uint j = 0; j < item.sprites.getNumSprites(); ++j) {
    //        const Sprite &sprite = item.sprites.getSprite(j);
    //        const Common::Point &origin = _scene.getOrigin();
    //        sprite.image.drawAt(_graphics->getPlaneSurface(kFore), sprite.pos.x - origin.x, sprite.pos.y - origin.y, true);
    //    }
    //}

    for (uint i = 0; i < _items.size(); ++i) {
        const Item &item = _items[i];
        const Sprite &sprite = item.sprites.getSprite(item.state);
        const Common::Point &origin = _scene.getOrigin();
        sprite.image->drawAt(_graphics->getPlaneSurface(kFore), sprite.pos.x - origin.x, sprite.pos.y - origin.y, true);
    }
}

BoltCmd SynchPuzzle::handleMsg(const BoltMsg &msg) {
	if (msg.type == Scene::kClickButton) {
		return handleButtonClick(msg.num);
	}

	return _scene.handleMsg(msg);
}

BoltCmd SynchPuzzle::handleButtonClick(int num) {
	debug(3, "Clicked button %d", num);
	// TODO: implement puzzle
	return CardCmd(CardCmd::kWin);
}

} // End of namespace Funhouse
