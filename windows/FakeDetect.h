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

#ifndef FakeDetect_H
#define FakeDetect_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <wtl/atlcrack.h>
#include "PropPage.h"
#include "ExListViewCtrl.h"

#include "../client/RawManager.h"

class FakeDetect : public CPropertyPage<IDD_FAKEDETECT>, public PropPage, protected RawSelector
{
public:
	FakeDetect(SettingsManager *s) : PropPage(s) {
		title = _tcsdup((TSTRING(SETTINGS_ADVANCED) + _T('\\') + TSTRING(SETTINGS_FAKEDETECT)).c_str());
		SetTitle(title);
		m_psp.dwFlags |= PSP_RTLREADING;
	};

	~FakeDetect() {
		ctrlList.Detach();
		free(title);
	};

	BEGIN_MSG_MAP(FakeDetect)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_ID_HANDLER(IDC_RAW_DETECTOR, onRawChanged)
		COMMAND_ID_HANDLER(IDC_CHECKER_CONF, onCheckerConf)
		NOTIFY_HANDLER(IDC_DETECTOR_ITEMS, LVN_ITEMCHANGED, onItemChanged)
		NOTIFY_HANDLER(IDC_DETECTOR_ITEMS, NM_CUSTOMDRAW, onCustomDraw)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT onRawChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCheckerConf(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	
	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();

protected:
	static Item items[];
	static TextItem texts[];
	TCHAR* title;
	static ListItem listItems[];

	ExListViewCtrl ctrlList;
	CComboBox cRaw;

	void insertAllItem();
	void insertItem(const tstring& a, int b);
	int settingRaw[7];
};

#endif //FakeDetect_H

/**
 * @file
 * $Id$
 */

