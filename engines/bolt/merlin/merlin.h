/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef MERLIN_MERLIN_H
#define MERLIN_MERLIN_H

#include "bolt/bolt.h"

namespace Bolt {

namespace Merlin {

struct Scene;

class MerlinEngine : public BoltEngine {
public:
	MerlinEngine(OSystem *syst, const ADGameDescription *gameDesc);
	
protected:
	// xpMain
	void boltMain() override;

	void initCallbacks() override;
	
	BOLTLib *_boltlib = nullptr;
	BOLTCallbacks _boltCallbacks;

	// Scenes
	Scene *loadScene(const byte* bltScene);
	void drawScene(const Scene *scene, byte flags);
	void drawSceneBackground(const byte *bltScene, byte plane);
	void updateSceneButtons(Scene* scene, int x, int y);
	void drawSceneButton(const byte *buttonGfx, uint16 plane);
	void applyPaletteMod(const byte *bltPaletteMod, byte startBase);
	void swapPlaneDesc(); // Type 26
	void swapPaletteModDesc(); // Type 29
	void swapButtonGfxDesc(); // Type 30
	void swapButtonDesc(); // Type 31
	void swapSceneDesc(); // Type 32
	
	static void swapPlaneDescCb(); // Type 26
	static void swapPaletteModDescCb(); // Type 29
	static void swapButtonGfxDescCb(); // Type 30
	static void swapButtonDescCb(); // Type 31
	static void swapSceneDescCb(); // Type 32

	// Main Menu
	void loadMainMenu();
	void swapMainMenuDesc();
	void runMainMenu();
	
	static void swapMainMenuDescCb();

	const byte *_mainMenuDesc;
	Scene *_mainMenuScene;
};

} // End of namespace Merlin

} // End of namespace Bolt

#endif // MERLIN_MERLIN_H
