#if !defined(FAV_DIR_DLG_H)
#define FAV_DIR_DLG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <wtl/atlcrack.h>
#include <client/Util.h>

class FavDirDlg : public CDialogImpl<FavDirDlg>
{
public:
	FavDirDlg() { };
	~FavDirDlg() { };

	tstring name;
	tstring dir;
	tstring extensions;

	enum { IDD = IDD_FAVORITEDIR };

	BEGIN_MSG_MAP_EX(FavDirDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SETFOCUS, OnFocus)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		COMMAND_ID_HANDLER(IDC_FAVDIR_BROWSE, OnBrowse);
	END_MSG_MAP()

	LRESULT OnFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlName.SetFocus();
		return FALSE;
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		#define ATTACH(id, var) var.Attach(GetDlgItem(id))
		ATTACH(IDC_FAVDIR_NAME, ctrlName);
		ATTACH(IDC_FAVDIR, ctrlDirectory);
		ATTACH(IDC_FAVDIR_EXTENSION, ctrlExtensions);

		ctrlName.SetWindowText(name.c_str());
		ctrlDirectory.SetWindowText(dir.c_str());
		ctrlExtensions.SetWindowText(extensions.c_str());

		return 0;
	}

	LRESULT OnBrowse(UINT /*uMsg*/, WPARAM /*wParam*/, HWND /*lParam*/, BOOL& /*bHandled*/) {
		tstring target;
		if(WinUtil::browseDirectory(target, m_hWnd)) {
			ctrlDirectory.SetWindowText(target.c_str());
			if(ctrlName.GetWindowTextLength() == 0)
				ctrlName.SetWindowText(Util::getLastDir(target).c_str());
		}
		return 0;
	}

	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		if(wID == IDOK) {
			TCHAR buf[256];

			if((ctrlName.GetWindowTextLength() == 0) || (ctrlDirectory.GetWindowTextLength()== 0)){
				MessageBox(_T("Name nor directory must not be empty"));
				return 0;
			}

			#define GET_TEXT(id, var) \
			GetDlgItemText(id, buf, 256); \
			var = buf;

			GET_TEXT(IDC_FAVDIR_NAME, name);
			GET_TEXT(IDC_FAVDIR, dir);
			GET_TEXT(IDC_FAVDIR_EXTENSION, extensions);

		}

		ctrlName.Detach();
		ctrlDirectory.Detach();
		ctrlExtensions.Detach();

		EndDialog(wID);
		return 0;
	}

private:
	FavDirDlg(const FavDirDlg&) { dcassert(0); };
	CEdit ctrlName;
	CEdit ctrlDirectory;
	CEdit ctrlExtensions;

};

#endif // !defined(FAV_DIR_DLG_H)