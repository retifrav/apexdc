/*
 * Copyright (C) 2011 Crise, crise<at>mail.berlios.de
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

#ifndef HASH_TOOL_DLG_H
#define HASH_TOOL_DLG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <wtl/atlctrlx.h>
#include <client/typedefs.h>

class HashToolDlg : public CDialogImpl<HashToolDlg>
{
public:
	enum { IDD = IDD_HASHTOOL };

	HashToolDlg(HWND hWndParent = ::GetActiveWindow()) : parentWnd(hWndParent) { }
	~HashToolDlg() { }

	BEGIN_MSG_MAP(TTHToolDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCLOSE, OnCloseCmd)
	END_MSG_MAP()

	INT_PTR DoModal(const tstring& aFile, const tstring& aTTH, const tstring& aMagnet);

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		EndDialog(wID);
		return 0;
	}

private:
	HashToolDlg(const HashToolDlg&) { dcassert(0); }

	INT_PTR DoModal(HWND hWndParent = ::GetActiveWindow(), LPARAM dwInitParam = NULL) {
		return CDialogImpl<HashToolDlg>::DoModal(hWndParent, dwInitParam);
	}

	tstring file, hashes, magnet;
	HWND parentWnd;
};

#endif // HASH_TOOL_DLG_H
