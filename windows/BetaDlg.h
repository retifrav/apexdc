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

#if !defined(BETA_DLG_H)
#define BETA_DLG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../client/UpdateManager.h"

#include "WinUtil.h"
#include <atlctrlx.h>

class BetaDlg : public CDialogImpl<BetaDlg>
{
public:
	enum { IDD = IDD_BETA };

	BetaDlg() { };
	~BetaDlg() { };

	BEGIN_MSG_MAP(BetaDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		::EnableWindow(GetDlgItem(IDCANCEL), false);
		::EnableWindow(GetDlgItem(IDCLOSE), false);
		CenterWindow(GetParent());
		return TRUE;
	}
		
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		string user = getText(IDC_USER);
		string pass = getText(IDC_PW);

		if(user.empty() || pass.empty()) {
			::MessageBox(NULL, _T("Input is invalid, please try again."), _T("Invalid Input"), MB_OK | MB_ICONEXCLAMATION | MB_TOPMOST);
			return 0;
		}

		UpdateManager::getInstance()->setLogin(user, pass);

		EndDialog(wID);
		return 0;
	}

private:
	BetaDlg(const BetaDlg&) { dcassert(0); };

	string getText(int id) {
		TCHAR buf[512];
		GetDlgItemText(id, buf, 512);
		string text = Text::fromT(buf);
		return text;
	}

};

#endif // !defined(BETA_DLG_H)

/**
 * @file
 * $Id$
 */
