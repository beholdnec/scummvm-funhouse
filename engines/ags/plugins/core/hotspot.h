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

#ifndef AGS_PLUGINS_CORE_HOTSPOT_H
#define AGS_PLUGINS_CORE_HOTSPOT_H

#include "ags/plugins/plugin_base.h"

namespace AGS3 {
namespace Plugins {
namespace Core {

class Hotspot : public ScriptContainer {
public:
	void AGS_EngineStartup(IAGSEngine *engine);

	static void GetHotspotAtRoom(ScriptMethodParams &params);
	static void GetHotspotAtScreen(ScriptMethodParams &params);
	static void GetName(ScriptMethodParams &params);
	static void GetProperty(ScriptMethodParams &params);
	static void GetPropertyText(ScriptMethodParams &params);
	static void GetTextProperty(ScriptMethodParams &params);
	static void RunInteraction(ScriptMethodParams &params);
	static void GetEnabled(ScriptMethodParams &params);
	static void SetEnabled(ScriptMethodParams &params);
	static void GetID(ScriptMethodParams &params);
	static void GetName_New(ScriptMethodParams &params);
	static void GetWalkToX(ScriptMethodParams &params);
	static void GetWalkToY(ScriptMethodParams &params);
};

} // namespace Core
} // namespace Plugins
} // namespace AGS3

#endif
