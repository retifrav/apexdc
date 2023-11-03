/* 
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "stdafx.h"
#include "PluginApiWin.h"

#include "../client/PluginManager.h"
#include "../client/Text.h"

#include "MainFrm.h"

DCUI PluginApiWin::dcUI = {
	DCINTF_DCPP_UI_VER,

	&PluginApiWin::addCommand,
	&PluginApiWin::removeCommand,

	&PluginApiWin::playSound,
	&PluginApiWin::notify
};

void PluginApiWin::init() {
	PluginManager::getInstance()->registerInterface(DCINTF_DCPP_UI, &dcUI);
}

// Functions for DCUI
void PluginApiWin::addCommand(const char* /*guid*/, const char* name, DCCommandFunc command, const char* /*icon*/) {
	MainFrame::addPluginCommand(Text::toT(name), [=] { command(name); });
}

void PluginApiWin::removeCommand(const char* /*guid*/, const char* name) {
	MainFrame::removePluginCommand(Text::toT(name));
}

void PluginApiWin::playSound(const char* path) {
	::PlaySound(Text::toT(path).c_str(), NULL, SND_FILENAME | SND_ASYNC);
}

void PluginApiWin::notify(const char* title, const char* message) {
	WinUtil::showPopup(Text::toT(message), Text::toT(title), NIIF_INFO, true);
}
