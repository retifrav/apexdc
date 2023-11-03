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

#if !defined(SYSTEM_FRAME_H)
#define SYSTEM_FRAME_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "FlatTabCtrl.h"
#include "ExListViewCtrl.h"

#include <client/Text.h>
#include <client/LogManager.h>

class SystemFrame : public MDITabChildWindowImpl<SystemFrame, RGB(0, 0, 0), IDR_NOTEPAD>, public StaticFrame<SystemFrame, ResourceManager::SYSTEM_LOG>,
	private LogManagerListener, private SettingsManagerListener
{
public:
	DECLARE_FRAME_WND_CLASS_EX(_T("SystemFrame"), IDR_NOTEPAD, 0, COLOR_3DFACE);

	SystemFrame() : scroll(true) { }
	~SystemFrame() { }

	typedef MDITabChildWindowImpl<SystemFrame, RGB(0, 0, 0), IDR_NOTEPAD> baseClass;

	BEGIN_MSG_MAP(SystemFrame)
		MESSAGE_HANDLER(WM_SETFOCUS, OnFocus)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		COMMAND_ID_HANDLER(IDC_COPY, onCopy)
		COMMAND_ID_HANDLER(IDC_CLEAR, onClear)
		COMMAND_ID_HANDLER(IDC_AUTO_SCROLL, onAutoScroll)
		COMMAND_ID_HANDLER(IDC_LOG_FILE, onLogToFile)
		NOTIFY_HANDLER(IDC_CLIENT, NM_CUSTOMDRAW, ctrlLog.onCustomDraw)
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	void UpdateLayout(BOOL bResizeBars = TRUE);

	LRESULT OnFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlLog.SetFocus();
		return 0;
	}

	LRESULT onClear(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ctrlLog.SetRedraw(FALSE);
		LogManager::getInstance()->clearLastLogs();
		ctrlLog.DeleteAllItems();
		ctrlLog.SetRedraw(TRUE);
		return 0;
	}

	LRESULT onAutoScroll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		scroll = !scroll;
		return 0;
	}

	LRESULT onLogToFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		SettingsManager::getInstance()->set(SettingsManager::LOG_SYSTEM, !BOOLSETTING(LOG_SYSTEM));
		return 0;
	}

private:
	ExListViewCtrl ctrlLog;
	CMenu ctxMenu;
	bool scroll;

	void addEntry(time_t t, const tstring& msg);
	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept;
	void on(Message, const string& message, uint32_t /*sev*/) noexcept { PostMessage(WM_SPEAKER, (WPARAM)(new pair<time_t, tstring>(GET_TIME(), Text::toT(message)))); }
};

#endif // !defined(SYSTEM_FRAME_H)
