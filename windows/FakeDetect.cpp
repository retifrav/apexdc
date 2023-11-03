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

#include "FakeDetect.h"
#include "WinUtil.h"
#include "CheckerPrefDlg.h"
#include "BarShader.h"

PropPage::TextItem FakeDetect::texts[] = {
	{ DAA, ResourceManager::TEXT_FAKEPERCENT },
	{ IDC_TIMEOUTS, ResourceManager::ACCEPTED_TIMEOUTS },
	{ IDC_DISCONNECTS, ResourceManager::ACCEPTED_DISCONNECTS },
	{ IDC_PROT_USERS, ResourceManager::PROT_USERS },
	{ IDC_PROT_DESC, ResourceManager::PROT_DESC },
	{ IDC_PROT_FAVS, ResourceManager::PROT_FAVS },
	{ IDC_ACTION_STR, ResourceManager::SET_ACTION },
	{ IDC_FAKE_SET, ResourceManager::FAKE_SET },
	{ IDC_RAW_SET, ResourceManager::RAW_SET },
	{ IDC_CHECKER_CONF, ResourceManager::SETTINGS_ADVANCED },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
}; 

PropPage::Item FakeDetect::items[] = {
	{ IDC_PERCENT_FAKE_SHARE_TOLERATED, SettingsManager::PERCENT_FAKE_SHARE_TOLERATED, PropPage::T_INT }, 
	{ IDC_TIMEOUTS_NO, SettingsManager::ACCEPTED_TIMEOUTS, PropPage::T_INT }, 
	{ IDC_DISCONNECTS_NO, SettingsManager::ACCEPTED_DISCONNECTS, PropPage::T_INT },
	{ IDC_PROT_PATTERNS, SettingsManager::PROT_USERS, PropPage::T_STR }, 
	{ IDC_PROT_FAVS, SettingsManager::PROT_FAVS, PropPage::T_BOOL }, 
	{ 0, 0, PropPage::T_END }
};

FakeDetect::ListItem FakeDetect::listItems[] = {
	{ SettingsManager::CHECK_NEW_USERS, ResourceManager::CHECK_ON_CONNECT },
	{ SettingsManager::DISPLAY_CHEATS_IN_MAIN_CHAT, ResourceManager::SETTINGS_DISPLAY_CHEATS_IN_MAIN_CHAT },
	{ SettingsManager::SHOW_SHARE_CHECKED_USERS, ResourceManager::SETTINGS_ADVANCED_SHOW_SHARE_CHECKED_USERS },
	{ SettingsManager::DELETE_CHECKED, ResourceManager::DELETE_CHECKED },
	{ SettingsManager::IP_IN_CHAT, ResourceManager::IP_IN_CHAT },
	{ SettingsManager::COUNTRY_IN_CHAT, ResourceManager::COUNTRY_IN_CHAT },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT FakeDetect::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items, listItems, GetDlgItem(IDC_FAKE_BOOLEANS));

	int n = 0;
	settingRaw[n++] = SETTING(DISCONNECT_RAW);
	settingRaw[n++] = SETTING(TIMEOUT_RAW);
	settingRaw[n++] = SETTING(FAKESHARE_RAW);
	settingRaw[n++] = SETTING(LISTLEN_MISMATCH);
	settingRaw[n++] = SETTING(FILELIST_TOO_SMALL);
	settingRaw[n++] = SETTING(FILELIST_UNAVAILABLE);

	CRect rc;
	ctrlList.Attach(GetDlgItem(IDC_DETECTOR_ITEMS));
	ctrlList.GetClientRect(rc);
	ctrlList.InsertColumn(0, CTSTRING(SETTINGS_NAME), LVCFMT_LEFT, (rc.Width() / 2) + 40, 0);
	ctrlList.InsertColumn(1, CTSTRING(ACTION), LVCFMT_LEFT, (rc.Width() / 2) - 57, 1);
	ctrlList.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	insertAllItem();

	cRaw.Attach(GetDlgItem(IDC_RAW_DETECTOR));
	createList();
	for(Iter i = idAction.begin(); i != idAction.end(); ++i) {
		cRaw.AddString(RawManager::getInstance()->getNameActionId(i->second).c_str());
	}
	cRaw.SetCurSel(0);

	// Do specialized reading here
	
	return TRUE;
}

