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

#if !defined(LINE_DLG_H)
#define LINE_DLG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class LineDlg : public CDialogImpl<LineDlg>
{
	CEdit ctrlLine;
	CStatic ctrlDescription;
public:
	tstring line;
	tstring description;
	tstring title;
	bool password;
	bool disable;

	enum { IDD = IDD_LINE };
	
	BEGIN_MSG_MAP(LineDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()
	
	LineDlg() : password(false), disable(false) { }
	
	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlLine.SetFocus();
		return FALSE;
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		ctrlLine.Attach(GetDlgItem(IDC_LINE));
		ctrlLine.SetFocus();
		ctrlLine.SetWindowText(line.c_str());
		ctrlLine.SetSelAll(TRUE);
		if(password) {
			ctrlLine.SetWindowLongPtr(GWL_STYLE, ctrlLine.GetWindowLongPtr(GWL_STYLE) | ES_PASSWORD);
			ctrlLine.SetPasswordChar('*');
		}

		ctrlDescription.Attach(GetDlgItem(IDC_DESCRIPTION));
		ctrlDescription.SetWindowText(description.c_str());
		
		SetWindowText(title.c_str());
		
		if(disable) {
			::EnableWindow(GetDlgItem(IDCANCEL), false);
			::ShowWindow(GetDlgItem(IDCANCEL), false);
			::MoveWindow(GetDlgItem(IDOK), 275, 29, 75, 24, false);
			::SetForegroundWindow((HWND)*this);
		}

		CenterWindow(GetParent());
		return FALSE;
	}
	
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if(wID == IDOK) {
			line.resize(ctrlLine.GetWindowTextLength() + 1);
			line.resize(GetDlgItemText(IDC_LINE, &line[0], line.size()));
		}
		EndDialog(wID);
		return 0;
	}
	
};

class ChngPassDlg : public CDialogImpl<ChngPassDlg>
{
	CEdit ctrlOldLine;
	CEdit ctrlNewLine;
	CEdit ctrlConfirmLine;

public:
	tstring Oldline;
	tstring Newline;
	tstring Confirmline;
	tstring title;
	bool hideold;

	enum { IDD = IDD_CHANGE_PASS };
	
	BEGIN_MSG_MAP(LineDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()
	
	ChngPassDlg() : hideold(false) { };
	
	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		if(hideold) {
			ctrlNewLine.SetFocus();
		} else {
			ctrlOldLine.SetFocus();
		}
		return FALSE;
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		// Translate static
		SetDlgItemText(IDC_PSWD_CHNG_OLD, CTSTRING(OLD_PW));
		SetDlgItemText(IDC_PSWD_CHNG_NEW, CTSTRING(NEW_PW));
		SetDlgItemText(IDC_PSWD_CHNG_CONFIRM_NEW, CTSTRING(CONFIRM_NEW_PW));
		SetDlgItemText(IDC_PSWD_CHNG_STATIC, title.c_str());

		ctrlOldLine.Attach(GetDlgItem(IDC_LINE));
		if(hideold) {
			ctrlOldLine.EnableWindow(FALSE);
		} else {
			ctrlOldLine.EnableWindow(TRUE);
			ctrlOldLine.SetWindowLongPtr(GWL_STYLE, ctrlOldLine.GetWindowLongPtr(GWL_STYLE) | ES_PASSWORD);
			ctrlOldLine.SetPasswordChar('*');
			ctrlOldLine.SetFocus();
			ctrlOldLine.SetSelAll(TRUE);            
		}

		ctrlNewLine.Attach(GetDlgItem(IDC_LINE2));
		ctrlNewLine.SetWindowLongPtr(GWL_STYLE, ctrlNewLine.GetWindowLongPtr(GWL_STYLE) | ES_PASSWORD);
		ctrlNewLine.SetPasswordChar('*');
		if(hideold) ctrlNewLine.SetFocus();

		ctrlConfirmLine.Attach(GetDlgItem(IDC_LINE3));
		ctrlConfirmLine.SetWindowLongPtr(GWL_STYLE, ctrlConfirmLine.GetWindowLongPtr(GWL_STYLE) | ES_PASSWORD);
		ctrlConfirmLine.SetPasswordChar('*');

		SetWindowText(title.c_str());

		CenterWindow(GetParent());
		return FALSE;
	}
	
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if(wID == IDOK) {
			Oldline.resize(ctrlOldLine.GetWindowTextLength() + 1);
			Oldline.resize(GetDlgItemText(IDC_LINE, &Oldline[0], Oldline.size()));

			Newline.resize(ctrlNewLine.GetWindowTextLength() + 1);
			Newline.resize(GetDlgItemText(IDC_LINE2, &Newline[0], Newline.size()));

			Confirmline.resize(ctrlConfirmLine.GetWindowTextLength() + 1);
			Confirmline.resize(GetDlgItemText(IDC_LINE3, &Confirmline[0], Confirmline.size()));
		}
		EndDialog(wID);
		return 0;
	}
	
};

class KickDlg : public CDialogImpl<KickDlg> {
	CComboBox ctrlLine;
	CStatic ctrlDescription;
public:
	tstring line;
	static tstring m_sLastMsg;
	tstring description;
	tstring title;

	enum { IDD = IDD_KICK };
	
	BEGIN_MSG_MAP(KickDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()
	
	KickDlg() {};
	
	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlLine.SetFocus();
		return FALSE;
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

private:
	tstring Msgs[20];
};

#endif // !defined(LINE_DLG_H)

/**
 * @file
 * $Id$
 */
