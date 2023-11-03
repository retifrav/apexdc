/* 
 * Copyright (C) 2001-2003 Jacek Sieka, j_s@telia.com
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
#include "Resource.h"

#include "../client/SettingsManager.h"

#include "SDCPage.h"
#include "WinUtil.h"

PropPage::TextItem SDCPage::texts[] = {
	{ IDC_SETTINGS_WRITE_BUFFER, ResourceManager::SETTINGS_WRITE_BUFFER },
	{ IDC_SETTINGS_KB, ResourceManager::KB },
	{ IDC_SETTINGS_SEARCH_HISTORY, ResourceManager::SETTINGS_SEARCH_HISTORY },
	{ IDC_SETSTRONGDC_PM_LINES, ResourceManager::SETTINGS_PM_HISTORY },
	{ IDC_SETTINGS_AUTO_SEARCH_LIMIT, ResourceManager::SETTINGS_AUTO_SEARCH_LIMIT },
	{ IDC_STATIC1, ResourceManager::PORT },
	{ IDC_STATIC2, ResourceManager::USER },
	{ IDC_STATIC3, ResourceManager::PASSWORD },
	{ IDC_SETTINGS_ODC_SHUTDOWNTIMEOUT, ResourceManager::SETTINGS_ODC_SHUTDOWNTIMEOUT },
	{ IDC_MAXCOMPRESS, ResourceManager::SETTINGS_MAX_COMPRESS },
	{ IDC_INTERVAL_TEXT, ResourceManager::MINIMUM_SEARCH_INTERVAL },
	{ IDC_MATCH_QUEUE_TEXT, ResourceManager::SETTINGS_SB_MAX_SOURCES },
	{ IDC_SHUTDOWNACTION, ResourceManager::SHUTDOWN_ACTION },
	{ IDC_SETTINGS_DOWNCONN, ResourceManager::SETTINGS_DOWNCONN },
	{ IDC_SETTINGS_MAX_FILELIST_SIZE, ResourceManager::SETTINGS_MAX_FILELIST_SIZE },
	{ IDC_MAX_RESIZE_LINES_STR, ResourceManager::MAX_RESIZE_LINES },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item SDCPage::items[] = {
	{ IDC_BUFFERSIZE, SettingsManager::BUFFER_SIZE, PropPage::T_INT },
	{ IDC_SOCKET_IN_BUFFER, SettingsManager::SOCKET_IN_BUFFER, PropPage::T_INT },
	{ IDC_SOCKET_OUT_BUFFER, SettingsManager::SOCKET_OUT_BUFFER, PropPage::T_INT },
	{ IDC_PM_LINES, SettingsManager::PM_LAST_LINES_LOG, PropPage::T_INT },
	{ IDC_SEARCH_HISTORY, SettingsManager::SEARCH_HISTORY, PropPage::T_INT },
	{ IDC_EDIT1, SettingsManager::WEBSERVER_PORT, PropPage::T_INT }, 
	{ IDC_EDIT2, SettingsManager::WEBSERVER_USER, PropPage::T_STR }, 
	{ IDC_EDIT3, SettingsManager::WEBSERVER_PASS, PropPage::T_STR }, 
	{ IDC_SHUTDOWNTIMEOUT, SettingsManager::SHUTDOWN_TIMEOUT, PropPage::T_INT },
	{ IDC_MAX_COMPRESSION, SettingsManager::MAX_COMPRESSION, PropPage::T_INT },
	{ IDC_INTERVAL, SettingsManager::MINIMUM_SEARCH_INTERVAL, PropPage::T_INT },
	{ IDC_MATCH, SettingsManager::MAX_AUTO_MATCH_SOURCES, PropPage::T_INT },
	{ IDC_AUTO_SEARCH_LIMIT, SettingsManager::AUTO_SEARCH_LIMIT, PropPage::T_INT },
	{ IDC_DOWNCONN, SettingsManager::DOWNCONN_PER_SEC, PropPage::T_INT },
	{ IDC_MAX_FILELIST_SIZE, SettingsManager::MAX_FILELIST_SIZE, PropPage::T_INT },
	{ IDC_RESIZE_LINES, SettingsManager::MAX_RESIZE_LINES, PropPage::T_INT },
	{ 0, 0, PropPage::T_END }
};

#define setMinMax(x, y, z) \
	updown.Attach(GetDlgItem(x)); \
	updown.SetRange32(y, z); \
	updown.Detach();

LRESULT SDCPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);

	CUpDownCtrl updown;
	setMinMax(IDC_BUFFER_SPIN, 0, 4096);
	setMinMax(IDC_READ_SPIN, 1024, 1024*1024);
	setMinMax(IDC_WRITE_SPIN, 1024, 1024*1024);
	setMinMax(IDC_PM_LINESSPIN, 0, 999);
	setMinMax(IDC_SEARCH_HISTORY_SPIN, 1, 100);
	setMinMax(IDC_SHUTDOWN_SPIN , 1, 3600);
	setMinMax(IDC_MAX_COMP_SPIN, 0, 9);
	setMinMax(IDC_INTERVAL_SPIN, 10, 9999);
	setMinMax(IDC_MATCH_SPIN, 1, 999);
	setMinMax(IDC_AUTO_SEARCH_LIMIT_SPIN, 1, 999);
	setMinMax(IDC_DOWNCONN_SPIN, 0, 100);
	setMinMax(IDC_MAX_FILELIST_SIZE_SPIN, 64, 30000);
	setMinMax(IDC_RESIZE_LINES_SPIN, 1, 10);

	ctrlShutdownAction.Attach(GetDlgItem(IDC_COMBO1));
	ctrlShutdownAction.AddString(CTSTRING(POWER_OFF));
	ctrlShutdownAction.AddString(CTSTRING(LOG_OFF));
	ctrlShutdownAction.AddString(CTSTRING(REBOOT));
	ctrlShutdownAction.AddString(CTSTRING(SUSPEND));
	ctrlShutdownAction.AddString(CTSTRING(HIBERNATE));

	ctrlShutdownAction.SetCurSel(SETTING(SHUTDOWN_ACTION));

	// Do specialized reading here
	return TRUE;
}

void SDCPage::write()
{
	PropPage::write((HWND)*this, items);
	SettingsManager::getInstance()->set(SettingsManager::SHUTDOWN_ACTION, ctrlShutdownAction.GetCurSel());

	if(SETTING(AUTO_SEARCH_LIMIT) < 1)
		settings->set(SettingsManager::AUTO_SEARCH_LIMIT, 1);
	if(SETTING(MAX_FILELIST_SIZE) < 64)
		settings->set(SettingsManager::MAX_FILELIST_SIZE, 64);
}

/**
 * @file
 * $Id$
 */

