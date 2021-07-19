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

#include "funhouse/detection_tables.h"

class FunhouseMetaEngineDetection : public AdvancedMetaEngineDetection {
public:
	FunhouseMetaEngineDetection() : AdvancedMetaEngineDetection(Funhouse::gameDescriptions, sizeof(ADGameDescription), Funhouse::funhouseGames) {
	}

	const char* getEngineId() const override {
		return "funhouse";
	}

	const char* getName() const override {
		return "Funhouse";
	}

	const char *getOriginalCopyright() const override {
		return "(C) 1994 Philips Interactive Media";
	}
};

REGISTER_PLUGIN_STATIC(FUNHOUSE_DETECTION, PLUGIN_TYPE_ENGINE_DETECTION, FunhouseMetaEngineDetection);
