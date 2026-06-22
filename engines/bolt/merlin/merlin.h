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

struct BltPaletteMod;
struct BltButtonGfx;
struct BltButton;
struct BltScene;
struct Scene;

#include "common/pack-start.h"	// START STRUCT PACKING

// A rectangle structure that differs from Common::Rect in the following ways:
// - Data is stored as L, R, T, B
// - All edges are inclusive when testing if a point is contained in the rectangle
struct BltRect {
	int16 left;
	int16 right;
	int16 top;
	int16 bottom;

	void onLoad() {
		WRITE_UINT16(&left, READ_BE_UINT16(&left));
		WRITE_UINT16(&right, READ_BE_UINT16(&right));
		WRITE_UINT16(&top, READ_BE_UINT16(&top));
		WRITE_UINT16(&bottom, READ_BE_UINT16(&bottom));
	}

	bool contains(int x, int y) const {
		return x >= left && x <= right && y >= top && y <= bottom;
	}
} PACKED_STRUCT;

#include "common/pack-end.h"	// END STRUCT PACKING

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
	Scene *loadScene(BltScene* bltScene);
	void drawScene(const Scene *scene, byte flags);
	void drawSceneBackground(const BltScene *bltScene, byte plane);
	void updateSceneButtons(Scene* scene, int x, int y, int8* currButton);
	void drawSceneButton(const BltButtonGfx *buttonGfx, bool idle, uint16 plane);
	void applyPaletteMod(const BltPaletteMod *bltPaletteMod, byte startBase);
	void resetButtonPlanes();
	bool isPointInButton(const BltButton *bltButton, int x, int y);
	void setButtonGfx(Scene *scene, byte button, byte gfx);
	void swapPlaneDesc(); // Type 26
	void swapSpriteDesc(); // Type 27
	void swapPaletteModDesc(); // Type 29
	void swapButtonGfxDesc(); // Type 30
	void swapButtonDesc(); // Type 31
	void swapSceneDesc(); // Type 32
	
	static void swapPlaneDescCb(); // Type 26
	static void swapSpriteDescCb(); // Type 27
	static void swapPaletteModDescCb(); // Type 29
	static void swapButtonGfxDescCb(); // Type 30
	static void swapButtonDescCb(); // Type 31
	static void swapSceneDescCb(); // Type 32

	int16 _sceneOriginX;
	int16 _sceneOriginY;
	uint8 _buttonPlane0;
	uint8 _buttonPlane1;

	// Main Menu
	void loadMainMenu();
	void swapMainMenuDesc();
	void runMainMenu();
	
	static void swapMainMenuDescCb();

	const byte *_mainMenuDesc;
	Scene *_mainMenuScene;

	// Difficulty Menu
	void loadDifficultyMenu();
	void swapDifficultyMenuDesc();
	void runDifficultyMenu();
	void selectDifficulty(uint8 category, uint8 level);

	static void swapDifficultyMenuDescCb();

	Scene *_difficultyMenuScene;
};

} // End of namespace Merlin

} // End of namespace Bolt

#endif // MERLIN_MERLIN_H
