#if !defined(EXCEPTION_DLG_H)
#define EXCEPTION_DLG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WinUtil.h"

#include "../client/ResourceManager.h"

class CExceptionDlg : public CDialogImpl<CExceptionDlg>
{
public:
	enum { IDD = IDD_EXCEPTION };
	CExceptionDlg() : m_hIcon(NULL) { };

	~CExceptionDlg() {
		if (m_hIcon)
			DeleteObject((HGDIOBJ)m_hIcon);
	};

	BEGIN_MSG_MAP(CExceptionDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_HANDLER(IDC_COPY_EXCEPTION, BN_CLICKED, OnCopyException)
		COMMAND_HANDLER(IDOK, BN_CLICKED, OnContinue)
		COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnTerminate)
	END_MSG_MAP()

	void CopyEditToClipboard(HWND hWnd)
	{
		SendMessage(hWnd, EM_SETSEL, 0, 65535L);
		SendMessage(hWnd, WM_COPY, 0 , 0);
		SendMessage(hWnd, EM_SETSEL, 0, 0);
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		// Translate Dialog
		SetWindowText(CTSTRING(EXCEPTION_DIALOG));
		SetDlgItemText(IDC_EXCEPTION_HEADER, CTSTRING(EXCEPTION_HEADER));
		SetDlgItemText(IDC_EXCEPTION_FOOTER, CTSTRING(EXCEPTION_FOOTER));
		SetDlgItemText(IDC_EXCEPTION_RESTART, CTSTRING(EXCEPTION_RESTART));
		SetDlgItemText(IDC_COPY_EXCEPTION, CTSTRING(EXCEPTION_COPY));
		SetDlgItemText(IDOK, CTSTRING(EXCEPTION_CONTINUE));
		SetDlgItemText(IDCANCEL, CTSTRING(EXCEPTION_TERMINATE));

		m_hIcon = ::LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDR_STOP));
		SetIcon(m_hIcon, FALSE);
		SetIcon(m_hIcon, TRUE);

		CenterWindow();
		CEdit ctrlEI(GetDlgItem(IDC_EXCEPTION_DETAILS));
		ctrlEI.FmtLines(TRUE);
		ctrlEI.AppendText(WinUtil::exceptioninfo.c_str(), FALSE);
		ctrlEI.Detach();

		return TRUE;
	}

	LRESULT OnCopyException(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		CopyEditToClipboard(GetDlgItem(IDC_EXCEPTION_DETAILS));
		return 0;
	}

	LRESULT OnContinue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		EndDialog(IDOK);
		return 0;
	}

	LRESULT OnTerminate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		if(IsDlgButtonChecked(IDC_EXCEPTION_RESTART) == BST_CHECKED) {
			ShellExecute(NULL, NULL, Text::toT(WinUtil::getAppName()).c_str(), _T("/rebuild /crash"), NULL, SW_SHOWNORMAL);
		}
		EndDialog(IDCANCEL);
		return 0;
	}


private:
	HICON m_hIcon;
	CExceptionDlg(const CExceptionDlg&) { dcassert(0); };

};

#endif // !defined(EXCEPTION_DLG_H)
