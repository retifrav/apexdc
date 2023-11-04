/*
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

#if !defined(CHECKER_PREF_DLG_H)
#define CHECKER_PREF_DLG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WinUtil.h"
#include <wtl/atlctrlx.h>

class CheckerPrefDlg : public CDialogImpl<CheckerPrefDlg>
{
public:
	enum { IDD = IDD_CHECKER_PREFS };

	CheckerPrefDlg() { };
	~CheckerPrefDlg() { };

	BEGIN_MSG_MAP(CheckerPrefDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		// Translate Dialog
		SetWindowText(CTSTRING(FAKE_DETECT_ADV));
		SetDlgItemText(IDC_CHECKER_PREFS, CTSTRING(CHECKER_PREFS));
		SetDlgItemText(IDC_FLISTS_STR, CTSTRING(MAX_FILELISTS));
		SetDlgItemText(IDC_DELAY_STR, CTSTRING(CHECKER_DELAY));
		SetDlgItemText(IDC_STIME_STR, CTSTRING(CHECKER_SLEEP_TIME));
		SetDlgItemText(IDC_DELAYED_SENDING, CTSTRING(CHECKER_DELAYED_SENDING));

		// Load values
		SetDlgItemText(IDC_FLISTS, Util::toStringT(SETTING(MAX_FILELISTS)).c_str());
		SetDlgItemText(IDC_DELAY, Util::toStringT(SETTING(CHECK_DELAY)).c_str());
		SetDlgItemText(IDC_STIME, Util::toStringT(SETTING(SLEEP_TIME)).c_str());
		CheckDlgButton(IDC_DELAYED_SENDING, SETTING(DELAYED_RAW_SENDING) ? BST_CHECKED : BST_UNCHECKED);

		CenterWindow(GetParent());
		return TRUE;
	}
		
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		if(wID == IDOK) {
			TCHAR buf[128];

			GetDlgItemText(IDC_FLISTS, buf, sizeof(buf));
			SettingsManager::getInstance()->set(SettingsManager::MAX_FILELISTS, Text::fromT(buf));
			GetDlgItemText(IDC_DELAY, buf, sizeof(buf));
			SettingsManager::getInstance()->set(SettingsManager::CHECK_DELAY, Text::fromT(buf));
			GetDlgItemText(IDC_STIME, buf, sizeof(buf));
			SettingsManager::getInstance()->set(SettingsManager::SLEEP_TIME, Text::fromT(buf));

			SettingsManager::getInstance()->set(SettingsManager::DELAYED_RAW_SENDING, IsDlgButtonChecked(IDC_DELAYED_SENDING) == 1);
		}

		EndDialog(wID);
		return 0;
	}

private:
	CheckerPrefDlg(const CheckerPrefDlg&) { dcassert(0); };

};

#endif // !defined(CHECKER_PREF_DLG_H)