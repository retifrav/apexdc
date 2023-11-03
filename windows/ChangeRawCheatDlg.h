#include <client/RawManager.h>

#ifndef CHANGE_RAW_CHEAT_DLG_H
#define CHANGE_RAW_CHEAT_DLG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class ChangeRawCheatDlg : public CDialogImpl<ChangeRawCheatDlg>, protected RawSelector
{
	CEdit ctrlCheat;
	CComboBox ctrlRaw;
public:
	int raw;
	string cheatingDescription;
	string name;

	enum { IDD = IDD_RAW_CHEAT };
	
	BEGIN_MSG_MAP(ChangeRawCheatDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()
	
	ChangeRawCheatDlg() { };
	
	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlCheat.SetFocus();
		return FALSE;
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlCheat.Attach(GetDlgItem(IDC_CHEAT));
		ctrlCheat.SetFocus();
		ctrlCheat.SetWindowText(Text::toT(cheatingDescription).c_str());

		ctrlRaw.Attach(GetDlgItem(IDC_CHANGE_RAW));
		createList();
		for(Iter i = idAction.begin(); i != idAction.end(); ++i) {
			ctrlRaw.AddString(RawManager::getInstance()->getNameActionId(i->second).c_str());
		}
		ctrlRaw.SetCurSel(getId(raw));
		SetWindowText(Text::toT(name).c_str());
		
		CenterWindow(GetParent());
		return FALSE;
	}
	
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		if(wID == IDOK) {
			int len = ctrlCheat.GetWindowTextLength() + 1;
			TCHAR* buf = new TCHAR[len];
			GetDlgItemText(IDC_CHEAT, buf, len);
			cheatingDescription = Text::fromT(buf);
			delete[] buf;
			raw = getIdAction(ctrlRaw.GetCurSel());
		}
		EndDialog(wID);
		return 0;
	}
};

#endif // CHANGE_RAW_CHEAT_DLG_H