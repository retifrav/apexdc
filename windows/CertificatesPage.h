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

#if !defined(CERTIFICATES_PAGE_H)
#define CERTIFICATES_PAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <wtl/atlcrack.h>
#include "PropPage.h"
#include "ExListViewCtrl.h"

class CertificatesPage : public CPropertyPage<IDD_CERTIFICATESPAGE>, public PropPage
{
public:
	CertificatesPage(SettingsManager *s) : PropPage(s) {
		SetTitle(CTSTRING(SETTINGS_CERTIFICATES));
		m_psp.dwFlags |= PSP_RTLREADING;
	}

	~CertificatesPage() { }

	BEGIN_MSG_MAP(CertificatesPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_ID_HANDLER(IDC_BROWSE_PRIVATE_KEY, onBrowsePrivateKey)
		COMMAND_ID_HANDLER(IDC_BROWSE_CERTIFICATE, onBrowseCertificate)
		COMMAND_ID_HANDLER(IDC_BROWSE_TRUSTED_PATH, onBrowseTrustedPath)
		COMMAND_ID_HANDLER(IDC_GENERATE_CERTS, onGenerateCerts)
		COMMAND_ID_HANDLER(IDC_SET_PW, onSetPassword)
		COMMAND_ID_HANDLER(IDC_ANTIVIR_BROWSE, onAntiVirBrowse)
		COMMAND_ID_HANDLER(IDC_USE_ANTIVIR, onFixControls)
	END_MSG_MAP()

	LRESULT onBrowsePrivateKey(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onBrowseCertificate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onBrowseTrustedPath(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGenerateCerts(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSetPassword(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onAntiVirBrowse(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */);
	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	LRESULT onFixControls(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		fixControls();
		return 0;
	}

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();
	
private:
	static Item items[];
	static TextItem texts[];
	static ListItem listItems[];

	void fixControls();
};

#endif // !defined(CERTIFICATES_PAGE_H)

/**
 * @file
 * $Id$
 */
