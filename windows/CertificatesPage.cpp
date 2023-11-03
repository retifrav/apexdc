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

#include "stdafx.h"
#include "Resource.h"

#include <client/SettingsManager.h>
#include <client/FavoriteManager.h>
#include <client/CryptoManager.h>

#include <client/HashCalc.h>
#include <client/version.h>

#include "CertificatesPage.h"
#include "LineDlg.h"
#include "CommandDlg.h"
#include "WinUtil.h"

PropPage::TextItem CertificatesPage::texts[] = {
	{ IDC_STATIC1, ResourceManager::PRIVATE_KEY_FILE },
	{ IDC_STATIC2, ResourceManager::OWN_CERTIFICATE_FILE },
	{ IDC_STATIC3, ResourceManager::TRUSTED_CERTIFICATES_PATH },
	{ IDC_GENERATE_CERTS, ResourceManager::GENERATE_CERTIFICATES },
	{ IDC_USE_ANTIVIR, ResourceManager::USE_ANTIVIR },
	{ IDC_ANTIVIR_BROWSE, ResourceManager::BROWSE },
	{ IDC_HIDE_ANTIVIR, ResourceManager::HIDE_ANTIVIR },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item CertificatesPage::items[] = {
	{ IDC_TLS_CERTIFICATE_FILE, SettingsManager::TLS_CERTIFICATE_FILE, PropPage::T_STR },
	{ IDC_TLS_PRIVATE_KEY_FILE, SettingsManager::TLS_PRIVATE_KEY_FILE, PropPage::T_STR },
	{ IDC_TLS_TRUSTED_CERTIFICATES_PATH, SettingsManager::TLS_TRUSTED_CERTIFICATES_PATH, PropPage::T_STR },
	{ IDC_PROT_START, SettingsManager::PROTECT_START, PropPage::T_BOOL },
	{ IDC_PROT_CLOSE, SettingsManager::PROTECT_CLOSE, PropPage::T_BOOL },
	{ IDC_PROT_TRAY, SettingsManager::PROTECT_TRAY, PropPage::T_BOOL },
	{ IDC_USE_ANTIVIR, SettingsManager::USE_ANTIVIR, PropPage::T_BOOL },
	{ IDC_ANTIVIR_PATH, SettingsManager::ANTIVIR_PATH, PropPage::T_STR },
	{ IDC_ANTIVIR_PARAMS, SettingsManager::ANTIVIR_PARAMS, PropPage::T_STR },
	{ IDC_HIDE_ANTIVIR, SettingsManager::HIDE_ANTIVIR, PropPage::T_BOOL },
	{ 0, 0, PropPage::T_END }
};

PropPage::ListItem CertificatesPage::listItems[] = {
	{ SettingsManager::USE_TLS, ResourceManager::SETTINGS_USE_TLS },
	{ SettingsManager::ALLOW_UNTRUSTED_HUBS, ResourceManager::SETTINGS_ALLOW_UNTRUSTED_HUBS	},
	{ SettingsManager::ALLOW_UNTRUSTED_CLIENTS, ResourceManager::SETTINGS_ALLOW_UNTRUSTED_CLIENTS },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT CertificatesPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items, listItems, GetDlgItem(IDC_TLS_OPTIONS));

	fixControls();

	// Do specialized reading here
	return TRUE;
}

void CertificatesPage::write() {
	PropPage::write((HWND)*this, items, listItems, GetDlgItem(IDC_TLS_OPTIONS));
}

LRESULT CertificatesPage::onBrowsePrivateKey(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring target = Text::toT(SETTING(TLS_PRIVATE_KEY_FILE));
	CEdit edt(GetDlgItem(IDC_TLS_PRIVATE_KEY_FILE));

	if(WinUtil::browseFile(target, m_hWnd, false, target)) {
		edt.SetWindowText(&target[0]);
	}
	return 0;
}

LRESULT CertificatesPage::onBrowseCertificate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring target = Text::toT(SETTING(TLS_CERTIFICATE_FILE));
	CEdit edt(GetDlgItem(IDC_TLS_CERTIFICATE_FILE));

	if(WinUtil::browseFile(target, m_hWnd, false, target)) {
		edt.SetWindowText(&target[0]);
	}
	return 0;
}

LRESULT CertificatesPage::onBrowseTrustedPath(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring target = Text::toT(SETTING(TLS_TRUSTED_CERTIFICATES_PATH));
	CEdit edt(GetDlgItem(IDC_TLS_TRUSTED_CERTIFICATES_PATH));

	if(WinUtil::browseDirectory(target, m_hWnd)) {
		edt.SetWindowText(&target[0]);
	}
	return 0;
}

LRESULT CertificatesPage::onGenerateCerts(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	try {
		CryptoManager::getInstance()->generateCertificate();
	} catch(const CryptoException& e) {
		MessageBox(Text::toT(e.getError()).c_str(), L"Error generating certificate");
	}
	return 0;
}

LRESULT CertificatesPage::onSetPassword(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ChngPassDlg dlg;
	dlg.title = _T("Change password");
	dlg.hideold = (SETTING(PASSWORD).empty() || SETTING(PASSWORD) == "LWPNACQDBZRYXW3VHJVCJ64QBZNGHOHHHZWCLNQ");
	if(dlg.DoModal(m_hWnd) == IDOK){
		string tmp = TTH(Text::fromT(dlg.Oldline));
		if(dlg.hideold || tmp == SETTING(PASSWORD)) {
			if(dlg.Newline == dlg.Confirmline) {
				tmp = TTH(Text::fromT(dlg.Newline));
				settings->set(SettingsManager::PASSWORD, tmp);
			} else {
				::MessageBox(m_hWnd, _T("The passwords you provided did not match!"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
			}
		} else {
			::MessageBox(m_hWnd, _T("Old password was invalid!"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_OK | MB_ICONEXCLAMATION | MB_TOPMOST);
		}
	}
	return 0;
}

LRESULT CertificatesPage::onAntiVirBrowse(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	TCHAR buf[MAX_PATH];
	static const TCHAR types[] = _T("Executable Files\0*.exe\0All Files\0*.*\0");

	GetDlgItemText(IDC_ANTIVIR_PATH, buf, MAX_PATH);
	tstring x = buf;

	if(WinUtil::browseFile(x, m_hWnd, false, Text::toT(Util::getPath(Util::PATH_GLOBAL_CONFIG)), types) == IDOK) {
		SetDlgItemText(IDC_ANTIVIR_PATH, x.c_str());
	}
	return 0;
}

void CertificatesPage::fixControls() {
	bool state = (IsDlgButtonChecked(IDC_USE_ANTIVIR) != 0);
	::EnableWindow(GetDlgItem(IDC_ANTIVIR_PATH), state);
	::EnableWindow(GetDlgItem(IDC_ANTIVIR_PARAMS), state);
	::EnableWindow(GetDlgItem(IDC_ANTIVIR_BROWSE), state);
	::EnableWindow(GetDlgItem(IDC_HIDE_ANTIVIR), state);
}

/**
 * @file
 * $Id$
 */