void FakeDetect::write() {
	PropPage::write((HWND)*this, items, listItems, GetDlgItem(IDC_FAKE_BOOLEANS));
	
	// Do specialized writing here
	// settings->set(XX, YY);

	int n = 0;
	SettingsManager::getInstance()->set(SettingsManager::DISCONNECT_RAW, RawManager::getInstance()->getValidAction(settingRaw[n++]));
	SettingsManager::getInstance()->set(SettingsManager::TIMEOUT_RAW, RawManager::getInstance()->getValidAction(settingRaw[n++]));
	SettingsManager::getInstance()->set(SettingsManager::FAKESHARE_RAW, RawManager::getInstance()->getValidAction(settingRaw[n++]));
	SettingsManager::getInstance()->set(SettingsManager::LISTLEN_MISMATCH, RawManager::getInstance()->getValidAction(settingRaw[n++]));
	SettingsManager::getInstance()->set(SettingsManager::FILELIST_TOO_SMALL, RawManager::getInstance()->getValidAction(settingRaw[n++]));
	SettingsManager::getInstance()->set(SettingsManager::FILELIST_UNAVAILABLE, RawManager::getInstance()->getValidAction(settingRaw[n++]));
}

void FakeDetect::insertAllItem() {
	int n = 0;
	insertItem(TSTRING(DISCONNECT_RAW), settingRaw[n++]);
	insertItem(TSTRING(TIMEOUT_RAW), settingRaw[n++]);
	insertItem(TSTRING(FAKESHARE_RAW), settingRaw[n++]);
	insertItem(TSTRING(LISTLEN_MISMATCH), settingRaw[n++]);
	insertItem(TSTRING(FILELIST_TOO_SMALL), settingRaw[n++]);
	insertItem(TSTRING(FILELIST_UNAVAILABLE), settingRaw[n++]);
}

void FakeDetect::insertItem(const tstring& a, int b) {
	TStringList cols;
	cols.push_back(a);
	cols.push_back(RawManager::getInstance()->getNameActionId(RawManager::getInstance()->getValidAction(b)));
	ctrlList.insert(cols);
	cols.clear();
}

LRESULT FakeDetect::onRawChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int sel = cRaw.GetCurSel();
	if(sel >= 0) {
		int item = ctrlList.GetSelectedIndex();
		if(item >= 0) {
			settingRaw[item] = RawManager::getInstance()->getValidAction(getIdAction(sel));
			ctrlList.SetItemText(item, 1, RawManager::getInstance()->getNameActionId(settingRaw[item]).c_str());
		}
	}

	return 0;
}

LRESULT FakeDetect::onCheckerConf(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CheckerPrefDlg dlg;
	dlg.DoModal(m_hWnd);
	return 0;
}

LRESULT FakeDetect::onItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NM_LISTVIEW* lv = (NM_LISTVIEW*) pnmh;
	::EnableWindow(GetDlgItem(IDC_RAW_DETECTOR), (lv->uNewState & LVIS_FOCUSED));

	int sel = ctrlList.GetSelectedIndex();
	if(sel >= 0) {
		cRaw.SetCurSel(getId(settingRaw[sel]));
		cRaw.SetFocus();
	}

	return 0;
}

LRESULT FakeDetect::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)pnmh;

	switch(cd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
		{
			try	{
				if (settingRaw[cd->nmcd.dwItemSpec]) {
					cd->clrText = SETTING(BAD_CLIENT_COLOUR);
				}
				if(BOOLSETTING(USE_CUSTOM_LIST_BACKGROUND) && cd->nmcd.dwItemSpec % 2 != 0) {
					cd->clrTextBk = HLS_TRANSFORM(::GetSysColor(COLOR_WINDOW), -9, 0);
				}
				return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
			} 
			catch(const Exception&)
			{
			}
			catch(...)
			{
			}
		}
		return CDRF_NOTIFYSUBITEMDRAW;

	default:
		return CDRF_DODEFAULT;
	}
}

/**
 * @file
 * $Id$
 */

