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

#if !defined(ABOUT_DLG_H)
#define ABOUT_DLG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <client/HttpConnection.h>
#include <client/HttpManager.h>
#include <client/SimpleXML.h>

#include <wtl/atlctrlx.h>

static const TCHAR thanks[] = 
_T(" #Management#\r\n")
_T(" Lee (Project Manager)\r\n")
_T(" Crise (Lead Developer)\r\n")
_T(" \r\n")
_T(" #VIP#\r\n")
_T(" Almiteycow (PWDC Coder)\r\n")
_T(" Greg, KP, RadoX (Graphics)\r\n")
_T(" Forum Support\r\n")
_T(" Program Testers (Huge thanks)\r\n")
_T(" Subscribers (For keeping the project alive)\r\n")
_T(" Translators\r\n")
_T(" TechGeeks Online (All Operators and Users)");

class AboutDlg : public CDialogImpl<AboutDlg>, private TimerManagerListener
{
public:
	enum { IDD = IDD_ABOUTBOX };
	enum { WM_VERSIONDATA = WM_APP + 53 };

	AboutDlg() { }
	~AboutDlg() { }

	BEGIN_MSG_MAP(AboutDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_VERSIONDATA, onVersionData)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		COMMAND_ID_HANDLER(IDC_LINK, onLink)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		SetDlgItemText(IDC_VERSION, _T("ApexDC++ v") _T(VERSIONSTRING_FULL) _T("\n(c) Copyright 2006-2018 ApexDC++ Development Team\nBased on: StrongDC++  (c) Copyright 2004-2011 Big Muscle"));

		CEdit ctrlThanks(GetDlgItem(IDC_THANKS));
		ctrlThanks.FmtLines(TRUE);
		ctrlThanks.AppendText(thanks, TRUE);
		ctrlThanks.Detach();

		SetDlgItemText(IDC_TTH, WinUtil::tth.c_str());
		SetDlgItemText(IDC_GIT_REF, Text::toT(GIT_REF_COMMIT).c_str());
		SetDlgItemText(IDC_LATEST, CTSTRING(DOWNLOADING));
		SetDlgItemText(IDC_TOTALS, (_T("Upload: ") + Util::formatBytesT(SETTING(TOTAL_UPLOAD)) + _T(", Download: ") + 
			Util::formatBytesT(SETTING(TOTAL_DOWNLOAD))).c_str());
		SetDlgItemText(IDC_LINK, Text::toT(HOMEPAGE).c_str());
		SetDlgItemText(IDC_COMPT, (_T("Build date: ") + WinUtil::getCompileDate()).c_str());
		
		url.SubclassWindow(GetDlgItem(IDC_LINK));
		url.SetHyperLinkExtendedStyle(HLINK_UNDERLINEHOVER);

		url.SetHyperLink(_T(HOMEPAGE));
		url.SetLabel(_T(HOMEPAGE));

		TCHAR buf[128];
		if(SETTING(TOTAL_DOWNLOAD) > 0) {
			swprintf(buf, sizeof(buf), _T("Ratio (up/down): %.2f"), ((double)SETTING(TOTAL_UPLOAD)) / ((double)SETTING(TOTAL_DOWNLOAD)));
			SetDlgItemText(IDC_RATIO, buf);
		}

		swprintf(buf, sizeof(buf), _T("Uptime: %s"), Text::toT(WinUtil::formatTime(time(NULL) - Util::getStartTime())).c_str());
		SetDlgItemText(IDC_UPTIME, buf);
		TimerManager::getInstance()->addListener(this);

		HttpManager::getInstance()->download(VERSION_URL, [this](const HttpConnection* conn, OutputStream* os, Flags::MaskType flags) {
			versionDownload(conn, static_cast<StringOutputStream*>(os)->getString(), flags);
		});

		CenterWindow(GetParent());
		return TRUE;
	}

	LRESULT onVersionData(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		tstring* x = (tstring*) wParam;
		SetDlgItemText(IDC_LATEST, x->c_str());
		delete x;
		return 0;
	}

	LRESULT onLink (WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		url.Navigate();
		return 0;
	}
		
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		TimerManager::getInstance()->removeListener(this);
		EndDialog(wID);
		return 0;
	}

private:
	CHyperLink url;

	AboutDlg(const AboutDlg&) { dcassert(0); }

	void versionDownload(const HttpConnection* conn, const string& aLine, Flags::MaskType stFlags) {
		if((stFlags & HttpManager::CONN_FAILED) == HttpManager::CONN_FAILED) {
			tstring* x = new tstring(Text::toT(conn->getStatus()));
			PostMessage(WM_VERSIONDATA, (WPARAM) x);
		} else if(!aLine.empty()) {
			tstring version = Util::emptyStringT;
			try {
				SimpleXML xml;
				xml.fromXML(aLine);
				if(xml.findChild("DCUpdate")) {
					xml.stepIn();
					if(xml.findChild("Version")) {
						version = Text::toT(xml.getChildData());
						xml.resetCurrentChild();
					}
				}
			} catch(const SimpleXMLException&) { }

			tstring* x = new tstring(version.empty() ? _T("Parse Error!") : version.c_str());
			PostMessage(WM_VERSIONDATA, (WPARAM) x);
		}
	}

	void on(TimerManagerListener::Second /*type*/, uint64_t /*aTick*/) noexcept {
		TCHAR buf[128];
		swprintf(buf, sizeof(buf), _T("Uptime: %s"), Text::toT(WinUtil::formatTime(time(NULL) - Util::getStartTime())).c_str());
		SetDlgItemText(IDC_UPTIME, buf);
	}

};

#endif // !defined(ABOUT_DLG_H)

/**
 * @file
 * $Id$
 */
