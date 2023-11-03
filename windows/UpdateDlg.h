/* 
 * Copyright (C) 2002-2004 "Opera", <opera at home dot se>
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

#ifndef __UPDATE_DLG
#define __UPDATE_DLG

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <client/UpdateManager.h>

class UpdateDlg : public CDialogImpl<UpdateDlg> 
{
public:
	enum { IDD = IDD_UPDATE };

	BEGIN_MSG_MAP(UpdateDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		COMMAND_ID_HANDLER(IDC_UPDATE_DOWNLOAD, OnDownload)
		COMMAND_ID_HANDLER(IDCLOSE, OnCloseCmd)
	END_MSG_MAP()

	UpdateDlg(const UpdateCheckData& updateInfo) : ui(updateInfo), m_hIcon(NULL) { };

	~UpdateDlg();

	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		::SetFocus(GetDlgItem(ui.isForced ? IDC_UPDATE_DOWNLOAD : IDCLOSE));
		return FALSE;
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDownload(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		if(!ui.isForced)
			EndDialog(IDCLOSE);
		return 0;
	}

	static bool canAutoUpdate(const string& url);
private:
	UpdateCheckData ui;

	HICON m_hIcon;
};

#endif // __UPDATE_DLG
