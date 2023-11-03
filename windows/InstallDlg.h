/*
 * Copyright (C) 2006-2011 Crise, crise<at>mail.berlios.de
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

#if !defined(INSTALL_DLG_H)
#define INSTALL_DLG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WinUtil.h"
#include <client/version.h>
#include <atlctrlx.h>

class InstallDlg : public CDialogImpl<InstallDlg>
{
public:
	enum { IDD = IDD_INSTALL };

	InstallDlg(bool b = false) : fromHelp(b) { };
	~InstallDlg() { };

	BEGIN_MSG_MAP(InstallDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDC_LINK, onLink)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		if(fromHelp && MessageBox(CTSTRING(REALLY_RUN), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDNO) {
			EndDialog(IDCANCEL);
			return 0;
		}

		// Translate Dialog
		SetWindowText(Text::toT(APPNAME " " + STRING(INSTALL)).c_str());
		SetDlgItemText(IDC_INTRO, CTSTRING(INSTALL_INTRO));
		SetDlgItemText(IDC_INFO, CTSTRING(INSTALL_INFO));
		SetDlgItemText(IDC_NOTE, CTSTRING(INSTALL_NOTE));
		SetDlgItemText(IDC_START_MENU, CTSTRING(INSTALL_START));
		SetDlgItemText(IDC_DESKTOP, CTSTRING(INSTALL_DESKTOP));
		SetDlgItemText(IDC_QLAUNCH, CTSTRING(INSTALL_QLAUNCH));
		SetDlgItemText(IDC_FIREWALL, CTSTRING(INSTALL_FIREWALL));

		::EnableWindow(GetDlgItem(IDCANCEL), false);
		::EnableWindow(GetDlgItem(IDCLOSE), false);

		OSVERSIONINFOEX ver;
		WinUtil::getVersionInfo(ver);
		if(!((ver.dwMajorVersion >= 5 && ver.dwMinorVersion >= 1 && ver.wServicePackMajor >= 2) || (ver.dwMajorVersion >= 6)) || !WinUtil::isUserAdmin()) {
			::EnableWindow(GetDlgItem(IDC_FIREWALL), false);
		}

		url.SubclassWindow(GetDlgItem(IDC_LINK));
		url.SetHyperLinkExtendedStyle(HLINK_UNDERLINEHOVER);

		url.SetHyperLink(_T("http://www.gnu.org/copyleft/gpl.html#SEC1"));
		url.SetLabel(_T("GNU General Public License"));
		

		CenterWindow(GetParent());
		return TRUE;
	}

	LRESULT onLink (WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		url.Navigate();
		return 0;
	}

	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		if(IsDlgButtonChecked(IDC_START_MENU) == 1) {
			WinUtil::createShortcut(WinUtil::getAppName(), "ApexDC++", Util::getFilePath(WinUtil::getAppName()), "ApexDC++ - Pinnacle of File Sharing", CSIDL_STARTMENU);
		} if(IsDlgButtonChecked(IDC_DESKTOP) == 1) {
			WinUtil::createShortcut(WinUtil::getAppName(), "ApexDC++", Util::getFilePath(WinUtil::getAppName()), "ApexDC++ - Pinnacle of File Sharing", CSIDL_DESKTOP);
		} if(IsDlgButtonChecked(IDC_QLAUNCH) == 1) {
			WinUtil::createShortcut(WinUtil::getAppName(), "Microsoft\\Internet Explorer\\Quick Launch\\ApexDC++", Util::getFilePath(WinUtil::getAppName()), "ApexDC++ - Pinnacle of File Sharing", CSIDL_APPDATA);
		}

		WinUtil::alterWinFirewall(IsDlgButtonChecked(IDC_FIREWALL) == 1);
		WinUtil::createAddRemoveEntry();

		EndDialog(wID);
		return 0;
	}

private:
	CHyperLink url;
	bool fromHelp;

	InstallDlg(const InstallDlg&) { dcassert(0); };
};

#endif // !defined(INSTALL_DLG_H)
