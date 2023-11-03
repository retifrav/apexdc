/* 
 * Copyright (C) 2001-2006 Jacek Sieka, arnetheduck on gmail point com
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
#include "../client/StringTokenizer.h"

#include "AppearancePage.h"
#include "WinUtil.h"

PropPage::TextItem AppearancePage::texts[] = {
	{ IDC_SETTINGS_APPEARANCE_OPTIONS, ResourceManager::SETTINGS_OPTIONS },
	{ IDC_SETTINGS_BOLD_CONTENTS, ResourceManager::SETTINGS_BOLD_OPTIONS },
	{ IDC_SETTINGS_TIME_STAMPS_FORMAT, ResourceManager::SETTINGS_TIME_STAMPS_FORMAT },
	{ IDC_SETTINGS_LANGUAGE_FILE, ResourceManager::SETTINGS_LANGUAGE_FILE },
	{ IDC_BROWSE, ResourceManager::BROWSE_ACCEL },
	{ IDC_SETTINGS_REQUIRES_RESTART, ResourceManager::SETTINGS_REQUIRES_RESTART },
	{ IDC_USERLISTDBLCLICKACTION, ResourceManager::USERLISTDBLCLICKACTION },
	{ IDC_TRANSFERLISTDBLCLICKACTION, ResourceManager::TRANSFERLISTDBLCLICKACTION },
	{ IDC_CHATDBLCLICKACTION, ResourceManager::CHATDBLCLICKACTION },
	{ IDC_DBLCLICKS, ResourceManager::DBL_CLICK_ACTIONS },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item AppearancePage::items[] = {
	{ IDC_TIME_STAMPS_FORMAT, SettingsManager::TIME_STAMPS_FORMAT, PropPage::T_STR },
	{ IDC_LANGUAGE, SettingsManager::LANGUAGE_FILE, PropPage::T_STR },
	{ 0, 0, PropPage::T_END }
};

PropPage::ListItem AppearancePage::listItems[] = {
	{ SettingsManager::SHOW_PROGRESS_BARS, ResourceManager::SETTINGS_SHOW_PROGRESS_BARS },
	{ SettingsManager::SHOW_INFOTIPS, ResourceManager::SETTINGS_SHOW_INFO_TIPS },
	{ SettingsManager::FILTER_MESSAGES, ResourceManager::SETTINGS_FILTER_MESSAGES },
	{ SettingsManager::MINIMIZE_TRAY, ResourceManager::SETTINGS_MINIMIZE_TRAY },
	{ SettingsManager::TIME_STAMPS, ResourceManager::SETTINGS_TIME_STAMPS },
	{ SettingsManager::STATUS_IN_CHAT, ResourceManager::SETTINGS_STATUS_IN_CHAT },
	{ SettingsManager::SHOW_JOINS, ResourceManager::SETTINGS_SHOW_JOINS },
	{ SettingsManager::FAV_SHOW_JOINS, ResourceManager::SETTINGS_FAV_SHOW_JOINS },
	{ SettingsManager::EXPAND_QUEUE , ResourceManager::SETTINGS_EXPAND_QUEUE },
	{ SettingsManager::SORT_FAVUSERS_FIRST, ResourceManager::SETTINGS_SORT_FAVUSERS_FIRST },
	{ SettingsManager::USE_SYSTEM_ICONS, ResourceManager::SETTINGS_USE_SYSTEM_ICONS },
	{ SettingsManager::GET_USER_COUNTRY, ResourceManager::SETTINGS_GET_USER_COUNTRY },
	{ SettingsManager::CZCHARS_DISABLE, ResourceManager::SETSTRONGDC_CZCHARS_DISABLE },
	{ SettingsManager::USE_OLD_SHARING_UI, ResourceManager::SETTINGS_USE_OLD_SHARING_UI },
	{ SettingsManager::SUPPRESS_MAIN_CHAT, ResourceManager::SETTINGS_ADVANCED_SUPPRESS_MAIN_CHAT },
	{ SettingsManager::UC_SUBMENU, ResourceManager::UC_SUBMENU },
	{ SettingsManager::USE_EXPLORER_THEME, ResourceManager::USE_EXPLORER_THEME },
	{ SettingsManager::GLOBAL_HUBFRAME_CONF, ResourceManager::GLOBAL_HUBFRAME_CONF },
	{ SettingsManager::OLD_TRAY_BEHAV, ResourceManager::OLD_TRAY_BEHAV },
	{ SettingsManager::ZION_TABS, ResourceManager::ZION_TABS },
	{ SettingsManager::NON_HUBS_FRONT, ResourceManager::NON_HUBS_FRONT },
	{ SettingsManager::GLOBAL_MINI_TABS, ResourceManager::GLOBAL_MINI_TABS },
	{ SettingsManager::BLEND_OFFLINE_SEARCH, ResourceManager::BLEND_OFFLINE_SEARCH },
	{ SettingsManager::STRIP_TOPIC, ResourceManager::SETTINGS_STRIP_TOPIC },
	{ SettingsManager::OLD_NAMING_STYLE, ResourceManager::OLD_NAMING_STYLE },
	{ SettingsManager::NAT_SORT, ResourceManager::NAT_SORT },
	{ SettingsManager::USE_CUSTOM_LIST_BACKGROUND, ResourceManager::USE_CUSTOM_LIST_BACKGROUND },
	{ SettingsManager::USE_FAV_NAMES, ResourceManager::USE_FAV_NAMES },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::ListItem AppearancePage::boldItems[] = {
	{ SettingsManager::BOLD_FINISHED_DOWNLOADS, ResourceManager::FINISHED_DOWNLOADS },
	{ SettingsManager::BOLD_FINISHED_UPLOADS, ResourceManager::FINISHED_UPLOADS },
	{ SettingsManager::BOLD_QUEUE, ResourceManager::DOWNLOAD_QUEUE },
	{ SettingsManager::BOLD_HUB, ResourceManager::HUB },
	{ SettingsManager::BOLD_PM, ResourceManager::PRIVATE_MESSAGE },
	{ SettingsManager::BOLD_SEARCH, ResourceManager::SEARCH },
	{ SettingsManager::BOLD_UPLOAD_QUEUE, ResourceManager::UPLOAD_QUEUE },
	{ SettingsManager::BOLD_SYSTEM_LOG, ResourceManager::SYSTEM_LOG },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

AppearancePage::~AppearancePage() {
	userlistaction.Detach();
	transferlistaction.Detach(); 
	chataction.Detach();
}

void AppearancePage::write()
{
	PropPage::write((HWND)*this, items, listItems, GetDlgItem(IDC_APPEARANCE_BOOLEANS));
	PropPage::write((HWND)*this, items, boldItems, GetDlgItem(IDC_BOLD_BOOLEANS));

	settings->set(SettingsManager::USERLIST_DBLCLICK, userlistaction.GetCurSel());
	settings->set(SettingsManager::TRANSFERLIST_DBLCLICK, transferlistaction.GetCurSel());
	settings->set(SettingsManager::CHAT_DBLCLICK, chataction.GetCurSel());
}

LRESULT AppearancePage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);

	PropPage::read((HWND)*this, items, listItems, GetDlgItem(IDC_APPEARANCE_BOOLEANS));
	PropPage::read((HWND)*this, items, boldItems, GetDlgItem(IDC_BOLD_BOOLEANS));

	// Do specialized reading here
	userlistaction.Attach(GetDlgItem(IDC_USERLIST_DBLCLICK));
	transferlistaction.Attach(GetDlgItem(IDC_TRANSFERLIST_DBLCLICK));
	chataction.Attach(GetDlgItem(IDC_CHAT_DBLCLICK));

    userlistaction.AddString(CTSTRING(GET_FILE_LIST));
    userlistaction.AddString(CTSTRING(SEND_PUBLIC_MESSAGE));
    userlistaction.AddString(CTSTRING(SEND_PRIVATE_MESSAGE));
    userlistaction.AddString(CTSTRING(MATCH_QUEUE));
    userlistaction.AddString(CTSTRING(GRANT_EXTRA_SLOT));
	userlistaction.AddString(CTSTRING(ADD_TO_FAVORITES));
	userlistaction.AddString(CTSTRING(BROWSE_FILE_LIST));

	transferlistaction.AddString(CTSTRING(SEND_PRIVATE_MESSAGE));
	transferlistaction.AddString(CTSTRING(GET_FILE_LIST));
	transferlistaction.AddString(CTSTRING(MATCH_QUEUE));
	transferlistaction.AddString(CTSTRING(GRANT_EXTRA_SLOT));
	transferlistaction.AddString(CTSTRING(ADD_TO_FAVORITES));
	transferlistaction.AddString(CTSTRING(BROWSE_FILE_LIST));

	chataction.AddString(CTSTRING(SELECT_USER_LIST));
    chataction.AddString(CTSTRING(SEND_PUBLIC_MESSAGE));
    chataction.AddString(CTSTRING(SEND_PRIVATE_MESSAGE));
    chataction.AddString(CTSTRING(GET_FILE_LIST));
    chataction.AddString(CTSTRING(MATCH_QUEUE));
    chataction.AddString(CTSTRING(GRANT_EXTRA_SLOT));
	chataction.AddString(CTSTRING(ADD_TO_FAVORITES));

	userlistaction.SetCurSel(SETTING(USERLIST_DBLCLICK));
	transferlistaction.SetCurSel(SETTING(TRANSFERLIST_DBLCLICK));
	chataction.SetCurSel(SETTING(CHAT_DBLCLICK));

	return TRUE;
}

LRESULT AppearancePage::onBrowse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	TCHAR buf[MAX_PATH];
	static const TCHAR types[] = _T("Language Files\0*.xml\0All Files\0*.*\0");

	GetDlgItemText(IDC_LANGUAGE, buf, MAX_PATH);
	tstring x = buf;

	if(WinUtil::browseFile(x, m_hWnd, false, Text::toT(Util::getPath(Util::PATH_RESOURCES)), types) == IDOK) {
		SetDlgItemText(IDC_LANGUAGE, x.c_str());
	}
	return 0;
}

LRESULT AppearancePage::onClickedHelp(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	MessageBox(CTSTRING(TIMESTAMP_HELP), CTSTRING(TIMESTAMP_HELP_DESC), MB_OK | MB_ICONINFORMATION);
	return S_OK;
}

/**
 * @file
 * $Id$
 */
