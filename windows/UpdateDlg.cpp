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

#include "stdafx.h"
#include "Resource.h"

#include "../client/Util.h"
#include "../client/SimpleXML.h"
#include "../client/version.h"

#include "UpdateDlg.h"
#include "WinUtil.h"

#ifdef _WIN64
# define UPGRADE_TAG "UpdateURLx64"
#else
# define UPGRADE_TAG "UpdateURL"
#endif

UpdateDlg::~UpdateDlg() {
	if (m_hIcon)
		DeleteObject((HGDIOBJ)m_hIcon);
};

LRESULT UpdateDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	SetWindowText(Text::toT(STRING(UPDATE_CHECK) + " - " + (ui.isForced ? STRING(MANDATORY_UPDATE) : ui.title)).c_str());

	::SetWindowText(GetDlgItem(IDC_UPDATE_VERSION_CURRENT_LBL), (TSTRING(CURRENT_VERSION) + _T(":")).c_str());
	::SetWindowText(GetDlgItem(IDC_UPDATE_VERSION_LATEST_LBL), (TSTRING(LATEST_VERSION) + _T(":")).c_str());
	::SetWindowText(GetDlgItem(IDC_UPDATE_VERSION), CTSTRING(VERSION));
	::SetWindowText(GetDlgItem(IDC_UPDATE_HISTORY), CTSTRING(HISTORY));
	::SetWindowText(GetDlgItem(IDC_UPDATE_DOWNLOAD), CTSTRING(DOWNLOAD));
	::SetWindowText(GetDlgItem(IDCLOSE), CTSTRING(CLOSE));

	::SetWindowText(GetDlgItem(IDC_UPDATE_VERSION_CURRENT), _T(VERSIONSTRING));
	::SetWindowText(GetDlgItem(IDC_UPDATE_VERSION_LATEST), Text::toT(ui.version).c_str());
	::SetWindowText(GetDlgItem(IDC_UPDATE_HISTORY_TEXT), Text::toT(Text::toDOS(ui.message)).c_str());

	::EnableWindow(GetDlgItem(IDC_UPDATE_DOWNLOAD), UpdateManager::compareVersion(ui.version) < 0 ? TRUE : FALSE);

	m_hIcon = ::LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDR_UPDATE));
	SetIcon(m_hIcon, FALSE);
	SetIcon(m_hIcon, TRUE);

	CenterWindow(GetParent());
	return FALSE;
}

LRESULT UpdateDlg::OnDownload(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ui.isAutoUpdate && UpdateDlg::canAutoUpdate(ui.updateUrl)) {
		EndDialog(wID);
	} else {
		if(ui.isForced)
			MessageBox(Text::toT(ui.forcedReason + "\r\n\r\n" + STRING(UPDATE_INSTRUCTIONS)).c_str(), Text::toT(STRING(MANDATORY_UPDATE) + " - " APPNAME " " VERSIONSTRING).c_str(), MB_OK | MB_ICONEXCLAMATION);

		WinUtil::openLink(Text::toT(ui.url));
		EndDialog(IDCLOSE);
	}
	return FALSE;
}

LRESULT UpdateDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ui.isForced) {
		MessageBox(Text::toT(ui.forcedReason + "\r\n\r\n" + STRING(UPDATE_INSTRUCTIONS)).c_str(), Text::toT(STRING(MANDATORY_UPDATE) + " - " APPNAME " " VERSIONSTRING).c_str(), MB_OK | MB_ICONEXCLAMATION);
		WinUtil::openLink(Text::toT(ui.url));
	}

	EndDialog(IDCLOSE);
	return FALSE;
}

bool UpdateDlg::canAutoUpdate(const string& url) {
	if(Util::getFileExt(url) == ".zip") {
		tstring buf(256, _T('\0'));
		tstring sDrive = Text::toT(WinUtil::getAppName().substr(0, 3));
		GetVolumeInformation(sDrive.c_str(), NULL, 0, NULL, NULL, NULL, &buf[0], 256);
		return (buf.find(_T("FAT")) == string::npos);
	}
	return false;
}
