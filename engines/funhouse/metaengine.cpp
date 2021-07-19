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

#include "funhouse/bolt.h"
#include "funhouse/merlin/merlin.h"
#include "funhouse/detection_tables.h"

namespace Funhouse {

class FunhouseMetaEngine : public AdvancedMetaEngine {
public:
	const char *getName() const override {
		return "funhouse";
	}

	Common::Error createInstance(OSystem *syst, Engine **engine, const ADGameDescription *desc) const override;
};

Common::Error FunhouseMetaEngine::createInstance(OSystem *syst, Engine **engine, const ADGameDescription *desc) const {
	*engine = new FunhouseEngine(syst, desc);
	return Common::kNoError;
}

} // End of namespace Funhouse

#if PLUGIN_ENABLED_DYNAMIC(FUNHOUSE)
	REGISTER_PLUGIN_DYNAMIC(FUNHOUSE, PLUGIN_TYPE_ENGINE, Funhouse::FunhouseMetaEngine);
#else
	REGISTER_PLUGIN_STATIC(FUNHOUSE, PLUGIN_TYPE_ENGINE, Funhouse::FunhouseMetaEngine);
#endif
